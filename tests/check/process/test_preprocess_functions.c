/*
 * GStreamer
 * Copyright (C) 2019 RidgeRun
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
#include <gst/check/gstcheck.h>
#include <gst/video/video.h>
#include "gst/r2inference/gstinferencepreprocess.h"

static void gst_create_dump_frames (GstVideoFrame * inframe,
    GstVideoFrame * outframe, guchar value, gint buffer_size, gint width,
    gint height, GstVideoFormat format);
static void gst_check_output_pixels (GstVideoFrame * outframe,
    gfloat expected_value_red, gfloat expected_value_green,
    gfloat expected_value_blue, gint first_index, gint last_index,
    gint model_channels);

static void
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
      fail_if (out_value != expected_value_red);

      out_value =
          ((gfloat *) outframe->data[0])[(i * frame_width +
              j) * model_channels + 1];
      fail_if (out_value != expected_value_green);

      out_value =
          ((gfloat *) outframe->data[0])[(i * frame_width +
              j) * model_channels + last_index];
      fail_if (out_value != expected_value_blue);
    }
  }
}

static void
gst_create_dump_frames (GstVideoFrame * inframe, GstVideoFrame * outframe,
    guchar value, gint buffer_size, gint width, gint height,
    GstVideoFormat format)
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
  ret = gst_video_info_set_format (info, format, width, height);

  fail_if (ret == FALSE);
  fail_if (info == NULL);

  /* Allocate an output buffer for the pre-processed data */
  gst_allocation_params_init (&params);
  buffer =
      gst_buffer_new_allocate (NULL, buffer_size * sizeof (float), &params);
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
      ((guchar *) inframe->data[0])[(i * pixel_stride + j) * channels + 0] =
          value;
      ((guchar *) inframe->data[0])[(i * pixel_stride + j) * channels + 1] =
          value;
      ((guchar *) inframe->data[0])[(i * pixel_stride + j) * channels + 2] =
          value;
    }
  }
  gst_video_info_free (info);
  gst_video_frame_unmap (inframe);
  gst_video_frame_unmap (outframe);
}

GST_START_TEST (test_gst_pixel_to_float)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, model_channels;
  guchar frame_pixel_value;
  GstVideoFormat format;
  gfloat expected_value;

  frame_pixel_value = 2;
  buffer_size = 9;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_RGBA;

  expected_value = 2.0;
  first_index = 0;
  last_index = 2;
  model_channels = 3;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  gst_pixel_to_float (&inframe, &outframe, 3);

  gst_check_output_pixels (&outframe, expected_value, expected_value,
      expected_value, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_pixel_to_float_RGB)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, model_channels;
  guchar frame_pixel_value;
  GstVideoFormat format;
  gfloat expected_value;

  frame_pixel_value = 2;
  buffer_size = 9;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_RGB;

  expected_value = 2.0;
  first_index = 0;
  last_index = 2;
  model_channels = 3;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  gst_pixel_to_float (&inframe, &outframe, 3);

  gst_check_output_pixels (&outframe, expected_value, expected_value,
      expected_value, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_pixel_to_float_RGBx)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, model_channels;
  guchar frame_pixel_value;
  GstVideoFormat format;
  gfloat expected_value;

  frame_pixel_value = 2;
  buffer_size = 9;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_RGBx;

  expected_value = 2.0;
  first_index = 0;
  last_index = 2;
  model_channels = 3;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  gst_pixel_to_float (&inframe, &outframe, 3);

  gst_check_output_pixels (&outframe, expected_value, expected_value,
      expected_value, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_pixel_to_float_BGR)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, model_channels;
  guchar frame_pixel_value;
  GstVideoFormat format;
  gfloat expected_value;

  frame_pixel_value = 2;
  buffer_size = 9;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_BGR;

  expected_value = 2.0;
  first_index = 2;
  last_index = 0;
  model_channels = 3;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  gst_pixel_to_float (&inframe, &outframe, 3);

  gst_check_output_pixels (&outframe, expected_value, expected_value,
      expected_value, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_pixel_to_float_BGRx)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, model_channels;
  guchar frame_pixel_value;
  GstVideoFormat format;
  gfloat expected_value;

  frame_pixel_value = 2;
  buffer_size = 9;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_BGRx;

  expected_value = 2.0;
  first_index = 2;
  last_index = 0;
  model_channels = 3;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  gst_pixel_to_float (&inframe, &outframe, 3);

  gst_check_output_pixels (&outframe, expected_value, expected_value,
      expected_value, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_pixel_to_float_BGRA)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, model_channels;
  guchar frame_pixel_value;
  GstVideoFormat format;
  gfloat expected_value;

  frame_pixel_value = 2;
  buffer_size = 9;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_BGRA;

  expected_value = 2.0;
  first_index = 2;
  last_index = 0;
  model_channels = 3;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  gst_pixel_to_float (&inframe, &outframe, 3);

  gst_check_output_pixels (&outframe, expected_value, expected_value,
      expected_value, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_pixel_to_float_invalid_format)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size;
  guchar frame_pixel_value;
  GstVideoFormat format;
  gboolean ret;

  frame_pixel_value = 2;
  buffer_size = 9;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_AYUV;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  ret = gst_pixel_to_float (&inframe, &outframe, 3);

  fail_if (ret != FALSE);
}

