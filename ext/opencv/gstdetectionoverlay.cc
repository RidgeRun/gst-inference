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

#include "gstdetectionoverlay.h"
#include "gst/r2inference/gstinferencemeta.h"
#ifdef OCV_VERSION_LT_3_2
  #include "opencv2/highgui/highgui.hpp"
#else
  #include "opencv2/imgproc.hpp"
  #include "opencv2/highgui.hpp"
#endif

GST_DEBUG_CATEGORY_STATIC (gst_detection_overlay_debug_category);
#define GST_CAT_DEFAULT gst_detection_overlay_debug_category

#define MIN_FONT_SCALE 0
#define DEFAULT_FONT_SCALE 2
#define MAX_FONT_SCALE 100
#define MIN_BOX_THICKNESS 1
#define DEFAULT_BOX_THICKNESS 2
#define MAX_BOX_THICKNESS 100
#define DEFAULT_LABELS NULL
#define DEFAULT_NUM_LABELS 0

#define N_C 20
const cv::Scalar colors[] = {
  cv::Scalar (254, 254, 254), cv::Scalar (239, 211, 127),
  cv::Scalar (225, 169, 0), cv::Scalar (211, 127, 254),
  cv::Scalar (197, 84, 127), cv::Scalar (183, 42, 0),
  cv::Scalar (169, 0.0, 254), cv::Scalar (155, 42, 127),
  cv::Scalar (141, 84, 0), cv::Scalar (127, 254, 254),
  cv::Scalar (112, 211, 127), cv::Scalar (98, 169, 0),
  cv::Scalar (84, 127, 254), cv::Scalar (70, 84, 127),
  cv::Scalar (56, 42, 0), cv::Scalar (42, 0, 254),
  cv::Scalar (28, 42, 127), cv::Scalar (14, 84, 0),
  cv::Scalar (0, 254, 254), cv::Scalar (14, 211, 127)
};

/* prototypes */


static void gst_detection_overlay_set_property (GObject *object,
    guint property_id, const GValue *value, GParamSpec *pspec);
static void gst_detection_overlay_get_property (GObject *object,
    guint property_id, GValue *value, GParamSpec *pspec);
static void gst_detection_overlay_dispose (GObject *object);
static void gst_detection_overlay_finalize (GObject *object);

