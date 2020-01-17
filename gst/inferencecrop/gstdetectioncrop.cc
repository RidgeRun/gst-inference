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
 * SECTION:element-gstdetectioncrop
 *
 * The detectioncrop element parses a detectionmeta and crops the incomming
 * image to the bounding box.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 v4l2src device=$CAMERA ! videoconvert ! tee name=t t. ! videoscale ! queue !
   net.sink_model t. ! queue ! net.sink_bypass tinyyolov2 name=net model-location=$MODEL_LOCATION ! \
   backend=tensorflow backend::input-layer=$INPUT_LAYER backend::output-layer=OUTPUT_LAYER net.src_bypass \
   detectioncrop aspect-ratio="1:1" ! videoscale ! ximagesink sync=false
 * ]|
 * Process video frames from the camera using a detectioncrop model.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstdetectioncrop.h"

#include "gst/r2inference/gstinferencemeta.h"
#include "videocrop.h"

/* generic templates */
static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

GST_DEBUG_CATEGORY_STATIC (gst_detection_crop_debug_category);
#define GST_CAT_DEFAULT gst_detection_crop_debug_category

static void gst_detection_crop_finalize (GObject *object);
static void gst_detection_crop_set_property (GObject *object,
    guint property_id, const GValue *value, GParamSpec *pspec);
static void gst_detection_crop_get_property (GObject *object,
    guint property_id, GValue *value, GParamSpec *pspec);
static GstStateChangeReturn gst_detection_crop_change_state (GstElement *
    element, GstStateChange transition);
static gboolean gst_detection_crop_start (GstDetectionCrop *self);
static void gst_detection_crop_set_caps (GstPad *pad, GParamSpec *unused,
    GstDetectionCrop *self);
static GstPadProbeReturn gst_detection_crop_new_buffer (GstPad *pad,
    GstPadProbeInfo *info, GstDetectionCrop *self);
static gint gst_detection_crop_find_by_index (GstDetectionCrop *self,
    guint crop_index, GstDetectionMeta *meta);
static gint gst_detection_crop_find_by_class (GstDetectionCrop *self,
    gint crop_class, gboolean biggest_object, GstDetectionMeta *meta);
static gint gst_detection_crop_find_index (GstDetectionCrop *self,
    guint crop_index, gint crop_class, gboolean biggest_object,
    GstDetectionMeta *meta);

#define PROP_CROP_INDEX_DEFAULT 0
#define PROP_CROP_INDEX_MAX G_MAXUINT
#define PROP_CROP_INDEX_MIN 0

#define PROP_CROP_CLASS_DEFAULT -1
#define PROP_CROP_CLASS_MAX G_MAXINT
#define PROP_CROP_CLASS_MIN -1

#define PROP_CROP_BIGGEST_DEFAULT FALSE

enum {
  PROP_0,
  PROP_CROP_INDEX,
  PROP_CROP_CLASS,
  PROP_CROP_ASPECT_RATIO,
  PROP_CROP_BIGGEST,
};

struct _GstDetectionCrop {
  GstBin parent;
  CropElement *element;
  guint crop_index;
  gint crop_class;
  gint width_ratio;
  gint height_ratio;
  gboolean biggest_object;
};

struct _GstDetectionCropClass {
  GstBinClass parent;
};

/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstDetectionCrop, gst_detection_crop,
                         GST_TYPE_BIN,
                         GST_DEBUG_CATEGORY_INIT (gst_detection_crop_debug_category, "detectioncrop",
                             0, "debug category for detectioncrop element"));