GST_END_TEST;

GST_START_TEST (test_gst_pixel_to_float_odd_width)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, model_channels;
  guchar frame_pixel_value;
  GstVideoFormat format;
  gfloat expected_value;

  frame_pixel_value = 2;
  buffer_size = 9;
  width = 3;
  height = 2;
  format = GST_VIDEO_FORMAT_RGBA;

  expected_value = 2.0;
  first_index = 0;
  last_index = 2;
  model_channels = 3;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  gst_pixel_to_float (&inframe, &outframe, 3);

  gst_check_output_pixels (&outframe, expected_value, expected_value,
      expected_value, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_pixel_to_float_odd_height)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, model_channels;
  guchar frame_pixel_value;
  GstVideoFormat format;
  gfloat expected_value;

  frame_pixel_value = 2;
  buffer_size = 9;
  width = 3;
  height = 3;
  format = GST_VIDEO_FORMAT_RGBA;

  expected_value = 2.0;
  first_index = 0;
  last_index = 2;
  model_channels = 3;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  gst_pixel_to_float (&inframe, &outframe, 3);

  gst_check_output_pixels (&outframe, expected_value, expected_value,
      expected_value, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_pixel_to_float_null_inframe)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size;
  guchar frame_pixel_value;
  GstVideoFormat format;

  frame_pixel_value = 2;
  buffer_size = 9;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_RGBA;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  ASSERT_CRITICAL (gst_pixel_to_float (NULL, &outframe, 3));
}

GST_END_TEST;

GST_START_TEST (test_gst_pixel_to_float_null_outframe)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size;
  guchar frame_pixel_value;
  GstVideoFormat format;

  frame_pixel_value = 2;
  buffer_size = 9;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_RGBA;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  ASSERT_CRITICAL (gst_pixel_to_float (&inframe, NULL, 3));
}

GST_END_TEST;

