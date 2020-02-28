/*
 * GStreamer
 * Copyright (C) 2018-2020 RidgeRun <support@ridgerun.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

/**
 * SECTION:element-gstinferencecrop
 *
 * The inferencecrop element parses a inferencemeta and crops the incomming
 * image to the bounding box.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 v4l2src device=$CAMERA ! videoconvert ! tee name=t t. ! videoscale ! queue !
   net.sink_model t. ! queue ! net.sink_bypass tinyyolov2 name=net model-location=$MODEL_LOCATION ! \
   backend=tensorflow backend::input-layer=$INPUT_LAYER backend::output-layer=OUTPUT_LAYER net.src_bypass \
   inferencecrop aspect-ratio=1/1 ! videoscale ! ximagesink sync=false
 * ]|
 * Crops video frames based on inferencemeta information.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include "gstinferencecrop.h"

#include "gst/r2inference/gstinferencemeta.h"
#include "videocrop.h"

#include <math.h>
/* generic templates */
static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

GST_DEBUG_CATEGORY_STATIC (gst_inference_crop_debug_category);
#define GST_CAT_DEFAULT gst_inference_crop_debug_category

static void gst_inference_crop_finalize (GObject *object);
static void gst_inference_crop_set_property (GObject *object,
    guint property_id, const GValue *value, GParamSpec *pspec);
static void gst_inference_crop_get_property (GObject *object,
    guint property_id, GValue *value, GParamSpec *pspec);
static GstStateChangeReturn gst_inference_crop_change_state (GstElement *
    element, GstStateChange transition);
static gboolean gst_inference_crop_start (GstInferenceCrop *self);
static void gst_inference_crop_set_caps (GstPad *pad, GParamSpec *unused,
    GstInferenceCrop *self);
static void gst_inference_crop_new_buffer_size (GstInferenceCrop *self, gint x,
    gint y,
    gint width, gint height, gint width_ratio, gint height_ratio, gint *top,
    gint *bottom, gint *right, gint *left);
static GstPadProbeReturn gst_inference_crop_new_buffer (GstPad *pad,
    GstPadProbeInfo *info, GstInferenceCrop *self);
static void gst_inference_crop_find_predictions (GstInferenceCrop *self,
    gint *num_inferences, GstInferenceMeta *meta, GList **list,
    GstInferencePrediction *pred);


#define PROP_CROP_RATIO_DEFAULT_WIDTH 1
#define PROP_CROP_RATIO_DEFAULT_HEIGHT 1

enum {
  PROP_0,
  PROP_CROP_ASPECT_RATIO,
};

struct _GstInferenceCrop {
  GstBin parent;
  GstPad *sinkpad;
  GstPad *srcpad;
  CropElement *element;
  gint width_ratio;
  gint height_ratio;
  gint width;
  gint height;
};

struct _GstInferenceCropClass {
  GstBinClass parent;
};

/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstInferenceCrop, gst_inference_crop,
                         GST_TYPE_BIN,
                         GST_DEBUG_CATEGORY_INIT (gst_inference_crop_debug_category, "inferencecrop",
                             0, "debug category for inferencecrop element"));

static void
gst_inference_crop_class_init (GstInferenceCropClass *klass) {
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  gst_element_class_add_pad_template (element_class,
                                      gst_static_pad_template_get (&sink_template));
  gst_element_class_add_pad_template (element_class,
                                      gst_static_pad_template_get (&src_template));

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
                                         "Inference Crop", "Filter",
                                         "Crops an incoming image based on an inference prediction bounding box",
                                         "Michael Gruner <michael.gruner@ridgerun.com> \n\t\t\t"
                                         "   Lenin Torres <lenin.torres@ridgerun.com>");

  element_class->change_state =
    GST_DEBUG_FUNCPTR (gst_inference_crop_change_state);

  object_class->finalize = gst_inference_crop_finalize;
  object_class->set_property = gst_inference_crop_set_property;
  object_class->get_property = gst_inference_crop_get_property;

  g_object_class_install_property (object_class, PROP_CROP_ASPECT_RATIO,
                                   gst_param_spec_fraction ("aspect-ratio", "Aspect Ratio",
                                       "Aspect ratio to crop the predictions, width and height separated by '/'. "
                                       "If set to 0/1 it maintains the aspect ratio of each bounding box.", 0, 1,
                                       G_MAXINT, 1,
                                       PROP_CROP_RATIO_DEFAULT_WIDTH, PROP_CROP_RATIO_DEFAULT_HEIGHT,
                                       G_PARAM_READWRITE ));
}

