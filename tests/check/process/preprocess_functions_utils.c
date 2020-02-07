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

#include "preprocess_functions_utils.h"

void
gst_check_output_pixels (GstVideoFrame * outframe, gfloat expected_value_red,
    gfloat expected_value_green, gfloat expected_value_blue, gint first_index,
    gint last_index, gint model_channels)
{
  gfloat out_value;
  gint frame_width;
  gint frame_height;

  frame_width = GST_VIDEO_FRAME_WIDTH (outframe);
  frame_height = GST_VIDEO_FRAME_HEIGHT (outframe);

  fail_if (outframe == NULL);
  fail_if (frame_width == 0);
  fail_if (frame_height == 0);

  for (gint i = 0; i < frame_height; ++i) {
    for (gint j = 0; j < frame_width; ++j) {
      out_value =
          ((gfloat *) outframe->data[0])[(i * frame_width +
              j) * model_channels + first_index];
      if (out_value != expected_value_red)
        GST_LOG ("Value in index: %d is different to expected value red",
            (i * frame_width + j) * model_channels + first_index);
      fail_if (out_value != expected_value_red);

      out_value =
          ((gfloat *) outframe->data[0])[(i * frame_width +
              j) * model_channels + 1];
      if (out_value != expected_value_green)
        GST_LOG ("Value in index: %d is different to expected value green",
            (i * frame_width + j) * model_channels + first_index);
      fail_if (out_value != expected_value_green);

      out_value =
          ((gfloat *) outframe->data[0])[(i * frame_width +
              j) * model_channels + last_index];
      if (out_value != expected_value_blue)
        GST_LOG ("Value in index: %d is different to expected value blue",
            (i * frame_width + j) * model_channels + first_index);
      fail_if (out_value != expected_value_blue);
    }
  }
}

void
gst_create_test_frames (GstVideoFrame * inframe, GstVideoFrame * outframe,
    guchar value_red, guchar value_green, guchar value_blue, gint buffer_size,
    gint width, gint height, gint offset, GstVideoFormat format)
{
  gboolean ret = FALSE;
  GstAllocationParams params;
  GstMapFlags flags;
  guint pixel_stride;
  guint channels;
  gint frame_width;
  gint frame_height;

  GstVideoInfo *info = gst_video_info_new ();
  GstBuffer *buffer = gst_buffer_new ();
  GstBuffer *buffer_out = gst_buffer_new ();

  fail_if (inframe == NULL);
  fail_if (outframe == NULL);

  gst_video_info_init (info);
  gst_video_info_set_format (info, format, width, height);

  fail_if (info == NULL);

  /* Allocate an output buffer for the pre-processed data */
  gst_allocation_params_init (&params);
  buffer =
      gst_buffer_new_allocate (NULL, buffer_size * sizeof (guchar), &params);
  buffer_out =
      gst_buffer_new_allocate (NULL, buffer_size * sizeof (float), &params);

  fail_if (buffer == NULL);
  fail_if (buffer_out == NULL);

  flags = (GstMapFlags) (GST_MAP_WRITE | GST_VIDEO_FRAME_MAP_FLAG_NO_REF);

  ret = gst_video_frame_map (inframe, info, buffer, flags);

  fail_if (ret == FALSE);

  ret = gst_video_frame_map (outframe, info, buffer_out, flags);

  fail_if (ret == FALSE);

  channels = GST_VIDEO_FRAME_COMP_PSTRIDE (inframe, 0);
  pixel_stride = GST_VIDEO_FRAME_COMP_STRIDE (inframe, 0) / channels;
  frame_width = GST_VIDEO_FRAME_WIDTH (inframe);
  frame_height = GST_VIDEO_FRAME_HEIGHT (inframe);

  fail_if (frame_width == 0);
  fail_if (frame_height == 0);

  for (gint i = 0; i < frame_height; ++i) {
    for (gint j = 0; j < frame_width; ++j) {
      ((guchar *) inframe->data[0])[(i * pixel_stride + j) * channels + 0 +
          offset] = value_red;
      ((guchar *) inframe->data[0])[(i * pixel_stride + j) * channels + 1 +
          offset] = value_green;
      ((guchar *) inframe->data[0])[(i * pixel_stride + j) * channels + 2 +
          offset] = value_blue;
    }
  }
  gst_video_info_free (info);
  gst_video_frame_unmap (inframe);
  gst_video_frame_unmap (outframe);
}