GST_START_TEST (test_gst_subtract_mean)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, model_channels;
  guchar frame_pixel_value;
  gdouble mean_red, mean_green, mean_blue;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value = 150;
  buffer_size = 9;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_RGBA;

  mean_red = 123.68;
  mean_green = 116.78;
  mean_blue = 103.94;

  expected_value_red = 26.32;
  expected_value_green = 33.22;
  expected_value_blue = 46.06;
  first_index = 0;
  last_index = 2;
  model_channels = 3;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  gst_subtract_mean (&inframe, &outframe, mean_red, mean_green, mean_blue, 3);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_subtract_mean_RGB)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, model_channels;
  guchar frame_pixel_value;
  gdouble mean_red, mean_green, mean_blue;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value = 150;
  buffer_size = 9;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_RGB;

  mean_red = 123.68;
  mean_green = 116.78;
  mean_blue = 103.94;

  expected_value_red = 26.32;
  expected_value_green = 33.22;
  expected_value_blue = 46.06;
  first_index = 0;
  last_index = 2;
  model_channels = 3;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  gst_subtract_mean (&inframe, &outframe, mean_red, mean_green, mean_blue, 3);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_subtract_mean_RGBx)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, model_channels;
  guchar frame_pixel_value;
  gdouble mean_red, mean_green, mean_blue;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value = 150;
  buffer_size = 9;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_RGBx;

  mean_red = 123.68;
  mean_green = 116.78;
  mean_blue = 103.94;

  expected_value_red = 26.32;
  expected_value_green = 33.22;
  expected_value_blue = 46.06;
  first_index = 0;
  last_index = 2;
  model_channels = 3;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  gst_subtract_mean (&inframe, &outframe, mean_red, mean_green, mean_blue, 3);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_subtract_mean_BGR)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, model_channels;
  guchar frame_pixel_value;
  gdouble mean_red, mean_green, mean_blue;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value = 150;
  buffer_size = 9;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_BGR;

  mean_red = 123.68;
  mean_green = 116.78;
  mean_blue = 103.94;

  expected_value_red = 26.32;
  expected_value_green = 33.22;
  expected_value_blue = 46.06;
  first_index = 2;
  last_index = 0;
  model_channels = 3;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  gst_subtract_mean (&inframe, &outframe, mean_red, mean_green, mean_blue, 3);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_subtract_mean_BGRx)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, model_channels;
  guchar frame_pixel_value;
  gdouble mean_red, mean_green, mean_blue;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value = 150;
  buffer_size = 9;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_BGRx;

  mean_red = 123.68;
  mean_green = 116.78;
  mean_blue = 103.94;

  expected_value_red = 26.32;
  expected_value_green = 33.22;
  expected_value_blue = 46.06;
  first_index = 2;
  last_index = 0;
  model_channels = 3;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  gst_subtract_mean (&inframe, &outframe, mean_red, mean_green, mean_blue, 3);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_subtract_mean_BGRA)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, model_channels;
  guchar frame_pixel_value;
  gdouble mean_red, mean_green, mean_blue;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value = 150;
  buffer_size = 9;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_BGRA;

  mean_red = 123.68;
  mean_green = 116.78;
  mean_blue = 103.94;

  expected_value_red = 26.32;
  expected_value_green = 33.22;
  expected_value_blue = 46.06;
  first_index = 2;
  last_index = 0;
  model_channels = 3;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  gst_subtract_mean (&inframe, &outframe, mean_red, mean_green, mean_blue, 3);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_subtract_mean_invalid_format)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size;
  guchar frame_pixel_value;
  gdouble mean_red, mean_green, mean_blue;
  GstVideoFormat format;
  gboolean ret;

  frame_pixel_value = 150;
  buffer_size = 9;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_AYUV;

  mean_red = 123.68;
  mean_green = 116.78;
  mean_blue = 103.94;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  ret =
      gst_subtract_mean (&inframe, &outframe, mean_red, mean_green, mean_blue,
      3);

  fail_if (ret != FALSE);
}

GST_END_TEST;

