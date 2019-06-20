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
#include "preprocess_functions_utils.c"
#include "gst/r2inference/gstinferencepreprocess.h"

GST_START_TEST (test_gst_normalize_RGBA)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, offset,
      model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value_red = 200;
  frame_pixel_value_green = 100;
  frame_pixel_value_blue = 150;
  buffer_size = 32;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_RGBA;
  offset = 0;

  mean = 0;
  std = 1 / 255.0;

  expected_value_red = 200.0 / 255.0;
  expected_value_green = 100.0 / 255.0;
  expected_value_blue = 150.0 / 255.0;
  first_index = 0;
  last_index = 2;
  model_channels = 3;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  gst_normalize (&inframe, &outframe, mean, std, model_channels);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_RGB)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, offset,
      model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value_red = 200;
  frame_pixel_value_green = 100;
  frame_pixel_value_blue = 150;
  buffer_size = 24;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_RGB;
  offset = 0;

  mean = 0;
  std = 1 / 255.0;

  expected_value_red = 200.0 / 255.0;
  expected_value_green = 100.0 / 255.0;
  expected_value_blue = 150.0 / 255.0;
  first_index = 0;
  last_index = 2;
  model_channels = 3;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  gst_normalize (&inframe, &outframe, mean, std, model_channels);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_RGBx)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, offset,
      model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value_red = 200;
  frame_pixel_value_green = 100;
  frame_pixel_value_blue = 150;
  buffer_size = 32;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_RGBx;
  offset = 0;

  mean = 0;
  std = 1 / 255.0;

  expected_value_red = 200.0 / 255.0;
  expected_value_green = 100.0 / 255.0;
  expected_value_blue = 150.0 / 255.0;
  first_index = 0;
  last_index = 2;
  model_channels = 3;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  gst_normalize (&inframe, &outframe, mean, std, model_channels);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_BGR)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, offset,
      model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value_red = 200;
  frame_pixel_value_green = 100;
  frame_pixel_value_blue = 150;
  buffer_size = 24;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_BGR;
  offset = 0;

  mean = 0;
  std = 1 / 255.0;

  expected_value_red = 200.0 / 255.0;
  expected_value_green = 100.0 / 255.0;
  expected_value_blue = 150.0 / 255.0;
  first_index = 2;
  last_index = 0;
  model_channels = 3;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  gst_normalize (&inframe, &outframe, mean, std, model_channels);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_BGRx)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, offset,
      model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value_red = 200;
  frame_pixel_value_green = 100;
  frame_pixel_value_blue = 150;
  buffer_size = 32;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_BGRx;
  offset = 0;

  mean = 0;
  std = 1 / 255.0;

  expected_value_red = 200.0 / 255.0;
  expected_value_green = 100.0 / 255.0;
  expected_value_blue = 150.0 / 255.0;
  first_index = 2;
  last_index = 0;
  model_channels = 3;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  gst_normalize (&inframe, &outframe, mean, std, model_channels);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_BGRA)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, offset,
      model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value_red = 200;
  frame_pixel_value_green = 100;
  frame_pixel_value_blue = 150;
  buffer_size = 32;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_BGRA;
  offset = 0;

  mean = 0;
  std = 1 / 255.0;

  expected_value_red = 200.0 / 255.0;
  expected_value_green = 100.0 / 255.0;
  expected_value_blue = 150.0 / 255.0;
  first_index = 2;
  last_index = 0;
  model_channels = 3;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  gst_normalize (&inframe, &outframe, mean, std, model_channels);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_xRGB)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, offset,
      model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value_red = 200;
  frame_pixel_value_green = 100;
  frame_pixel_value_blue = 150;
  buffer_size = 32;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_xRGB;
  offset = 1;

  mean = 0;
  std = 1 / 255.0;

  expected_value_red = 200.0 / 255.0;
  expected_value_green = 100.0 / 255.0;
  expected_value_blue = 150.0 / 255.0;
  first_index = 0;
  last_index = 2;
  model_channels = 3;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  gst_normalize (&inframe, &outframe, mean, std, model_channels);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_ARGB)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, offset,
      model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value_red = 200;
  frame_pixel_value_green = 100;
  frame_pixel_value_blue = 150;
  buffer_size = 32;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_ARGB;
  offset = 1;

  mean = 0;
  std = 1 / 255.0;

  expected_value_red = 200.0 / 255.0;
  expected_value_green = 100.0 / 255.0;
  expected_value_blue = 150.0 / 255.0;
  first_index = 0;
  last_index = 2;
  model_channels = 3;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  gst_normalize (&inframe, &outframe, mean, std, model_channels);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_xBGR)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, offset,
      model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value_red = 200;
  frame_pixel_value_green = 100;
  frame_pixel_value_blue = 150;
  buffer_size = 32;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_xBGR;
  offset = 1;

  mean = 0;
  std = 1 / 255.0;

  expected_value_red = 200.0 / 255.0;
  expected_value_green = 100.0 / 255.0;
  expected_value_blue = 150.0 / 255.0;
  first_index = 2;
  last_index = 0;
  model_channels = 3;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  gst_normalize (&inframe, &outframe, mean, std, model_channels);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_ABGR)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, offset,
      model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value_red = 200;
  frame_pixel_value_green = 100;
  frame_pixel_value_blue = 150;
  buffer_size = 32;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_ABGR;
  offset = 1;

  mean = 0;
  std = 1 / 255.0;

  expected_value_red = 200.0 / 255.0;
  expected_value_green = 100.0 / 255.0;
  expected_value_blue = 150.0 / 255.0;
  first_index = 2;
  last_index = 0;
  model_channels = 3;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  gst_normalize (&inframe, &outframe, mean, std, model_channels);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_invalid_format)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, offset, model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean, std;
  GstVideoFormat format;
  gboolean ret;

  frame_pixel_value_red = 200;
  frame_pixel_value_green = 100;
  frame_pixel_value_blue = 150;
  buffer_size = 32;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_AYUV;
  offset = 0;
  model_channels = 3;

  mean = 0;
  std = 1 / 255.0;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  ret = gst_normalize (&inframe, &outframe, mean, std, model_channels);

  fail_if (ret != FALSE);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_odd_width)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, offset,
      model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value_red = 200;
  frame_pixel_value_green = 100;
  frame_pixel_value_blue = 150;
  buffer_size = 72;
  width = 9;
  height = 2;
  format = GST_VIDEO_FORMAT_RGBA;
  offset = 0;

  mean = 0;
  std = 1 / 255.0;

  expected_value_red = 200.0 / 255.0;
  expected_value_green = 100.0 / 255.0;
  expected_value_blue = 150.0 / 255.0;
  first_index = 0;
  last_index = 2;
  model_channels = 3;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  gst_normalize (&inframe, &outframe, mean, std, model_channels);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_odd_height)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, offset,
      model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value_red = 200;
  frame_pixel_value_green = 100;
  frame_pixel_value_blue = 150;
  buffer_size = 176;
  width = 4;
  height = 11;
  format = GST_VIDEO_FORMAT_RGBA;
  offset = 0;

  mean = 0;
  std = 1 / 255.0;

  expected_value_red = 200.0 / 255.0;
  expected_value_green = 100.0 / 255.0;
  expected_value_blue = 150.0 / 255.0;
  first_index = 0;
  last_index = 2;
  model_channels = 3;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  gst_normalize (&inframe, &outframe, mean, std, model_channels);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_null_inframe)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, offset, model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean, std;
  GstVideoFormat format;

  frame_pixel_value_red = 200;
  frame_pixel_value_green = 100;
  frame_pixel_value_blue = 150;
  buffer_size = 32;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_RGBA;
  offset = 0;
  model_channels = 3;

  mean = 0;
  std = 1 / 255.0;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  ASSERT_CRITICAL (gst_normalize (NULL, &outframe, mean, std, model_channels));
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_null_outframe)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, offset, model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean, std;
  GstVideoFormat format;

  frame_pixel_value_red = 200;
  frame_pixel_value_green = 100;
  frame_pixel_value_blue = 150;
  buffer_size = 32;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_RGBA;
  offset = 0;
  model_channels = 3;

  mean = 0;
  std = 1 / 255.0;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  ASSERT_CRITICAL (gst_normalize (&inframe, NULL, mean, std, model_channels));
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_zero_mean_RGBA)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, offset,
      model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value_red = 192;
  frame_pixel_value_green = 128;
  frame_pixel_value_blue = 40;
  buffer_size = 32;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_RGBA;
  offset = 0;

  mean = 128.0;
  std = 1 / 128.0;

  expected_value_red = 0.5;
  expected_value_green = 0;
  expected_value_blue = -0.6875;
  first_index = 0;
  last_index = 2;
  model_channels = 3;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  gst_normalize (&inframe, &outframe, mean, std, model_channels);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_zero_mean_RGB)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, offset,
      model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value_red = 192;
  frame_pixel_value_green = 128;
  frame_pixel_value_blue = 40;
  buffer_size = 24;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_RGB;
  offset = 0;

  mean = 128.0;
  std = 1 / 128.0;

  expected_value_red = 0.5;
  expected_value_green = 0;
  expected_value_blue = -0.6875;
  first_index = 0;
  last_index = 2;
  model_channels = 3;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  gst_normalize (&inframe, &outframe, mean, std, model_channels);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_zero_mean_RGBx)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, offset,
      model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value_red = 192;
  frame_pixel_value_green = 128;
  frame_pixel_value_blue = 40;
  buffer_size = 32;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_RGBx;
  offset = 0;

  mean = 128.0;
  std = 1 / 128.0;

  expected_value_red = 0.5;
  expected_value_green = 0;
  expected_value_blue = -0.6875;
  first_index = 0;
  last_index = 2;
  model_channels = 3;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  gst_normalize (&inframe, &outframe, mean, std, model_channels);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_zero_mean_BGR)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, offset,
      model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value_red = 192;
  frame_pixel_value_green = 128;
  frame_pixel_value_blue = 40;
  buffer_size = 24;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_BGR;
  offset = 0;

  mean = 128.0;
  std = 1 / 128.0;

  expected_value_red = 0.5;
  expected_value_green = 0;
  expected_value_blue = -0.6875;
  first_index = 2;
  last_index = 0;
  model_channels = 3;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  gst_normalize (&inframe, &outframe, mean, std, model_channels);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_zero_mean_BGRx)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, offset,
      model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value_red = 192;
  frame_pixel_value_green = 128;
  frame_pixel_value_blue = 40;
  buffer_size = 32;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_BGRx;
  offset = 0;

  mean = 128.0;
  std = 1 / 128.0;

  expected_value_red = 0.5;
  expected_value_green = 0;
  expected_value_blue = -0.6875;
  first_index = 2;
  last_index = 0;
  model_channels = 3;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  gst_normalize (&inframe, &outframe, mean, std, model_channels);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_zero_mean_BGRA)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, offset,
      model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value_red = 192;
  frame_pixel_value_green = 128;
  frame_pixel_value_blue = 40;
  buffer_size = 32;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_BGRA;
  offset = 0;

  mean = 128.0;
  std = 1 / 128.0;

  expected_value_red = 0.5;
  expected_value_green = 0;
  expected_value_blue = -0.6875;
  first_index = 2;
  last_index = 0;
  model_channels = 3;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  gst_normalize (&inframe, &outframe, mean, std, model_channels);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_zero_mean_xRGB)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, offset,
      model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value_red = 192;
  frame_pixel_value_green = 128;
  frame_pixel_value_blue = 40;
  buffer_size = 32;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_xRGB;
  offset = 1;

  mean = 128.0;
  std = 1 / 128.0;

  expected_value_red = 0.5;
  expected_value_green = 0;
  expected_value_blue = -0.6875;
  first_index = 0;
  last_index = 2;
  model_channels = 3;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  gst_normalize (&inframe, &outframe, mean, std, model_channels);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_zero_mean_ARGB)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, offset,
      model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value_red = 192;
  frame_pixel_value_green = 128;
  frame_pixel_value_blue = 40;
  buffer_size = 32;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_ARGB;
  offset = 1;

  mean = 128.0;
  std = 1 / 128.0;

  expected_value_red = 0.5;
  expected_value_green = 0;
  expected_value_blue = -0.6875;
  first_index = 0;
  last_index = 2;
  model_channels = 3;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  gst_normalize (&inframe, &outframe, mean, std, model_channels);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_zero_mean_xBGR)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, offset,
      model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value_red = 192;
  frame_pixel_value_green = 128;
  frame_pixel_value_blue = 40;
  buffer_size = 32;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_xBGR;
  offset = 1;

  mean = 128.0;
  std = 1 / 128.0;

  expected_value_red = 0.5;
  expected_value_green = 0;
  expected_value_blue = -0.6875;
  first_index = 2;
  last_index = 0;
  model_channels = 3;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  gst_normalize (&inframe, &outframe, mean, std, model_channels);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_zero_mean_ABGR)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, offset,
      model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value_red = 192;
  frame_pixel_value_green = 128;
  frame_pixel_value_blue = 40;
  buffer_size = 32;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_ABGR;
  offset = 1;

  mean = 128.0;
  std = 1 / 128.0;

  expected_value_red = 0.5;
  expected_value_green = 0;
  expected_value_blue = -0.6875;
  first_index = 2;
  last_index = 0;
  model_channels = 3;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  gst_normalize (&inframe, &outframe, mean, std, model_channels);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_zero_mean_invalid_format)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, offset, model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean, std;
  GstVideoFormat format;
  gboolean ret;

  frame_pixel_value_red = 192;
  frame_pixel_value_green = 128;
  frame_pixel_value_blue = 40;
  buffer_size = 32;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_AYUV;
  offset = 0;
  model_channels = 3;

  mean = 128.0;
  std = 1 / 128.0;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  ret = gst_normalize (&inframe, &outframe, mean, std, model_channels);
  fail_if (ret != FALSE);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_zero_mean_odd_width)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, offset,
      model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value_red = 192;
  frame_pixel_value_green = 128;
  frame_pixel_value_blue = 40;
  buffer_size = 88;
  width = 11;
  height = 2;
  format = GST_VIDEO_FORMAT_BGRA;
  offset = 0;

  mean = 128.0;
  std = 1 / 128.0;

  expected_value_red = 0.5;
  expected_value_green = 0;
  expected_value_blue = -0.6875;
  first_index = 2;
  last_index = 0;
  model_channels = 3;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  gst_normalize (&inframe, &outframe, mean, std, model_channels);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_zero_mean_odd_height)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, offset,
      model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean, std;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value_red = 192;
  frame_pixel_value_green = 128;
  frame_pixel_value_blue = 40;
  buffer_size = 48;
  width = 4;
  height = 3;
  format = GST_VIDEO_FORMAT_BGRA;
  offset = 0;

  mean = 128.0;
  std = 1 / 128.0;

  expected_value_red = 0.5;
  expected_value_green = 0;
  expected_value_blue = -0.6875;
  first_index = 2;
  last_index = 0;
  model_channels = 3;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  gst_normalize (&inframe, &outframe, mean, std, model_channels);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_zero_mean_null_inframe)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, offset, model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean, std;
  GstVideoFormat format;

  frame_pixel_value_red = 192;
  frame_pixel_value_green = 128;
  frame_pixel_value_blue = 40;
  buffer_size = 32;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_BGRA;
  offset = 0;
  model_channels = 3;

  mean = 128.0;
  std = 1 / 128.0;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  ASSERT_CRITICAL (gst_normalize (NULL, &outframe, mean, std, model_channels));
}