static void
gst_inference_crop_init (GstInferenceCrop *self) {
  GstElement *element;
  GstPad *sinkpad, *sinkgpad, *srcpad, *srcgpad;

  self->sinkpad = NULL;
  self->element = new VideoCrop ();
  self->width_ratio = PROP_CROP_RATIO_DEFAULT_WIDTH;
  self->height_ratio = PROP_CROP_RATIO_DEFAULT_HEIGHT;
  if (FALSE == self->element->Validate ()) {
    const std::string factory = self->element->GetFactory ();
    GST_ERROR_OBJECT (self, "Unable to find element %s", factory.c_str ());
    return;
  }

  element = self->element->GetElement ();
  gst_bin_add (GST_BIN (self), GST_ELEMENT (gst_object_ref (element)));

  sinkpad = self->element->GetSinkPad ();
  g_return_if_fail (sinkpad);

  self->sinkpad = GST_PAD(gst_object_ref(sinkpad));

  sinkgpad = gst_ghost_pad_new ("sink", sinkpad);
  gst_pad_set_active (sinkgpad, TRUE);
  gst_element_add_pad (GST_ELEMENT (self), sinkgpad);

  g_signal_connect (sinkgpad, "notify::caps",
                    G_CALLBACK (gst_inference_crop_set_caps), self);
  gst_pad_add_probe (sinkgpad, GST_PAD_PROBE_TYPE_BUFFER,
                     (GstPadProbeCallback) gst_inference_crop_new_buffer, self, NULL);

  srcpad = self->element->GetSrcPad ();
  g_return_if_fail (srcpad);

  self->srcpad = GST_PAD(gst_object_ref(srcpad));

  srcgpad = gst_ghost_pad_new ("src", srcpad);
  gst_pad_set_active (srcgpad, TRUE);
  gst_element_add_pad (GST_ELEMENT (self), srcgpad);

  gst_object_unref (sinkpad);
  gst_object_unref (srcpad);
}

static void
gst_inference_crop_finalize (GObject *object) {
  GstInferenceCrop *self = GST_INFERENCE_CROP (object);

  g_return_if_fail(self);

  delete (self->element);
  gst_object_unref (self->sinkpad);
  self->sinkpad = NULL;
  gst_object_unref (self->srcpad);
  self->srcpad = NULL;

  G_OBJECT_CLASS (gst_inference_crop_parent_class)->finalize (object);
}