GST_START_TEST (test_gst_subtract_mean_odd_width)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, model_channels;
  guchar frame_pixel_value;
  gdouble mean_red, mean_green, mean_blue;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value = 150;
  buffer_size = 14;
  width = 7;
  height = 2;
  format = GST_VIDEO_FORMAT_RGBA;

  mean_red = 123.68;
  mean_green = 116.78;
  mean_blue = 103.94;

  expected_value_red = 26.32;
  expected_value_green = 33.22;
  expected_value_blue = 46.06;
  first_index = 0;
  last_index = 2;
  model_channels = 3;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  gst_subtract_mean (&inframe, &outframe, mean_red, mean_green, mean_blue, 3);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_subtract_mean_odd_height)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, model_channels;
  guchar frame_pixel_value;
  gdouble mean_red, mean_green, mean_blue;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value = 150;
  buffer_size = 20;
  width = 4;
  height = 5;
  format = GST_VIDEO_FORMAT_RGBA;

  mean_red = 123.68;
  mean_green = 116.78;
  mean_blue = 103.94;

  expected_value_red = 26.32;
  expected_value_green = 33.22;
  expected_value_blue = 46.06;
  first_index = 0;
  last_index = 2;
  model_channels = 3;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  gst_subtract_mean (&inframe, &outframe, mean_red, mean_green, mean_blue, 3);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_subtract_mean_null_inframe)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size;
  guchar frame_pixel_value;
  gdouble mean_red, mean_green, mean_blue;
  GstVideoFormat format;

  frame_pixel_value = 150;
  buffer_size = 9;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_RGBA;

  mean_red = 123.68;
  mean_green = 116.78;
  mean_blue = 103.94;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  ASSERT_CRITICAL (gst_subtract_mean (NULL, &outframe, mean_red, mean_green,
          mean_blue, 3));
}

GST_END_TEST;

