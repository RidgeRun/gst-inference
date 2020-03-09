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

#ifndef __GST_INFERENCE_BASE_OVERLAY_H__
#define __GST_INFERENCE_BASE_OVERLAY_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/video/video.h>

#ifdef OCV_VERSION_LT_4_0
#include "opencv4/opencv2/highgui.hpp"
#include "opencv4/opencv2/imgproc.hpp"
#elif OCV_VERSION_LT_3_2
#include "opencv2/highgui/highgui.hpp"
#else
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#endif

G_BEGIN_DECLS

#define LINE_STYLE_BOUNDING_BOX (line_style_bounding_box_get_type ())
/**
 * LineStyleBoundingBox:
 * @CLASSIC : Draw lines without any style
 * @DOTTED  : Draw lines with dots
 * @DASHED  : Draw lines with dashed lines
 *
 **/
typedef enum
{
  CLASSIC,
  DOTTED,
  DASHED,
} LineStyleBoundingBox;

GType line_style_bounding_box_get_type (void) G_GNUC_CONST;
#define GST_TYPE_INFERENCE_BASE_OVERLAY gst_inference_base_overlay_get_type ()
G_DECLARE_DERIVABLE_TYPE (GstInferenceBaseOverlay, gst_inference_base_overlay, GST,
    INFERENCE_BASE_OVERLAY, GstVideoFilter);

struct _GstInferenceBaseOverlayClass
{
  GstVideoFilterClass parent_class;

  GstFlowReturn (* process_meta) (GstInferenceBaseOverlay * inference_base_overlay,
      cv::Mat &mat, GstVideoFrame * frame, GstMeta* meta, gdouble font_scale,
      gint thickness, gchar **labels_list, gint num_labels, LineStyleBoundingBox style,
      gdouble alpha_overlay);

  GType meta_type;
};

G_END_DECLS

#endif //__GST_INFERENCE_OVERLAY_H__