GST_END_TEST;

GST_START_TEST (test_gst_normalize_zero_mean_null_outframe)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, offset, model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean, std;
  GstVideoFormat format;

  frame_pixel_value_red = 192;
  frame_pixel_value_green = 128;
  frame_pixel_value_blue = 40;
  buffer_size = 32;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_BGRA;
  offset = 0;
  model_channels = 3;

  mean = 128.0;
  std = 1 / 128.0;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  ASSERT_CRITICAL (gst_normalize (&inframe, NULL, mean, std, model_channels));
}

GST_END_TEST;

static Suite *
gst_normalize_suite (void)
{
  Suite *suite = suite_create ("GstInference");
  TCase *tc = tcase_create ("gst_normalize");

  suite_add_tcase (suite, tc);

  tcase_add_test (tc, test_gst_normalize_RGBA);
  tcase_add_test (tc, test_gst_normalize_RGB);
  tcase_add_test (tc, test_gst_normalize_RGBx);
  tcase_add_test (tc, test_gst_normalize_BGR);
  tcase_add_test (tc, test_gst_normalize_BGRx);
  tcase_add_test (tc, test_gst_normalize_BGRA);
  tcase_add_test (tc, test_gst_normalize_xRGB);
  tcase_add_test (tc, test_gst_normalize_ARGB);
  tcase_add_test (tc, test_gst_normalize_xBGR);
  tcase_add_test (tc, test_gst_normalize_ABGR);
  tcase_add_test (tc, test_gst_normalize_invalid_format);
  tcase_add_test (tc, test_gst_normalize_odd_width);
  tcase_add_test (tc, test_gst_normalize_odd_height);
  tcase_add_test (tc, test_gst_normalize_null_inframe);
  tcase_add_test (tc, test_gst_normalize_null_outframe);
  tcase_add_test (tc, test_gst_normalize_zero_mean_RGBA);
  tcase_add_test (tc, test_gst_normalize_zero_mean_RGB);
  tcase_add_test (tc, test_gst_normalize_zero_mean_RGBx);
  tcase_add_test (tc, test_gst_normalize_zero_mean_BGR);
  tcase_add_test (tc, test_gst_normalize_zero_mean_BGRx);
  tcase_add_test (tc, test_gst_normalize_zero_mean_BGRA);
  tcase_add_test (tc, test_gst_normalize_zero_mean_xRGB);
  tcase_add_test (tc, test_gst_normalize_zero_mean_ARGB);
  tcase_add_test (tc, test_gst_normalize_zero_mean_xBGR);
  tcase_add_test (tc, test_gst_normalize_zero_mean_ABGR);
  tcase_add_test (tc, test_gst_normalize_zero_mean_invalid_format);
  tcase_add_test (tc, test_gst_normalize_zero_mean_odd_width);
  tcase_add_test (tc, test_gst_normalize_zero_mean_odd_height);
  tcase_add_test (tc, test_gst_normalize_zero_mean_null_inframe);
  tcase_add_test (tc, test_gst_normalize_zero_mean_null_outframe);

  return suite;
}

GST_CHECK_MAIN (gst_normalize);
