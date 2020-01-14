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
                                    gchar **labels_list, gint num_labels, gint style);
void draw_line(cv::Mat *img, cv::Point pt1, cv::Point pt2, cv::Scalar color,
               int thickness, int style, int gap);

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
                                         "Lenin Torres <lenin.torres@ridgerun.com>");

  io_class->process_meta =
    GST_DEBUG_FUNCPTR (gst_inference_overlay_process_meta);
  io_class->meta_type = GST_INFERENCE_META_API_TYPE;
}

static void
gst_inference_overlay_init (GstInferenceOverlay *inference_overlay) {
}

void draw_line(cv::Mat *img, cv::Point pt1, cv::Point pt2, cv::Scalar color,
               int thickness, int style, int gap) {

  g_return_if_fail (img != NULL);

  float dx = pt1.x - pt2.x;
  float dy = pt1.y - pt2.y;

  float dist = sqrt(dx * dx + dy * dy);

  std::vector<cv::Point> pts;
  for (int i = 0; i < dist; i += gap) {
    float r = (float)i / dist;
    int x = int((pt1.x * (1.0 - r) + pt2.x * r) + .5);
    int y = int((pt1.y * (1.0 - r) + pt2.y * r) + .5);
    cv::Point p = cv::Point(x, y);
    pts.push_back(p);
  }

  int pts_size = pts.size();

  if (1 == style) {
    for (int i = 0; i < pts_size; i++) {
      cv::circle(*img, pts[i], thickness, color, -1);
    }
  } else {
    cv::Point s = pts[0];
    cv::Point e = pts[0];

    for (int i = 0; i < pts_size; i++) {
      s = e;
      e = pts[i];

      if (1 == i % 2) {
        cv::line(*img, s, e, color, thickness);
      }
    }
  }
}

static void
gst_get_meta (GstInferencePrediction *pred, cv::Mat cv_mat, gdouble font_scale,
              gint thickness,
              gchar **labels_list, gint num_labels, gint style) {
  guint i;
  cv::Size size;
  cv::String label;
  GList *iter = NULL;
  cv::String prob;
  BoundingBox box;
  int classes = 0;
  double alpha = 0.5;
  cv::Mat alpha_overlay;

  g_return_if_fail (pred != NULL);
  g_return_if_fail (labels_list != NULL);

  for (i = 0; i < g_node_n_children(pred->predictions) ; ++i) {
    GstInferencePrediction   *predict = (GstInferencePrediction *)g_node_nth_child (
                                          pred->predictions, i)->data;
    gst_get_meta (predict, cv_mat, font_scale, thickness,
                  labels_list,  num_labels, style);
  }
  box = pred->bbox;

  for (iter = pred->classifications; iter != NULL; iter = g_list_next(iter)) {
    GstInferenceClassification *classification = (GstInferenceClassification *)
        iter->data;

    classes++;
    if (classification->num_classes > classification->class_id) {
      label = cv::format ("%s  Prob: %f", classification->labels[classification->class_id],
                          classification->class_prob);
    } else {
      label = cv::format ("Label #%d  Prob: %f", classification->class_id,
                          classification->class_prob);
    }
    cv::putText (cv_mat, label, cv::Point (box.x + box.width, box.y + classes * 30),
                 cv::FONT_HERSHEY_PLAIN, font_scale, cv::Scalar::all(0), thickness);
  }

  cv_mat.copyTo(alpha_overlay);
  cv::Size text = cv::getTextSize (label, cv::FONT_HERSHEY_PLAIN, font_scale,
                                   thickness, 0);
  cv::rectangle(alpha_overlay, cv::Rect(box.x + box.width, box.y, text.width,
                                        50 * classes), cv::Scalar(255, 255, 255), -1);
  cv::addWeighted(alpha_overlay, alpha, cv_mat, 1 - alpha, 0, cv_mat);

  if (FALSE == G_NODE_IS_ROOT(pred->predictions)) {
    if (0 == style) {
      cv::rectangle (cv_mat, cv::Point (box.x, box.y),
                     cv::Point (box.x + box.width, box.y + box.height),
                     colors[50 % N_C], thickness);
    }else {
      draw_line (&cv_mat, cv::Point (box.x, box.y),
                 cv::Point (box.x + box.width, box.y),
                 colors[50 % N_C], thickness, style, 20);
      draw_line (&cv_mat, cv::Point (box.x, box.y),
                 cv::Point (box.x, box.y + box.height),
                 colors[50 % N_C], thickness, style, 20);
      draw_line (&cv_mat, cv::Point (box.x + box.width, box.y),
                 cv::Point (box.x + box.width, box.y + box.height),
                 colors[50 % N_C], thickness, style, 20);
      draw_line (&cv_mat, cv::Point (box.x, box.y + box.height),
                 cv::Point (box.x + box.width, box.y + box.height),
                 colors[50 % N_C], thickness, style, 20);
    }
  }

}

static
GstFlowReturn
gst_inference_overlay_process_meta (GstInferenceBaseOverlay *inference_overlay,
                                    GstVideoFrame *frame, GstMeta *meta, gdouble font_scale, gint thickness,
                                    gchar **labels_list, gint num_labels, gint style) {
  GstInferenceMeta *
  detect_meta;
  gint  width, height, channels;
  cv::Mat cv_mat;

  g_return_val_if_fail (inference_overlay != NULL ,GST_FLOW_ERROR);
  g_return_val_if_fail (frame != NULL ,GST_FLOW_ERROR);
  g_return_val_if_fail (meta != NULL  ,GST_FLOW_ERROR);
  g_return_val_if_fail (labels_list != NULL  ,GST_FLOW_ERROR);

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
                labels_list,  num_labels, style);

  return GST_FLOW_OK;
}
