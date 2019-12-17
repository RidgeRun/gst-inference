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

#include "gstinferenceoverlay.h"
#include "gst/r2inference/gstinferencemeta.h"
#ifdef OCV_VERSION_LT_3_2
#include "opencv2/highgui/highgui.hpp"
#else
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#endif

static const
cv::Scalar
colors[] = {
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

#define N_C (sizeof (colors)/sizeof (cv::Scalar))

GST_DEBUG_CATEGORY_STATIC (gst_inference_overlay_debug_category);
#define GST_CAT_DEFAULT gst_inference_overlay_debug_category

/* prototypes */
static
GstFlowReturn
gst_inference_overlay_process_meta (GstInferenceBaseOverlay *inference_overlay,
                                    GstVideoFrame *frame, GstMeta *meta, gdouble font_scale, gint thickness,
                                    gchar **labels_list, gint num_labels);

enum {
  PROP_0
};

struct _GstInferenceOverlay {
  GstInferenceBaseOverlay
  parent;
};

struct _GstInferenceOverlayClass {
  GstInferenceBaseOverlay
  parent;
};

/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstInferenceOverlay, gst_inference_overlay,
                         GST_TYPE_INFERENCE_BASE_OVERLAY,
                         GST_DEBUG_CATEGORY_INIT (gst_inference_overlay_debug_category,
                             "inferenceoverlay", 0, "debug category for inference_overlay element"));

static void
gst_inference_overlay_class_init (GstInferenceOverlayClass *klass) {
  GstInferenceBaseOverlayClass *
  io_class = GST_INFERENCE_BASE_OVERLAY_CLASS (klass);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
                                         "inferenceoverlay", "Filter",
                                         "Overlays Inferece metadata on input buffer",
                                         "Carlos Rodriguez <carlos.rodriguez@ridgerun.com> \n\t\t\t"
                                         "   Jose Jimenez <jose.jimenez@ridgerun.com> \n\t\t\t"
                                         "   Michael Gruner <michael.gruner@ridgerun.com> \n\t\t\t"
                                         "   Carlos Aguero <carlos.aguero@ridgerun.com> \n\t\t\t"
                                         "   Miguel Taylor <miguel.taylor@ridgerun.com> \n\t\t\t"
                                         "   Greivin Fallas <greivin.fallas@ridgerun.com>");

  io_class->process_meta =
    GST_DEBUG_FUNCPTR (gst_inference_overlay_process_meta);
  io_class->meta_type = GST_INFERENCE_META_API_TYPE;
}

static void
gst_inference_overlay_init (GstInferenceOverlay *inference_overlay) {
}

static
void
gst_get_meta (Prediction *pred, cv::Mat cv_mat, gdouble font_scale,
              gint thickness,
              gchar **labels_list, gint num_labels) {
  gint i;
  cv::Size size;
  cv::String label;
  cv::String prob;
  BBox box;
  for (i = 0; i < pred->num_predictions ; ++i) {
    Prediction   *predict = &pred->predictions[i];
    gst_get_meta (predict, cv_mat, font_scale, thickness,
                  labels_list,  num_labels);
  }
  if (NULL != pred->box) {
    box = *pred->box;
    if (num_labels > box.label) {
      label = cv::format ("%s  Prob: %f", labels_list[box.label], box.prob);
    } else {
      label = cv::format ("Label #%d  Prob: %f", box.label, box.prob);
    }
    cv::putText (cv_mat, label, cv::Point (box.x, box.y - 5),
                 cv::FONT_HERSHEY_PLAIN, font_scale, colors[box.label % N_C], thickness);
    cv::rectangle (cv_mat, cv::Point (box.x, box.y),
                   cv::Point (box.x + box.width, box.y + box.height),
                   colors[box.label % N_C], thickness);

  }
}

static
GstFlowReturn
gst_inference_overlay_process_meta (GstInferenceBaseOverlay *inference_overlay,
                                    GstVideoFrame *frame, GstMeta *meta, gdouble font_scale, gint thickness,
                                    gchar **labels_list, gint num_labels) {
  GstInferenceMeta *
  detect_meta;
  gint  width, height, channels;
  cv::Mat cv_mat;

  switch (GST_VIDEO_FRAME_FORMAT (frame)) {
    case GST_VIDEO_FORMAT_RGB:
    case GST_VIDEO_FORMAT_BGR:
      channels = 3;
      break;
    default:
      channels = 4;
      break;
  }
  width = GST_VIDEO_FRAME_COMP_STRIDE (frame, 0) / channels;
  height = GST_VIDEO_FRAME_HEIGHT (frame);

  detect_meta = (GstInferenceMeta *) meta;

  cv_mat = cv::Mat (height, width, CV_MAKETYPE (CV_8U, channels),
                    (char *) frame->data[0]);
  gst_get_meta (detect_meta->prediction, cv_mat, font_scale, thickness,
                labels_list,  num_labels);

  return GST_FLOW_OK;
}
