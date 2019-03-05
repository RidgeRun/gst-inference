/*
 * GStreamer
 * Copyright (C) 2018 RidgeRun
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstclassificationoverlay.h"
#include "gst/r2inference/gstinferencemeta.h"
#ifdef OCV_VERSION_LT_3_2
  #include "opencv2/highgui/highgui.hpp"
#else
  #include "opencv2/imgproc.hpp"
  #include "opencv2/highgui.hpp"
#endif

GST_DEBUG_CATEGORY_STATIC (gst_classification_overlay_debug_category);
#define GST_CAT_DEFAULT gst_classification_overlay_debug_category

#define MIN_FONT_SCALE 0
#define DEFAULT_FONT_SCALE 1.5
#define MAX_FONT_SCALE 100
#define MIN_BOX_THICKNESS 1
#define DEFAULT_BOX_THICKNESS 2
#define MAX_BOX_THICKNESS 100
#define DEFAULT_LABELS NULL
#define DEFAULT_NUM_LABELS 0

/* prototypes */


static void gst_classification_overlay_set_property (GObject *object,
    guint property_id, const GValue *value, GParamSpec *pspec);
static void gst_classification_overlay_get_property (GObject *object,
    guint property_id, GValue *value, GParamSpec *pspec);
static void gst_classification_overlay_dispose (GObject *object);
static void gst_classification_overlay_finalize (GObject *object);

static gboolean gst_classification_overlay_start (GstBaseTransform *trans);
static gboolean gst_classification_overlay_stop (GstBaseTransform *trans);
static GstFlowReturn
gst_classification_overlay_transform_frame_ip (GstVideoFilter *trans,
    GstVideoFrame *frame);

enum {
  PROP_0,
  PROP_FONT_SCALE,
  PROP_BOX_THICKNESS,
  PROP_LABELS
};

/* pad templates */

#define VIDEO_SRC_CAPS \
    GST_VIDEO_CAPS_MAKE("{ RGB }")

#define VIDEO_SINK_CAPS \
    GST_VIDEO_CAPS_MAKE("{ RGB }")


/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstClassificationOverlay, gst_classification_overlay,
                         GST_TYPE_VIDEO_FILTER,
                         GST_DEBUG_CATEGORY_INIT (gst_classification_overlay_debug_category,
                             "classification_overlay", 0,
                             "debug category for classification_overlay element"));

