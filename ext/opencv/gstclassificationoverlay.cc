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

static const cv::Scalar black = cv::Scalar (0, 0, 0);
static const cv::Scalar white = cv::Scalar (255, 255, 255);

GST_DEBUG_CATEGORY_STATIC (gst_classification_overlay_debug_category);
#define GST_CAT_DEFAULT gst_classification_overlay_debug_category

/* prototypes */
static GstFlowReturn gst_classification_overlay_process_meta (GstVideoFrame *
    frame, GstMeta * meta, gdouble font_scale, gint thickness,
    gchar ** labels_list, gint num_labels);

enum
{
  PROP_0
};

struct _GstClassificationOverlay
{
  GstInferenceOverlay parent;
};

struct _GstClassificationOverlayClass
{
  GstInferenceOverlay parent;
};

/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstClassificationOverlay, gst_classification_overlay,
    GST_TYPE_INFERENCE_OVERLAY,
    GST_DEBUG_CATEGORY_INIT (gst_classification_overlay_debug_category,
        "classificationoverlay", 0,
        "debug category for classification_overlay element"));

static void
gst_classification_overlay_class_init (GstClassificationOverlayClass * klass)
{
  GstInferenceOverlayClass *io_class = GST_INFERENCE_OVERLAY_CLASS (klass);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "classificationoverlay", "Filter",
      "Overlays classification metadata on input buffer",
      "Carlos Rodriguez <carlos.rodriguez@ridgerun.com> \n\t\t\t"
      "   Jose Jimenez <jose.jimenez@ridgerun.com> \n\t\t\t"
      "   Michael Gruner <michael.gruner@ridgerun.com> \n\t\t\t"
      "   Carlos Aguero <carlos.aguero@ridgerun.com> \n\t\t\t"
      "   Miguel Taylor <miguel.taylor@ridgerun.com> \n\t\t\t"
      "   Greivin Fallas <greivin.fallas@ridgerun.com>");

  io_class->process_meta =
      GST_DEBUG_FUNCPTR (gst_classification_overlay_process_meta);
  io_class->meta_type = GST_CLASSIFICATION_META_API_TYPE;
}

static void
gst_classification_overlay_init (GstClassificationOverlay *
    classification_overlay)
{
}

static GstFlowReturn
gst_classification_overlay_process_meta (GstVideoFrame * frame, GstMeta * meta,
    gdouble font_scale, gint thickness, gchar ** labels_list, gint num_labels)
{
  GstClassificationMeta *class_meta;
  gint index, i, width, height;
  gdouble max, current;
  cv::Mat cv_mat;
  cv::String str;
  cv::Size size;

  width =
      GST_VIDEO_FRAME_COMP_STRIDE (frame,
      0) / GST_VIDEO_FRAME_N_COMPONENTS (frame);
  height = GST_VIDEO_FRAME_HEIGHT (frame);

  class_meta = (GstClassificationMeta *) meta;

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
  if (num_labels > index) {
    str = cv::format ("%s prob:%f", labels_list[index], max);
  } else {
    str = cv::format ("Label #%d prob:%f", index, max);
  }
  /* Get size of string on screen */
  int baseline = 0;
  size =
      cv::getTextSize (str, cv::FONT_HERSHEY_TRIPLEX, thickness, font_scale,
      &baseline);
  /* Put string on screen */
  cv_mat = cv::Mat (height, width, CV_8UC3, (char *) frame->data[0]);
  cv::putText (cv_mat, str, cv::Point (0, size.height), cv::FONT_HERSHEY_PLAIN,
      font_scale, white, thickness + 1);
  cv::putText (cv_mat, str, cv::Point (0, size.height), cv::FONT_HERSHEY_PLAIN,
      font_scale, black, thickness);

  return GST_FLOW_OK;
}