GST_START_TEST (test_gst_subtract_mean_null_outframe)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size;
  guchar frame_pixel_value;
  gdouble mean_red, mean_green, mean_blue;
  GstVideoFormat format;

  frame_pixel_value = 150;
  buffer_size = 9;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_RGBA;

  mean_red = 123.68;
  mean_green = 116.78;
  mean_blue = 103.94;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  ASSERT_CRITICAL (gst_subtract_mean (&inframe, NULL, mean_red, mean_green,
          mean_blue, 3));
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, model_channels;
  guchar frame_pixel_value;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value;

  frame_pixel_value = 200;
  buffer_size = 9;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_RGBA;

  mean = 0;
  std = 1 / 255.0;

  expected_value = 200.0 / 255.0;
  first_index = 0;
  last_index = 2;
  model_channels = 3;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  gst_normalize (&inframe, &outframe, mean, std, 3);

  gst_check_output_pixels (&outframe, expected_value, expected_value,
      expected_value, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_RGB)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, model_channels;
  guchar frame_pixel_value;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value;

  frame_pixel_value = 200;
  buffer_size = 9;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_RGB;

  mean = 0;
  std = 1 / 255.0;

  expected_value = 200.0 / 255.0;
  first_index = 0;
  last_index = 2;
  model_channels = 3;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  gst_normalize (&inframe, &outframe, mean, std, 3);

  gst_check_output_pixels (&outframe, expected_value, expected_value,
      expected_value, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_RGBx)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, model_channels;
  guchar frame_pixel_value;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value;

  frame_pixel_value = 200;
  buffer_size = 9;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_RGBx;

  mean = 0;
  std = 1 / 255.0;

  expected_value = 200.0 / 255.0;
  first_index = 0;
  last_index = 2;
  model_channels = 3;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  gst_normalize (&inframe, &outframe, mean, std, 3);

  gst_check_output_pixels (&outframe, expected_value, expected_value,
      expected_value, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_BGR)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, model_channels;
  guchar frame_pixel_value;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value;

  frame_pixel_value = 200;
  buffer_size = 9;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_BGR;

  mean = 0;
  std = 1 / 255.0;

  expected_value = 200.0 / 255.0;
  first_index = 2;
  last_index = 0;
  model_channels = 3;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  gst_normalize (&inframe, &outframe, mean, std, 3);

  gst_check_output_pixels (&outframe, expected_value, expected_value,
      expected_value, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_BGRx)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, model_channels;
  guchar frame_pixel_value;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value;

  frame_pixel_value = 200;
  buffer_size = 9;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_BGRx;

  mean = 0;
  std = 1 / 255.0;

  expected_value = 200.0 / 255.0;
  first_index = 2;
  last_index = 0;
  model_channels = 3;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  gst_normalize (&inframe, &outframe, mean, std, 3);

  gst_check_output_pixels (&outframe, expected_value, expected_value,
      expected_value, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_BGRA)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, model_channels;
  guchar frame_pixel_value;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value;

  frame_pixel_value = 200;
  buffer_size = 9;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_BGRA;

  mean = 0;
  std = 1 / 255.0;

  expected_value = 200.0 / 255.0;
  first_index = 2;
  last_index = 0;
  model_channels = 3;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  gst_normalize (&inframe, &outframe, mean, std, 3);

  gst_check_output_pixels (&outframe, expected_value, expected_value,
      expected_value, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_invalid_format)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size;
  guchar frame_pixel_value;
  gdouble mean, std;
  GstVideoFormat format;
  gboolean ret;

  frame_pixel_value = 200;
  buffer_size = 9;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_AYUV;

  mean = 0;
  std = 1 / 255.0;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  ret = gst_normalize (&inframe, &outframe, mean, std, 3);

  fail_if (ret != FALSE);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_odd_width)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, model_channels;
  guchar frame_pixel_value;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value;

  frame_pixel_value = 200;
  buffer_size = 18;
  width = 9;
  height = 2;
  format = GST_VIDEO_FORMAT_RGBA;

  mean = 0;
  std = 1 / 255.0;

  expected_value = 200.0 / 255.0;
  first_index = 0;
  last_index = 2;
  model_channels = 3;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  gst_normalize (&inframe, &outframe, mean, std, 3);

  gst_check_output_pixels (&outframe, expected_value, expected_value,
      expected_value, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_odd_height)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, model_channels;
  guchar frame_pixel_value;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value;

  frame_pixel_value = 200;
  buffer_size = 44;
  width = 4;
  height = 11;
  format = GST_VIDEO_FORMAT_RGBA;

  mean = 0;
  std = 1 / 255.0;

  expected_value = 200.0 / 255.0;
  first_index = 0;
  last_index = 2;
  model_channels = 3;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  gst_normalize (&inframe, &outframe, mean, std, 3);

  gst_check_output_pixels (&outframe, expected_value, expected_value,
      expected_value, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_null_inframe)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size;
  guchar frame_pixel_value;
  gdouble mean, std;
  GstVideoFormat format;

  frame_pixel_value = 200;
  buffer_size = 9;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_RGBA;

  mean = 0;
  std = 1 / 255.0;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  ASSERT_CRITICAL (gst_normalize (NULL, &outframe, mean, std, 3));
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_null_outframe)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size;
  guchar frame_pixel_value;
  gdouble mean, std;
  GstVideoFormat format;

  frame_pixel_value = 200;
  buffer_size = 9;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_RGBA;

  mean = 0;
  std = 1 / 255.0;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  ASSERT_CRITICAL (gst_normalize (&inframe, NULL, mean, std, 3));
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_zero_mean)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, model_channels;
  guchar frame_pixel_value;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value;

  frame_pixel_value = 192;
  buffer_size = 9;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_RGBA;

  mean = 128.0;
  std = 1 / 128.0;

  expected_value = 0.5;
  first_index = 0;
  last_index = 2;
  model_channels = 3;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  gst_normalize (&inframe, &outframe, mean, std, 3);

  gst_check_output_pixels (&outframe, expected_value, expected_value,
      expected_value, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_zero_mean_RGB)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, model_channels;
  guchar frame_pixel_value;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value;

  frame_pixel_value = 192;
  buffer_size = 9;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_RGB;

  mean = 128.0;
  std = 1 / 128.0;

  expected_value = 0.5;
  first_index = 0;
  last_index = 2;
  model_channels = 3;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  gst_normalize (&inframe, &outframe, mean, std, 3);

  gst_check_output_pixels (&outframe, expected_value, expected_value,
      expected_value, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_zero_mean_RGBx)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, model_channels;
  guchar frame_pixel_value;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value;

  frame_pixel_value = 192;
  buffer_size = 9;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_RGBx;

  mean = 128.0;
  std = 1 / 128.0;

  expected_value = 0.5;
  first_index = 0;
  last_index = 2;
  model_channels = 3;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  gst_normalize (&inframe, &outframe, mean, std, 3);

  gst_check_output_pixels (&outframe, expected_value, expected_value,
      expected_value, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_zero_mean_BGR)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, model_channels;
  guchar frame_pixel_value;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value;

  frame_pixel_value = 192;
  buffer_size = 9;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_BGR;

  mean = 128.0;
  std = 1 / 128.0;

  expected_value = 0.5;
  first_index = 2;
  last_index = 0;
  model_channels = 3;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  gst_normalize (&inframe, &outframe, mean, std, 3);

  gst_check_output_pixels (&outframe, expected_value, expected_value,
      expected_value, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_zero_mean_BGRx)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, model_channels;
  guchar frame_pixel_value;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value;

  frame_pixel_value = 192;
  buffer_size = 9;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_BGRx;

  mean = 128.0;
  std = 1 / 128.0;

  expected_value = 0.5;
  first_index = 2;
  last_index = 0;
  model_channels = 3;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  gst_normalize (&inframe, &outframe, mean, std, 3);

  gst_check_output_pixels (&outframe, expected_value, expected_value,
      expected_value, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_zero_mean_BGRA)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, model_channels;
  guchar frame_pixel_value;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value;

  frame_pixel_value = 192;
  buffer_size = 9;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_BGRA;

  mean = 128.0;
  std = 1 / 128.0;

  expected_value = 0.5;
  first_index = 2;
  last_index = 0;
  model_channels = 3;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  gst_normalize (&inframe, &outframe, mean, std, 3);

  gst_check_output_pixels (&outframe, expected_value, expected_value,
      expected_value, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_zero_mean_invalid_format)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size;
  guchar frame_pixel_value;
  gdouble mean, std;
  GstVideoFormat format;
  gboolean ret;

  frame_pixel_value = 192;
  buffer_size = 9;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_AYUV;

  mean = 128.0;
  std = 1 / 128.0;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  ret = gst_normalize (&inframe, &outframe, mean, std, 3);
  fail_if (ret != FALSE);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_zero_mean_odd_width)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, model_channels;
  guchar frame_pixel_value;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value;

  frame_pixel_value = 192;
  buffer_size = 22;
  width = 11;
  height = 2;
  format = GST_VIDEO_FORMAT_BGRA;

  mean = 128.0;
  std = 1 / 128.0;

  expected_value = 0.5;
  first_index = 2;
  last_index = 0;
  model_channels = 3;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  gst_normalize (&inframe, &outframe, mean, std, 3);

  gst_check_output_pixels (&outframe, expected_value, expected_value,
      expected_value, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_zero_mean_odd_height)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, model_channels;
  guchar frame_pixel_value;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value;

  frame_pixel_value = 192;
  buffer_size = 12;
  width = 4;
  height = 3;
  format = GST_VIDEO_FORMAT_BGRA;

  mean = 128.0;
  std = 1 / 128.0;

  expected_value = 0.5;
  first_index = 2;
  last_index = 0;
  model_channels = 3;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  gst_normalize (&inframe, &outframe, mean, std, 3);

  gst_check_output_pixels (&outframe, expected_value, expected_value,
      expected_value, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_zero_mean_null_inframe)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size;
  guchar frame_pixel_value;
  gdouble mean, std;
  GstVideoFormat format;

  frame_pixel_value = 192;
  buffer_size = 12;
  width = 4;
  height = 3;
  format = GST_VIDEO_FORMAT_BGRA;

  mean = 128.0;
  std = 1 / 128.0;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  ASSERT_CRITICAL (gst_normalize (NULL, &outframe, mean, std, 3));
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_zero_mean_null_outframe)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size;
  guchar frame_pixel_value;
  gdouble mean, std;
  GstVideoFormat format;

  frame_pixel_value = 192;
  buffer_size = 12;
  width = 4;
  height = 3;
  format = GST_VIDEO_FORMAT_BGRA;

  mean = 128.0;
  std = 1 / 128.0;

  gst_create_dump_frames (&inframe, &outframe, frame_pixel_value, buffer_size,
      width, height, format);

  ASSERT_CRITICAL (gst_normalize (&inframe, NULL, mean, std, 3));
}