static void
gst_inference_crop_set_property (GObject *object, guint property_id,
                                 const GValue *value, GParamSpec *pspec) {
  GstInferenceCrop *self = GST_INFERENCE_CROP (object);

  GST_DEBUG_OBJECT (self, "set_property");

  switch (property_id) {
    case PROP_CROP_ASPECT_RATIO:
      GST_OBJECT_LOCK (self);
      if (GST_VALUE_HOLDS_FRACTION (value)) {
        self->width_ratio = gst_value_get_fraction_numerator (value);
        self->height_ratio = gst_value_get_fraction_denominator (value);
      }
      GST_OBJECT_UNLOCK (self);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
gst_inference_crop_get_property (GObject *object, guint property_id,
                                 GValue *value, GParamSpec *pspec) {
  GstInferenceCrop *self = GST_INFERENCE_CROP (object);

  GST_DEBUG_OBJECT (self, "get_property");

  switch (property_id) {
    case PROP_CROP_ASPECT_RATIO:
      GST_OBJECT_LOCK (self);
      gst_value_set_fraction (value, self->width_ratio, self->height_ratio);
      GST_OBJECT_UNLOCK (self);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static gboolean
gst_inference_crop_start (GstInferenceCrop *self) {
  g_return_val_if_fail (self, FALSE);

  return self->element->Validate ();
}

static GstStateChangeReturn
gst_inference_crop_change_state (GstElement *element,
                                 GstStateChange transition) {
  GstStateChangeReturn ret;
  GstInferenceCrop *self = GST_INFERENCE_CROP (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      if (FALSE == gst_inference_crop_start (self)) {
        GST_ERROR_OBJECT (self, "Failed to start");
        ret = GST_STATE_CHANGE_FAILURE;
        goto out;
      }
    default:
      break;
  }
  ret =
    GST_ELEMENT_CLASS (gst_inference_crop_parent_class)->change_state
    (element, transition);
  if (GST_STATE_CHANGE_FAILURE == ret) {
    GST_ERROR_OBJECT (self, "Parent failed to change state");
    goto out;
  }

out:
  return ret;
}

static void
gst_inference_crop_set_caps (GstPad *pad, GParamSpec *unused,
                             GstInferenceCrop *self) {
  GstCaps *caps;
  GstStructure *st;
  gint width;
  gint height;

  g_object_get (pad, "caps", &caps, NULL);
  if (NULL == caps) {
    return;
  }

  st = gst_caps_get_structure (caps, 0);
  gst_structure_get_int (st, "width", &width);
  gst_structure_get_int (st, "height", &height);

  GST_INFO_OBJECT (self, "Set new caps to %" GST_PTR_FORMAT, caps);
  self->width = width;
  self->height = height;
  gst_caps_unref(caps);
}

static void
gst_inference_crop_new_buffer_size (GstInferenceCrop *self, gint x, gint y,
                                    gint width, gint height, gint width_ratio, gint height_ratio, gint *top,
                                    gint *bottom, gint *right, gint *left) {

  *top = y;
  *bottom = self->height - y - height;
  *left = x;
  *right = self->width - x - width;

  if (width_ratio > 0 && height_ratio > 0) {
    gint top_bottom_modify = round(((height_ratio * width) / width_ratio - height) /
                                   2);
    gint left_right_modify = round(((width_ratio * height) / height_ratio - width) /
                                   2);
    if (width_ratio <= height_ratio) {
      if (width > height) {
        *top = *top - top_bottom_modify;
        *bottom = *bottom - top_bottom_modify;
      } else {
        *left = *left - left_right_modify;
        *right = *right - left_right_modify;
      }
    } else {
      if (width < height) {
        *left = *left - left_right_modify;
        *right = *right - left_right_modify;
      } else {
        *top = *top - top_bottom_modify;
        *bottom = *bottom - top_bottom_modify;
      }
    }
  }

  if (*top < 0) {
    *top = 0;
  }

  if (*bottom < 0) {
    *bottom = 0;
  }

  if (*left < 0) {
    *left = 0;
  }

  if (*right < 0) {
    *right = 0;
  }

}

static void
gst_inference_crop_find_predictions (GstInferenceCrop *self,
                                     gint *num_inferences,
                                     GstInferenceMeta *meta, GList **list, GstInferencePrediction *pred) {
  GSList *children_list = NULL;
  GSList *iter = NULL;

  g_return_if_fail (self);
  g_return_if_fail (num_inferences);
  g_return_if_fail (meta);
  g_return_if_fail (list);
  g_return_if_fail (pred);

  children_list = gst_inference_prediction_get_children(pred);

  for (iter = children_list; iter != NULL; iter = g_slist_next(iter)) {
    GstInferencePrediction *predict = (GstInferencePrediction *)iter->data;

    gst_inference_crop_find_predictions (self, num_inferences, meta, list,
                                         predict );
  }
  if (FALSE == G_NODE_IS_ROOT(pred->predictions) && TRUE == pred->enabled ) {
    *list = g_list_append (*list, pred);
    *num_inferences = *num_inferences + 1;
  }

}

static GstPadProbeReturn
gst_inference_crop_new_buffer (GstPad *pad, GstPadProbeInfo *info,
                               GstInferenceCrop *self) {
  GstBuffer *buffer;
  GstInferenceMeta *inference_meta;
  gint num_inferences;
  gint crop_width_ratio;
  gint crop_height_ratio;
  BoundingBox box;
  GstPadProbeReturn ret = GST_PAD_PROBE_DROP;
  GList *list = NULL;
  GList *iter = NULL;
  gboolean gap = TRUE;

  GST_OBJECT_LOCK (self);
  crop_width_ratio = self->width_ratio;
  crop_height_ratio = self->height_ratio;
  GST_OBJECT_UNLOCK (self);

  buffer = gst_pad_probe_info_get_buffer (info);

  inference_meta =
    (GstInferenceMeta *) gst_buffer_get_meta (buffer,
        GST_INFERENCE_META_API_TYPE);

  if (NULL == inference_meta ) {
    GST_LOG_OBJECT (self, "No meta found, dropping buffer");
    goto out;
  }

  num_inferences = 0;
  gst_inference_crop_find_predictions (self, &num_inferences,
                                       inference_meta, &list, inference_meta->prediction);

  for (iter = list; iter != NULL; iter = g_list_next(iter)) {
    GstInferencePrediction *pred = (GstInferencePrediction *)iter->data;
    GstBuffer *croped_buffer;
    GstInferenceMeta *dmeta;
    gint top, bottom, right, left = 0;
    box = pred->bbox;
    GST_LOG_OBJECT (self, "BBox: %dx%dx%dx%d", box.x, box.y, box.width,
                    box.height);

    gst_inference_crop_new_buffer_size (self, box.x, box.y, box.width, box.height,
                                        crop_width_ratio, crop_height_ratio,
                                        &top, &bottom, &right, &left);
    self->element->SetCroppingSize ((gint) top, (gint) bottom, (gint) right,
                                    (gint) left);

    croped_buffer = gst_buffer_copy (buffer);

    dmeta = (GstInferenceMeta *) gst_buffer_get_meta (croped_buffer,
        GST_INFERENCE_META_API_TYPE);

    gst_inference_prediction_unref (dmeta->prediction);

    dmeta->prediction = gst_inference_prediction_copy (pred);

    dmeta->prediction->bbox.x = 0;
    dmeta->prediction->bbox.y = 0;
    dmeta->prediction->bbox.width = self->width - right - left;
    dmeta->prediction->bbox.height = self->height - top - bottom;

    if (GST_FLOW_OK != gst_pad_chain(self->sinkpad, croped_buffer)) {
      GST_ELEMENT_ERROR(self, CORE, FAILED,
                        ("Failed to push a new buffer into crop element"), (NULL));
    }
    gap = FALSE;
  }

  ret = GST_PAD_PROBE_DROP;

out:
  if (gap) {
    gst_pad_push_event (self->srcpad, gst_event_new_gap (GST_BUFFER_TIMESTAMP (buffer),
							 GST_BUFFER_DURATION (buffer)));
  }

  return ret;
}