static void
gst_detection_crop_class_init (GstDetectionCropClass *klass) {
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  gst_element_class_add_pad_template (element_class,
                                      gst_static_pad_template_get (&sink_template));
  gst_element_class_add_pad_template (element_class,
                                      gst_static_pad_template_get (&src_template));

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
                                         "detectioncrop", "Filter",
                                         "Crops an incoming image based on an inference prediction bounding box",
                                         "Michael Gruner <michael.gruner@ridgerun.com> \n\t\t\t"
                                         "   Lenin Torres <lenin.torres@ridgerun.com>");

  element_class->change_state =
    GST_DEBUG_FUNCPTR (gst_detection_crop_change_state);

  object_class->finalize = gst_detection_crop_finalize;
  object_class->set_property = gst_detection_crop_set_property;
  object_class->get_property = gst_detection_crop_get_property;

  g_object_class_install_property (object_class, PROP_CROP_INDEX,
                                   g_param_spec_uint ("crop-index", "Crop Index", "Index of the detected "
                                       "object to crop in the prediction array. This will be ignored if "
                                       "crop-class is set to a non-negative value", PROP_CROP_INDEX_MIN,
                                       PROP_CROP_INDEX_MAX, PROP_CROP_INDEX_DEFAULT, G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_CROP_CLASS,
                                   g_param_spec_int ("crop-class", "Crop Class",
                                       "Object class to crop look for "
                                       "cropping. If set to -1, crop-index will be used. If set to a non-negative "
                                       "value, the detections will be iterated until a valid class is found and then "
                                       "used that one for cropping.", PROP_CROP_CLASS_MIN,
                                       PROP_CROP_CLASS_MAX, PROP_CROP_CLASS_DEFAULT, G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_CROP_ASPECT_RATIO,
                                   gst_param_spec_fraction ("aspect-ratio", "Aspect Ratio",
                                       "Aspect ratio to crop the detections, width and height separated by '/'. "
                                       "If set to 0/1 it maintains the aspect ratio of each bounding box.", 0, 1,
                                       G_MAXINT, 1,
                                       PROP_CROP_RATIO_DEFAULT_WIDTH, PROP_CROP_RATIO_DEFAULT_HEIGHT,
                                       G_PARAM_READWRITE ));
  g_object_class_install_property (object_class, PROP_CROP_BIGGEST,
                                   g_param_spec_boolean ("biggest-object", "Biggest Object",
                                       "Crop the biggest object detected of the class given in crop-class."
                                       "The biggest object usually implies the nearest object to the camera."
                                       "This only applies if crop-class is non-negative.",
                                       PROP_CROP_BIGGEST_DEFAULT,
                                       G_PARAM_READWRITE ));
}

static void
gst_detection_crop_init (GstDetectionCrop *self) {
  GstElement *element;
  GstPad *sinkpad, *sinkgpad, *srcpad, *srcgpad;

  self->element = new VideoCrop ();
  self->crop_index = PROP_CROP_INDEX_DEFAULT;
  self->crop_class = PROP_CROP_CLASS_DEFAULT;
  self->width_ratio = PROP_CROP_RATIO_DEFAULT_WIDTH;
  self->height_ratio = PROP_CROP_RATIO_DEFAULT_HEIGHT;
  self->biggest_object = PROP_CROP_BIGGEST_DEFAULT;
  if (FALSE == self->element->Validate ()) {
    const std::string factory = self->element->GetFactory ();
    GST_ERROR_OBJECT (self, "Unable to find element %s", factory.c_str ());
    return;
  }

  element = self->element->GetElement ();
  gst_bin_add (GST_BIN (self), GST_ELEMENT (gst_object_ref (element)));

  sinkpad = self->element->GetSinkPad ();
  g_return_if_fail (sinkpad);

  sinkgpad = gst_ghost_pad_new ("sink", sinkpad);
  gst_pad_set_active (sinkgpad, TRUE);
  gst_element_add_pad (GST_ELEMENT (self), sinkgpad);

  g_signal_connect (sinkgpad, "notify::caps",
                    G_CALLBACK (gst_detection_crop_set_caps), self);
  gst_pad_add_probe (sinkgpad, GST_PAD_PROBE_TYPE_BUFFER,
                     (GstPadProbeCallback) gst_detection_crop_new_buffer, self, NULL);

  srcpad = self->element->GetSrcPad ();
  g_return_if_fail (srcpad);

  srcgpad = gst_ghost_pad_new ("src", srcpad);
  gst_pad_set_active (srcgpad, TRUE);
  gst_element_add_pad (GST_ELEMENT (self), srcgpad);

  gst_object_unref (sinkpad);
  gst_object_unref (srcpad);
}

static void
gst_detection_crop_finalize (GObject *object) {
  GstDetectionCrop *self = GST_DETECTION_CROP (object);

  delete (self->element);

  G_OBJECT_CLASS (gst_detection_crop_parent_class)->finalize (object);
}

static void
gst_detection_crop_set_property (GObject *object, guint property_id,
                                 const GValue *value, GParamSpec *pspec) {
  GstDetectionCrop *self = GST_DETECTION_CROP (object);

  GST_DEBUG_OBJECT (self, "set_property");

  switch (property_id) {
    case PROP_CROP_INDEX:
      GST_OBJECT_LOCK (self);
      self->crop_index = g_value_get_uint (value);
      GST_OBJECT_UNLOCK (self);
      break;
    case PROP_CROP_CLASS:
      GST_OBJECT_LOCK (self);
      self->crop_class = g_value_get_int (value);
      GST_OBJECT_UNLOCK (self);
      break;
    case PROP_CROP_ASPECT_RATIO:
      GST_OBJECT_LOCK (self);
      if (GST_VALUE_HOLDS_FRACTION (value)) {
        self->width_ratio = gst_value_get_fraction_numerator (value);
        self->height_ratio = gst_value_get_fraction_denominator (value);
      }
      GST_OBJECT_UNLOCK (self);
      break;
    case PROP_CROP_BIGGEST:
      GST_OBJECT_LOCK (self);
      self->biggest_object = g_value_get_boolean (value);
      GST_OBJECT_UNLOCK (self);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
gst_detection_crop_get_property (GObject *object, guint property_id,
                                 GValue *value, GParamSpec *pspec) {
  GstDetectionCrop *self = GST_DETECTION_CROP (object);

  GST_DEBUG_OBJECT (self, "get_property");

  switch (property_id) {
    case PROP_CROP_INDEX:
      GST_OBJECT_LOCK (self);
      g_value_set_uint (value, self->crop_index);
      GST_OBJECT_UNLOCK (self);
      break;
    case PROP_CROP_CLASS:
      GST_OBJECT_LOCK (self);
      g_value_set_int (value, self->crop_class);
      GST_OBJECT_UNLOCK (self);
      break;
    case PROP_CROP_ASPECT_RATIO:
      GST_OBJECT_LOCK (self);
      gst_value_set_fraction (value, self->width_ratio, self->height_ratio);
      GST_OBJECT_UNLOCK (self);
      break;
    case PROP_CROP_BIGGEST:
      GST_OBJECT_LOCK (self);
      g_value_set_boolean (value, self->biggest_object);
      GST_OBJECT_UNLOCK (self);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static gboolean
gst_detection_crop_start (GstDetectionCrop *self) {
  g_return_val_if_fail (self, FALSE);

  return self->element->Validate ();
}

static GstStateChangeReturn
gst_detection_crop_change_state (GstElement *element,
                                 GstStateChange transition) {
  GstStateChangeReturn ret;
  GstDetectionCrop *self = GST_DETECTION_CROP (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      if (FALSE == gst_detection_crop_start (self)) {
        GST_ERROR_OBJECT (self, "Failed to start");
        ret = GST_STATE_CHANGE_FAILURE;
        goto out;
      }
    default:
      break;
  }
  ret =
    GST_ELEMENT_CLASS (gst_detection_crop_parent_class)->change_state
    (element, transition);
  if (GST_STATE_CHANGE_FAILURE == ret) {
    GST_ERROR_OBJECT (self, "Parent failed to change state");
    goto out;
  }

out:
  return ret;
}

static void
gst_detection_crop_set_caps (GstPad *pad, GParamSpec *unused,
                             GstDetectionCrop *self) {
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

  self->element->SetImageSize (width, height);
  gst_caps_unref(caps);
}

static gint
gst_detection_crop_find_by_index (GstDetectionCrop *self, guint crop_index,
                                  GstDetectionMeta *meta) {
  gint ret = crop_index;

  g_return_val_if_fail (self, -1);
  g_return_val_if_fail (meta, -1);

  if ((gint)crop_index >= meta->num_boxes) {
    GST_LOG_OBJECT (self, "Configured crop index is larger than "
                    "the amount of objects in the prediction");

    ret = -1;
  }

  return ret;
}

static gint
gst_detection_crop_find_by_class (GstDetectionCrop *self, gint crop_class,
                                  gboolean biggest_object,
                                  GstDetectionMeta *meta) {
  gint i;
  gint ret = -1;
  gint biggest = -1;
  gint box_size = -1;

  g_return_val_if_fail (self, -1);
  g_return_val_if_fail (meta, -1);

  for (i = 0; i < meta->num_boxes; ++i) {
    if (meta->boxes[i].label == crop_class) {
      box_size = meta->boxes[i].width * meta->boxes[i].height;
      if (box_size > biggest) {
        biggest = box_size;
        ret = i;
        if (false == biggest_object) {
          break;
        }
      }
    }
  }

  if (-1 == ret) {
    GST_LOG_OBJECT (self, "No valid class detected");
  }

  return ret;
}

static gint
gst_detection_crop_find_index (GstDetectionCrop *self, guint crop_index,
                               gint crop_class, gboolean biggest_object, GstDetectionMeta *meta) {
  g_return_val_if_fail (self, -1);
  g_return_val_if_fail (meta, -1);

  if (-1 == crop_class) {
    return gst_detection_crop_find_by_index (self, crop_index, meta);
  } else {
    return gst_detection_crop_find_by_class (self, crop_class, biggest_object,
           meta);
  }
}

static GstPadProbeReturn
gst_detection_crop_new_buffer (GstPad *pad, GstPadProbeInfo *info,
                               GstDetectionCrop *self) {
  GstBuffer *buffer;
  GstDetectionMeta *meta;
  guint crop_index;
  gint crop_class;
  gint crop_width_ratio;
  gint crop_height_ratio;
  gint requested_index;
  gboolean biggest_object;
  BBox box;
  GstPadProbeReturn ret = GST_PAD_PROBE_DROP;

  GST_OBJECT_LOCK (self);
  crop_index = self->crop_index;
  crop_class = self->crop_class;
  crop_width_ratio = self->width_ratio;
  crop_height_ratio = self->height_ratio;
  biggest_object = self->biggest_object;
  GST_OBJECT_UNLOCK (self);

  buffer = gst_pad_probe_info_get_buffer (info);

  meta =
    (GstDetectionMeta *) gst_buffer_get_meta (buffer,
        GST_DETECTION_META_API_TYPE);

  if (NULL == meta) {
    GST_LOG_OBJECT (self, "No meta found, dropping buffer");
    goto out;
  }

  if (meta->num_boxes <= 0) {
    GST_LOG_OBJECT (self, "Meta has no valid objects");
    goto out;
  }

  requested_index =
    gst_detection_crop_find_index (self, crop_index, crop_class, biggest_object,
                                   meta);
  if (-1 == requested_index) {
    goto out;
  }

  box = meta->boxes[requested_index];
  GST_LOG_OBJECT (self, "BBox: %fx%fx%fx%f", box.x, box.y, box.width,
                  box.height);
  self->element->SetBoundingBox ((gint) box.x, (gint) box.y, (gint) box.width,
                                 (gint) box.height, (gint) crop_width_ratio, (gint) crop_height_ratio);

  ret = GST_PAD_PROBE_OK;

out:
  return ret;
}

static gboolean
plugin_init (GstPlugin *plugin) {
  gboolean ret = TRUE;

  ret =
    gst_element_register (plugin, "detectioncrop", GST_RANK_NONE,
                          GST_TYPE_DETECTION_CROP);

  return ret;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
                   GST_VERSION_MINOR,
                   inferencecrop,
                   "Crops an incoming image based on an inference prediction bounding box",
                   plugin_init, VERSION, "LGPL", PACKAGE_NAME, GST_PACKAGE_ORIGIN)