GST_END_TEST;

static Suite *
gst_inference_preprocess_suite (void)
{
  Suite *suite = suite_create ("GstInference");
  TCase *tc = tcase_create ("preprocess");

  suite_add_tcase (suite, tc);

  tcase_add_test (tc, test_gst_pixel_to_float);
  tcase_add_test (tc, test_gst_pixel_to_float_RGB);
  tcase_add_test (tc, test_gst_pixel_to_float_RGBx);
  tcase_add_test (tc, test_gst_pixel_to_float_BGR);
  tcase_add_test (tc, test_gst_pixel_to_float_BGRx);
  tcase_add_test (tc, test_gst_pixel_to_float_BGRA);
  tcase_add_test (tc, test_gst_pixel_to_float_invalid_format);
  tcase_add_test (tc, test_gst_pixel_to_float_odd_width);
  tcase_add_test (tc, test_gst_pixel_to_float_odd_height);
  tcase_add_test (tc, test_gst_pixel_to_float_null_inframe);
  tcase_add_test (tc, test_gst_pixel_to_float_null_outframe);
  tcase_add_test (tc, test_gst_subtract_mean);
  tcase_add_test (tc, test_gst_subtract_mean_RGB);
  tcase_add_test (tc, test_gst_subtract_mean_RGBx);
  tcase_add_test (tc, test_gst_subtract_mean_BGR);
  tcase_add_test (tc, test_gst_subtract_mean_BGRx);
  tcase_add_test (tc, test_gst_subtract_mean_BGRA);
  tcase_add_test (tc, test_gst_subtract_mean_invalid_format);
  tcase_add_test (tc, test_gst_subtract_mean_odd_width);
  tcase_add_test (tc, test_gst_subtract_mean_odd_height);
  tcase_add_test (tc, test_gst_subtract_mean_null_inframe);
  tcase_add_test (tc, test_gst_subtract_mean_null_outframe);
  tcase_add_test (tc, test_gst_normalize);
  tcase_add_test (tc, test_gst_normalize_RGB);
  tcase_add_test (tc, test_gst_normalize_RGBx);
  tcase_add_test (tc, test_gst_normalize_BGR);
  tcase_add_test (tc, test_gst_normalize_BGRx);
  tcase_add_test (tc, test_gst_normalize_BGRA);
  tcase_add_test (tc, test_gst_normalize_invalid_format);
  tcase_add_test (tc, test_gst_normalize_odd_width);
  tcase_add_test (tc, test_gst_normalize_odd_height);
  tcase_add_test (tc, test_gst_normalize_null_inframe);
  tcase_add_test (tc, test_gst_normalize_null_outframe);
  tcase_add_test (tc, test_gst_normalize_zero_mean);
  tcase_add_test (tc, test_gst_normalize_zero_mean_RGB);
  tcase_add_test (tc, test_gst_normalize_zero_mean_RGBx);
  tcase_add_test (tc, test_gst_normalize_zero_mean_BGR);
  tcase_add_test (tc, test_gst_normalize_zero_mean_BGRx);
  tcase_add_test (tc, test_gst_normalize_zero_mean_BGRA);
  tcase_add_test (tc, test_gst_normalize_zero_mean_invalid_format);
  tcase_add_test (tc, test_gst_normalize_zero_mean_odd_width);
  tcase_add_test (tc, test_gst_normalize_zero_mean_odd_height);
  tcase_add_test (tc, test_gst_normalize_zero_mean_null_inframe);
  tcase_add_test (tc, test_gst_normalize_zero_mean_null_outframe);

  return suite;
}

GST_CHECK_MAIN (gst_inference_preprocess);