static void
gst_classification_overlay_class_init (GstClassificationOverlayClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseTransformClass *base_transform_class =
    GST_BASE_TRANSFORM_CLASS (klass);
  GstVideoFilterClass *video_filter_class = GST_VIDEO_FILTER_CLASS (klass);

  gst_element_class_add_pad_template (GST_ELEMENT_CLASS (klass),
                                      gst_pad_template_new ("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                                          gst_caps_from_string (VIDEO_SRC_CAPS)));
  gst_element_class_add_pad_template (GST_ELEMENT_CLASS (klass),
                                      gst_pad_template_new ("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                                          gst_caps_from_string (VIDEO_SINK_CAPS)));

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
                                         "classificationoverlay", "Filter",
                                         "Overlays classification metadata on input buffer",
                                         "Carlos Rodriguez <carlos.rodriguez@ridgerun.com> \n\t\t\t"
                                         "   Jose Jimenez <jose.jimenez@ridgerun.com> \n\t\t\t"
                                         "   Michael Gruner <michael.gruner@ridgerun.com> \n\t\t\t"
                                         "   Carlos Aguero <carlos.aguero@ridgerun.com> \n\t\t\t"
                                         "   Miguel Taylor <miguel.taylor@ridgerun.com> \n\t\t\t"
                                         "   Greivin Fallas <greivin.fallas@ridgerun.com>");

  gobject_class->set_property = gst_classification_overlay_set_property;
  gobject_class->get_property = gst_classification_overlay_get_property;
  gobject_class->dispose = gst_classification_overlay_dispose;
  gobject_class->finalize = gst_classification_overlay_finalize;

  g_object_class_install_property (gobject_class, PROP_FONT_SCALE,
                                   g_param_spec_double ("font-scale", "font", "Font scale", MIN_FONT_SCALE,
                                       MAX_FONT_SCALE, DEFAULT_FONT_SCALE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_BOX_THICKNESS,
                                   g_param_spec_int ("thickness", "thickness", "Box line thickness in pixels",
                                       MIN_BOX_THICKNESS,
                                       MAX_BOX_THICKNESS, DEFAULT_BOX_THICKNESS, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_LABELS,
                                   g_param_spec_string ("labels", "labels",
                                       "Semicolon separated string containing inference labels", DEFAULT_LABELS,
                                       G_PARAM_READWRITE));

  base_transform_class->start =
    GST_DEBUG_FUNCPTR (gst_classification_overlay_start);
  base_transform_class->stop =
    GST_DEBUG_FUNCPTR (gst_classification_overlay_stop);
  video_filter_class->transform_frame_ip =
    GST_DEBUG_FUNCPTR (gst_classification_overlay_transform_frame_ip);

}

static void
gst_classification_overlay_init (GstClassificationOverlay *
                                 classification_overlay) {
  classification_overlay->font_scale = DEFAULT_FONT_SCALE;
  classification_overlay->box_thickness = DEFAULT_BOX_THICKNESS;
  classification_overlay->labels = DEFAULT_LABELS;
  classification_overlay->labels_list = DEFAULT_LABELS;
  classification_overlay->num_labels = DEFAULT_NUM_LABELS;
}

void
gst_classification_overlay_set_property (GObject *object, guint property_id,
    const GValue *value, GParamSpec *pspec) {
  GstClassificationOverlay *classification_overlay =
    GST_CLASSIFICATION_OVERLAY (object);

  GST_DEBUG_OBJECT (classification_overlay, "set_property");

  switch (property_id) {
    case PROP_FONT_SCALE:
      classification_overlay->font_scale = g_value_get_double (value);
      GST_DEBUG_OBJECT (classification_overlay, "Changed font scale to %lf",
                        classification_overlay->font_scale);
      break;
    case PROP_BOX_THICKNESS:
      classification_overlay->box_thickness = g_value_get_int (value);
      GST_DEBUG_OBJECT (classification_overlay, "Changed box thickness to %d",
                        classification_overlay->box_thickness);
      break;
    case PROP_LABELS:
      classification_overlay->labels = g_value_get_string (value);
      if (classification_overlay->labels_list != NULL) {
        g_strfreev (classification_overlay->labels_list);
      }
      classification_overlay->labels_list = g_strsplit (
                                              classification_overlay->labels, ";", 0);
      classification_overlay->num_labels = g_strv_length (classification_overlay->labels_list);
      GST_DEBUG_OBJECT (classification_overlay, "Changed inference labels %s",
                        classification_overlay->labels);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_classification_overlay_get_property (GObject *object, guint property_id,
    GValue *value, GParamSpec *pspec) {
  GstClassificationOverlay *classification_overlay =
    GST_CLASSIFICATION_OVERLAY (object);

  GST_DEBUG_OBJECT (classification_overlay, "get_property");

  switch (property_id) {
    case PROP_FONT_SCALE:
      g_value_set_double (value, classification_overlay->font_scale);
      break;
    case PROP_BOX_THICKNESS:
      g_value_set_int (value, classification_overlay->box_thickness);
      break;
    case PROP_LABELS:
      g_value_set_string (value, classification_overlay->labels);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_classification_overlay_dispose (GObject *object) {
  GstClassificationOverlay *classification_overlay =
    GST_CLASSIFICATION_OVERLAY (object);

  GST_DEBUG_OBJECT (classification_overlay, "dispose");

  /* clean up as possible.  may be called multiple times */
  if (classification_overlay->labels_list != NULL) {
    g_strfreev (classification_overlay->labels_list);
  }

  G_OBJECT_CLASS (gst_classification_overlay_parent_class)->dispose (object);
}

void
gst_classification_overlay_finalize (GObject *object) {
  GstClassificationOverlay *classification_overlay =
    GST_CLASSIFICATION_OVERLAY (object);

  GST_DEBUG_OBJECT (classification_overlay, "finalize");

  /* clean up object here */

  G_OBJECT_CLASS (gst_classification_overlay_parent_class)->finalize (object);
}

static gboolean
gst_classification_overlay_start (GstBaseTransform *trans) {
  GstClassificationOverlay *classification_overlay =
    GST_CLASSIFICATION_OVERLAY (trans);

  GST_DEBUG_OBJECT (classification_overlay, "start");

  return TRUE;
}

static gboolean
gst_classification_overlay_stop (GstBaseTransform *trans) {
  GstClassificationOverlay *classification_overlay =
    GST_CLASSIFICATION_OVERLAY (trans);

  GST_DEBUG_OBJECT (classification_overlay, "stop");

  return TRUE;
}

/* transform */
static GstFlowReturn
gst_classification_overlay_transform_frame_ip (GstVideoFilter *trans,
    GstVideoFrame *frame) {
  GstClassificationOverlay *classification_overlay =
    GST_CLASSIFICATION_OVERLAY (trans);
  GstClassificationMeta *class_meta;
  gint index, i, width, height;
  gdouble max, current;
  cv::Mat cv_mat;
  cv::String str;
  cv::Size size;
  const gint bpp = 3;
  cv::Scalar black = cv::Scalar (0, 0, 0);
  cv::Scalar white = cv::Scalar (255, 255, 255);

  width = GST_VIDEO_FRAME_COMP_STRIDE (frame, 0) / bpp;
  height = GST_VIDEO_FRAME_HEIGHT (frame);

  class_meta =
    (GstClassificationMeta *) gst_buffer_get_meta (frame->buffer,
        GST_CLASSIFICATION_META_API_TYPE);
  if (NULL == class_meta) {
    GST_LOG_OBJECT (trans, "No classification meta found");
    return GST_FLOW_OK;
  }

  GST_LOG_OBJECT (trans, "Valid classification meta found");

  /* Get the most probable label */
  index = 0;
  max = -1;
  for (i = 0; i < class_meta->num_labels; ++i) {
    current = class_meta->label_probs[i];
    if (current > max) {
      max = current;
      index = i;
    }
  }
  if (classification_overlay->num_labels > index) {
    str = cv::format ("%s prob:%f", classification_overlay->labels_list[index],
                      max);
  } else {
    str = cv::format ("Label #%d prob:%f", index, max);
  }
  /* Get size of string on screen */
  int baseline = 0;
  size =
    cv::getTextSize (str, cv::FONT_HERSHEY_TRIPLEX,
                     classification_overlay->box_thickness, classification_overlay->font_scale,
                     &baseline);
  /* Put string on screen */
  cv_mat = cv::Mat (height, width, CV_8UC3, (char *) frame->data[0]);
  cv::putText (cv_mat, str, cv::Point (0, size.height),
               cv::FONT_HERSHEY_PLAIN, classification_overlay->font_scale, white,
               classification_overlay->box_thickness+1);

  cv::putText (cv_mat, str, cv::Point (0, size.height),
               cv::FONT_HERSHEY_PLAIN, classification_overlay->font_scale, black,
               classification_overlay->box_thickness);

  return GST_FLOW_OK;
}