static gboolean gst_detection_overlay_start (GstBaseTransform *trans);
static gboolean gst_detection_overlay_stop (GstBaseTransform *trans);
static GstFlowReturn
gst_detection_overlay_transform_frame_ip (GstVideoFilter *trans,
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

G_DEFINE_TYPE_WITH_CODE (GstDetectionOverlay, gst_detection_overlay,
                         GST_TYPE_VIDEO_FILTER,
                         GST_DEBUG_CATEGORY_INIT (gst_detection_overlay_debug_category,
                             "detection_overlay", 0,
                             "debug category for detection_overlay element"));

static void
gst_detection_overlay_class_init (GstDetectionOverlayClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseTransformClass *base_transform_class = GST_BASE_TRANSFORM_CLASS (klass);
  GstVideoFilterClass *video_filter_class = GST_VIDEO_FILTER_CLASS (klass);

  gst_element_class_add_pad_template (GST_ELEMENT_CLASS (klass),
                                      gst_pad_template_new ("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                                          gst_caps_from_string (VIDEO_SRC_CAPS)));
  gst_element_class_add_pad_template (GST_ELEMENT_CLASS (klass),
                                      gst_pad_template_new ("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                                          gst_caps_from_string (VIDEO_SINK_CAPS)));

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
                                         "detectionoverlay", "Filter",
                                         "Overlays detection metadata on input buffer",
                                         "Carlos Rodriguez <carlos.rodriguez@ridgerun.com> \n\t\t\t"
                                         "   Jose Jimenez <jose.jimenez@ridgerun.com> \n\t\t\t"
                                         "   Michael Gruner <michael.gruner@ridgerun.com> \n\t\t\t"
                                         "   Carlos Aguero <carlos.aguero@ridgerun.com> \n\t\t\t"
                                         "   Miguel Taylor <miguel.taylor@ridgerun.com> \n\t\t\t"
                                         "   Greivin Fallas <greivin.fallas@ridgerun.com>");

  gobject_class->set_property = gst_detection_overlay_set_property;
  gobject_class->get_property = gst_detection_overlay_get_property;
  gobject_class->dispose = gst_detection_overlay_dispose;
  gobject_class->finalize = gst_detection_overlay_finalize;

  g_object_class_install_property (gobject_class, PROP_FONT_SCALE,
                                   g_param_spec_double ("font-scale", "font", "Font scale", MIN_FONT_SCALE,
                                       MAX_FONT_SCALE, DEFAULT_FONT_SCALE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_BOX_THICKNESS,
                                   g_param_spec_int ("thickness", "thickness", "Box line thickness in pixels",
                                       MIN_BOX_THICKNESS, MAX_BOX_THICKNESS, DEFAULT_BOX_THICKNESS,
                                       G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_LABELS,
                                   g_param_spec_string ("labels", "labels",
                                       "Semicolon separated string containing inference labels", DEFAULT_LABELS,
                                       G_PARAM_READWRITE));

  base_transform_class->start = GST_DEBUG_FUNCPTR (gst_detection_overlay_start);
  base_transform_class->stop = GST_DEBUG_FUNCPTR (gst_detection_overlay_stop);
  video_filter_class->transform_frame_ip = GST_DEBUG_FUNCPTR (
        gst_detection_overlay_transform_frame_ip);

}

static void
gst_detection_overlay_init (GstDetectionOverlay *detection_overlay) {
  detection_overlay->font_scale = DEFAULT_FONT_SCALE;
  detection_overlay->box_thickness = DEFAULT_BOX_THICKNESS;
  detection_overlay->labels = DEFAULT_LABELS;
  detection_overlay->labels_list = DEFAULT_LABELS;
  detection_overlay->num_labels = DEFAULT_NUM_LABELS;
}

void
gst_detection_overlay_set_property (GObject *object, guint property_id,
                                    const GValue *value, GParamSpec *pspec) {
  GstDetectionOverlay *detection_overlay = GST_DETECTION_OVERLAY (object);

  GST_DEBUG_OBJECT (detection_overlay, "set_property");

  switch (property_id) {
    case PROP_FONT_SCALE:
      detection_overlay->font_scale = g_value_get_double (value);
      GST_DEBUG_OBJECT (detection_overlay, "Changed font scale to %lf",
                        detection_overlay->font_scale);
      break;
    case PROP_BOX_THICKNESS:
      detection_overlay->box_thickness = g_value_get_int (value);
      GST_DEBUG_OBJECT (detection_overlay, "Changed box thickness to %d",
                        detection_overlay->box_thickness);
      break;
    case PROP_LABELS:
      detection_overlay->labels = g_value_get_string (value);
      if (detection_overlay->labels_list != NULL) {
        g_strfreev (detection_overlay->labels_list);
      }
      detection_overlay->labels_list = g_strsplit (detection_overlay->labels, ";", 0);
      detection_overlay->num_labels = g_strv_length (detection_overlay->labels_list);
      GST_DEBUG_OBJECT (detection_overlay, "Changed inference labels %s",
                        detection_overlay->labels);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_detection_overlay_get_property (GObject *object, guint property_id,
                                    GValue *value, GParamSpec *pspec) {
  GstDetectionOverlay *detection_overlay = GST_DETECTION_OVERLAY (object);

  GST_DEBUG_OBJECT (detection_overlay, "get_property");

  switch (property_id) {
    case PROP_FONT_SCALE:
      g_value_set_double (value, detection_overlay->font_scale);
      break;
    case PROP_BOX_THICKNESS:
      g_value_set_int (value, detection_overlay->box_thickness);
      break;
    case PROP_LABELS:
      g_value_set_string (value, detection_overlay->labels);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_detection_overlay_dispose (GObject *object) {
  GstDetectionOverlay *detection_overlay = GST_DETECTION_OVERLAY (object);

  GST_DEBUG_OBJECT (detection_overlay, "dispose");

  /* clean up as possible.  may be called multiple times */
  if (detection_overlay->labels_list != NULL) {
    g_strfreev (detection_overlay->labels_list);
  }

  G_OBJECT_CLASS (gst_detection_overlay_parent_class)->dispose (object);
}

void
gst_detection_overlay_finalize (GObject *object) {
  GstDetectionOverlay *detection_overlay = GST_DETECTION_OVERLAY (object);

  GST_DEBUG_OBJECT (detection_overlay, "finalize");

  /* clean up object here */

  G_OBJECT_CLASS (gst_detection_overlay_parent_class)->finalize (object);
}

static gboolean
gst_detection_overlay_start (GstBaseTransform *trans) {
  GstDetectionOverlay *detection_overlay = GST_DETECTION_OVERLAY (trans);

  GST_DEBUG_OBJECT (detection_overlay, "start");

  return TRUE;
}

static gboolean
gst_detection_overlay_stop (GstBaseTransform *trans) {
  GstDetectionOverlay *detection_overlay = GST_DETECTION_OVERLAY (trans);

  GST_DEBUG_OBJECT (detection_overlay, "stop");

  return TRUE;
}

/* transform */
static GstFlowReturn
gst_detection_overlay_transform_frame_ip (GstVideoFilter *trans,
    GstVideoFrame *frame) {
  GstDetectionOverlay *detection_overlay = GST_DETECTION_OVERLAY (trans);
  GstDetectionMeta *detect_meta;
  gint i, width, height;
  cv::Mat cv_mat;
  cv::Size size;
  cv::String str;
  BBox box;
  const gint bpp = 3;

  width = GST_VIDEO_FRAME_COMP_STRIDE (frame, 0) / bpp;
  height = GST_VIDEO_FRAME_HEIGHT (frame);

  detect_meta =
    (GstDetectionMeta *) gst_buffer_get_meta (frame->buffer,
        GST_DETECTION_META_API_TYPE);
  if (NULL == detect_meta) {
    GST_LOG_OBJECT (trans, "No detection meta found");
    return GST_FLOW_OK;
  }

  GST_LOG_OBJECT (trans, "Valid detection meta found");

  cv_mat = cv::Mat (height, width, CV_8UC3, (char *) frame->data[0]);
  for (i = 0; i < detect_meta->num_boxes; ++i) {
    box = detect_meta->boxes[i];

    if (detection_overlay->num_labels > box.label) {
      str = detection_overlay->labels_list[box.label];
    } else {
      str = cv::format ("Label #%d", box.label);
    }

    /* Put string on screen */
    cv::putText (cv_mat, str, cv::Point (box.x, box.y - 5), cv::FONT_HERSHEY_PLAIN,
                 detection_overlay->font_scale, colors[box.label % N_C],
                 detection_overlay->box_thickness);
    cv::rectangle (cv_mat, cv::Point (box.x, box.y), cv::Point (box.x + box.width,
                   box.y + box.height), colors[box.label % N_C], detection_overlay->box_thickness);
  }

  return GST_FLOW_OK;
}
