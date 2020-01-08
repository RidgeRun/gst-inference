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

#ifndef __GST_VIDEO_INFERENCE_H__
#define __GST_VIDEO_INFERENCE_H__

#include <gst/gst.h>
#include <gst/video/video.h>

G_BEGIN_DECLS
#define GST_TYPE_VIDEO_INFERENCE gst_video_inference_get_type ()
G_DECLARE_DERIVABLE_TYPE (GstVideoInference, gst_video_inference, GST,
    VIDEO_INFERENCE, GstElement);

struct _GstVideoInferenceClass
{
  GstElementClass parent_class;

    gboolean (*start) (GstVideoInference * self);
    gboolean (*stop) (GstVideoInference * self);
    gboolean (*preprocess) (GstVideoInference * self, GstVideoFrame * inframe,
      GstVideoFrame * outframe);
    gboolean (*postprocess) (GstVideoInference * self,
      const gpointer prediction, gsize size, GstMeta * meta_model[2],
      GstVideoInfo * info_model, gboolean * valid_prediction);

  const GstMetaInfo *inference_meta_info;

};

G_END_DECLS
#endif //__GST_VIDEO_INFERENCE_H__
