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

static const cv::Scalar colors[] = {
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

GST_DEBUG_CATEGORY_STATIC (gst_detection_overlay_debug_category);
#define GST_CAT_DEFAULT gst_detection_overlay_debug_category

/* prototypes */
static GstFlowReturn gst_detection_overlay_process_meta
    (GstInferenceBaseOverlay * inference_overlay, cv::Mat &cv_mat,
    GstVideoFrame * frame, GstMeta * meta, gdouble font_scale, gint thickness,
    gchar ** labels_list, gint num_labels, LineStyleBoundingBox style);

enum
{
  PROP_0
};

struct _GstDetectionOverlay
{
  GstInferenceBaseOverlay parent;
};

struct _GstDetectionOverlayClass
{
  GstInferenceBaseOverlay parent;
};

/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstDetectionOverlay, gst_detection_overlay,
    GST_TYPE_INFERENCE_BASE_OVERLAY,
    GST_DEBUG_CATEGORY_INIT (gst_detection_overlay_debug_category,
        "detectionoverlay", 0, "debug category for detection_overlay element"));

static void
gst_detection_overlay_class_init (GstDetectionOverlayClass * klass)
{
  GstInferenceBaseOverlayClass *io_class = GST_INFERENCE_BASE_OVERLAY_CLASS (klass);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "detectionoverlay", "Filter",
      "Overlays detection metadata on input buffer",
      "Carlos Rodriguez <carlos.rodriguez@ridgerun.com> \n\t\t\t"
      "   Jose Jimenez <jose.jimenez@ridgerun.com> \n\t\t\t"
      "   Michael Gruner <michael.gruner@ridgerun.com> \n\t\t\t"
      "   Carlos Aguero <carlos.aguero@ridgerun.com> \n\t\t\t"
      "   Miguel Taylor <miguel.taylor@ridgerun.com> \n\t\t\t"
      "   Greivin Fallas <greivin.fallas@ridgerun.com>");

  io_class->process_meta =
      GST_DEBUG_FUNCPTR (gst_detection_overlay_process_meta);
  io_class->meta_type = GST_DETECTION_META_API_TYPE;
}

static void
gst_detection_overlay_init (GstDetectionOverlay * detection_overlay)
{
}

static GstFlowReturn
gst_detection_overlay_process_meta (GstInferenceBaseOverlay * inference_overlay,
    cv::Mat &cv_mat, GstVideoFrame * frame, GstMeta * meta, gdouble font_scale,
    gint thickness, gchar ** labels_list, gint num_labels, LineStyleBoundingBox style)
{
  GstDetectionMeta *detect_meta;
  gint i;
  cv::Size size;
  cv::String str;
  BBox box;

  detect_meta = (GstDetectionMeta *) meta;
  for (i = 0; i < detect_meta->num_boxes; ++i) {
    box = detect_meta->boxes[i];
    if (num_labels > box.label) {
      str = labels_list[box.label];
    } else {
      str = cv::format ("Label #%d", box.label);
    }

    /* Put string on screen */
    cv::putText (cv_mat, str, cv::Point (box.x, box.y - 5),
        cv::FONT_HERSHEY_PLAIN, font_scale, colors[box.label % N_C], thickness);
    cv::rectangle (cv_mat, cv::Point (box.x, box.y),
        cv::Point (box.x + box.width, box.y + box.height),
        colors[box.label % N_C], thickness);
  }

  return GST_FLOW_OK;
}
