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

#include <gst/check/gstcheck.h>
#include "preprocess_functions_utils.c"
#include "gst/r2inference/gstinferencepreprocess.h"

GST_START_TEST (test_gst_subtract_mean_RGBA)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, offset,
      model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean_red, mean_green, mean_blue;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value_red = 150;
  frame_pixel_value_green = 135;
  frame_pixel_value_blue = 170;
  buffer_size = 32;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_RGBA;
  offset = 0;

  mean_red = 123.68;
  mean_green = 116.78;
  mean_blue = 103.94;

  expected_value_red = 26.32;
  expected_value_green = 18.22;
  expected_value_blue = 66.06;
  first_index = 0;
  last_index = 2;
  model_channels = 3;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  gst_subtract_mean (&inframe, &outframe, mean_red, mean_green, mean_blue,
      model_channels);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_subtract_mean_RGB)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, offset,
      model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean_red, mean_green, mean_blue;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value_red = 150;
  frame_pixel_value_green = 135;
  frame_pixel_value_blue = 170;
  buffer_size = 24;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_RGB;
  offset = 0;

  mean_red = 123.68;
  mean_green = 116.78;
  mean_blue = 103.94;

  expected_value_red = 26.32;
  expected_value_green = 18.22;
  expected_value_blue = 66.06;
  first_index = 0;
  last_index = 2;
  model_channels = 3;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  gst_subtract_mean (&inframe, &outframe, mean_red, mean_green, mean_blue,
      model_channels);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_subtract_mean_RGBx)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, offset,
      model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean_red, mean_green, mean_blue;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value_red = 150;
  frame_pixel_value_green = 135;
  frame_pixel_value_blue = 170;
  buffer_size = 32;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_RGBx;
  offset = 0;

  mean_red = 123.68;
  mean_green = 116.78;
  mean_blue = 103.94;

  expected_value_red = 26.32;
  expected_value_green = 18.22;
  expected_value_blue = 66.06;
  first_index = 0;
  last_index = 2;
  model_channels = 3;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  gst_subtract_mean (&inframe, &outframe, mean_red, mean_green, mean_blue,
      model_channels);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_subtract_mean_BGR)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, offset,
      model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean_red, mean_green, mean_blue;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value_red = 150;
  frame_pixel_value_green = 135;
  frame_pixel_value_blue = 170;
  buffer_size = 24;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_BGR;
  offset = 0;

  mean_red = 123.68;
  mean_green = 116.78;
  mean_blue = 103.94;

  expected_value_red = 26.32;
  expected_value_green = 18.22;
  expected_value_blue = 66.06;
  first_index = 2;
  last_index = 0;
  model_channels = 3;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  gst_subtract_mean (&inframe, &outframe, mean_red, mean_green, mean_blue,
      model_channels);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_subtract_mean_BGRx)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, offset,
      model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean_red, mean_green, mean_blue;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value_red = 150;
  frame_pixel_value_green = 135;
  frame_pixel_value_blue = 170;
  buffer_size = 32;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_BGRx;
  offset = 0;

  mean_red = 123.68;
  mean_green = 116.78;
  mean_blue = 103.94;

  expected_value_red = 26.32;
  expected_value_green = 18.22;
  expected_value_blue = 66.06;
  first_index = 2;
  last_index = 0;
  model_channels = 3;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  gst_subtract_mean (&inframe, &outframe, mean_red, mean_green, mean_blue,
      model_channels);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_subtract_mean_BGRA)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, offset,
      model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean_red, mean_green, mean_blue;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value_red = 150;
  frame_pixel_value_green = 135;
  frame_pixel_value_blue = 170;
  buffer_size = 32;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_BGRA;
  offset = 0;

  mean_red = 123.68;
  mean_green = 116.78;
  mean_blue = 103.94;

  expected_value_red = 26.32;
  expected_value_green = 18.22;
  expected_value_blue = 66.06;
  first_index = 2;
  last_index = 0;
  model_channels = 3;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  gst_subtract_mean (&inframe, &outframe, mean_red, mean_green, mean_blue,
      model_channels);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_subtract_mean_xRGB)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, offset,
      model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean_red, mean_green, mean_blue;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value_red = 150;
  frame_pixel_value_green = 135;
  frame_pixel_value_blue = 170;
  buffer_size = 32;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_xRGB;
  offset = 1;

  mean_red = 123.68;
  mean_green = 116.78;
  mean_blue = 103.94;

  expected_value_red = 26.32;
  expected_value_green = 18.22;
  expected_value_blue = 66.06;
  first_index = 0;
  last_index = 2;
  model_channels = 3;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  gst_subtract_mean (&inframe, &outframe, mean_red, mean_green, mean_blue,
      model_channels);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_subtract_mean_ARGB)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, offset,
      model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean_red, mean_green, mean_blue;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value_red = 150;
  frame_pixel_value_green = 135;
  frame_pixel_value_blue = 170;
  buffer_size = 32;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_ARGB;
  offset = 1;

  mean_red = 123.68;
  mean_green = 116.78;
  mean_blue = 103.94;

  expected_value_red = 26.32;
  expected_value_green = 18.22;
  expected_value_blue = 66.06;
  first_index = 0;
  last_index = 2;
  model_channels = 3;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  gst_subtract_mean (&inframe, &outframe, mean_red, mean_green, mean_blue,
      model_channels);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_subtract_mean_xBGR)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, offset,
      model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean_red, mean_green, mean_blue;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value_red = 150;
  frame_pixel_value_green = 135;
  frame_pixel_value_blue = 170;
  buffer_size = 32;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_xBGR;
  offset = 1;

  mean_red = 123.68;
  mean_green = 116.78;
  mean_blue = 103.94;

  expected_value_red = 26.32;
  expected_value_green = 18.22;
  expected_value_blue = 66.06;
  first_index = 2;
  last_index = 0;
  model_channels = 3;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  gst_subtract_mean (&inframe, &outframe, mean_red, mean_green, mean_blue,
      model_channels);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_subtract_mean_ABGR)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, offset,
      model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean_red, mean_green, mean_blue;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value_red = 150;
  frame_pixel_value_green = 135;
  frame_pixel_value_blue = 170;
  buffer_size = 32;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_ABGR;
  offset = 1;

  mean_red = 123.68;
  mean_green = 116.78;
  mean_blue = 103.94;

  expected_value_red = 26.32;
  expected_value_green = 18.22;
  expected_value_blue = 66.06;
  first_index = 2;
  last_index = 0;
  model_channels = 3;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  gst_subtract_mean (&inframe, &outframe, mean_red, mean_green, mean_blue,
      model_channels);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_subtract_mean_invalid_format)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, offset, model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean_red, mean_green, mean_blue;
  GstVideoFormat format;
  gboolean ret;

  frame_pixel_value_red = 150;
  frame_pixel_value_green = 135;
  frame_pixel_value_blue = 170;
  buffer_size = 32;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_AYUV;
  offset = 0;
  model_channels = 3;

  mean_red = 123.68;
  mean_green = 116.78;
  mean_blue = 103.94;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  ret =
      gst_subtract_mean (&inframe, &outframe, mean_red, mean_green, mean_blue,
      model_channels);

  fail_if (ret != FALSE);
}

GST_END_TEST;

GST_START_TEST (test_gst_subtract_mean_odd_width)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, offset,
      model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean_red, mean_green, mean_blue;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value_red = 150;
  frame_pixel_value_green = 135;
  frame_pixel_value_blue = 170;
  buffer_size = 56;
  width = 7;
  height = 2;
  format = GST_VIDEO_FORMAT_RGBA;
  offset = 0;

  mean_red = 123.68;
  mean_green = 116.78;
  mean_blue = 103.94;

  expected_value_red = 26.32;
  expected_value_green = 18.22;
  expected_value_blue = 66.06;
  first_index = 0;
  last_index = 2;
  model_channels = 3;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  gst_subtract_mean (&inframe, &outframe, mean_red, mean_green, mean_blue,
      model_channels);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_subtract_mean_odd_height)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, first_index, last_index, offset,
      model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean_red, mean_green, mean_blue;
  GstVideoFormat format;
  gfloat expected_value_red, expected_value_green, expected_value_blue;

  frame_pixel_value_red = 150;
  frame_pixel_value_green = 135;
  frame_pixel_value_blue = 170;
  buffer_size = 80;
  width = 4;
  height = 5;
  format = GST_VIDEO_FORMAT_RGBA;
  offset = 0;

  mean_red = 123.68;
  mean_green = 116.78;
  mean_blue = 103.94;

  expected_value_red = 26.32;
  expected_value_green = 18.22;
  expected_value_blue = 66.06;
  first_index = 0;
  last_index = 2;
  model_channels = 3;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  gst_subtract_mean (&inframe, &outframe, mean_red, mean_green, mean_blue,
      model_channels);

  gst_check_output_pixels (&outframe, expected_value_red, expected_value_green,
      expected_value_blue, first_index, last_index, model_channels);
}

GST_END_TEST;

GST_START_TEST (test_gst_subtract_mean_null_inframe)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, offset, model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean_red, mean_green, mean_blue;
  GstVideoFormat format;

  frame_pixel_value_red = 150;
  frame_pixel_value_green = 135;
  frame_pixel_value_blue = 170;
  buffer_size = 32;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_RGBA;
  offset = 0;
  model_channels = 3;

  mean_red = 123.68;
  mean_green = 116.78;
  mean_blue = 103.94;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  ASSERT_CRITICAL (gst_subtract_mean (NULL, &outframe, mean_red, mean_green,
          mean_blue, model_channels));
}

GST_END_TEST;

GST_START_TEST (test_gst_subtract_mean_null_outframe)
{
  GstVideoFrame inframe;
  GstVideoFrame outframe;
  gint width, height, buffer_size, offset, model_channels;
  guchar frame_pixel_value_red, frame_pixel_value_green, frame_pixel_value_blue;
  gdouble mean_red, mean_green, mean_blue;
  GstVideoFormat format;

  frame_pixel_value_red = 150;
  frame_pixel_value_green = 135;
  frame_pixel_value_blue = 170;
  buffer_size = 32;
  width = 4;
  height = 2;
  format = GST_VIDEO_FORMAT_RGBA;
  offset = 0;
  model_channels = 3;

  mean_red = 123.68;
  mean_green = 116.78;
  mean_blue = 103.94;

  gst_create_test_frames (&inframe, &outframe, frame_pixel_value_red,
      frame_pixel_value_green, frame_pixel_value_blue, buffer_size, width,
      height, offset, format);

  ASSERT_CRITICAL (gst_subtract_mean (&inframe, NULL, mean_red, mean_green,
          mean_blue, model_channels));
}

GST_END_TEST;

static Suite *
gst_subtract_mean_suite (void)
{
  Suite *suite = suite_create ("GstInference");
  TCase *tc = tcase_create ("gst_subtract_mean");

  suite_add_tcase (suite, tc);

  tcase_add_test (tc, test_gst_subtract_mean_RGBA);
  tcase_add_test (tc, test_gst_subtract_mean_RGB);
  tcase_add_test (tc, test_gst_subtract_mean_RGBx);
  tcase_add_test (tc, test_gst_subtract_mean_BGR);
  tcase_add_test (tc, test_gst_subtract_mean_BGRx);
  tcase_add_test (tc, test_gst_subtract_mean_BGRA);
  tcase_add_test (tc, test_gst_subtract_mean_xRGB);
  tcase_add_test (tc, test_gst_subtract_mean_ARGB);
  tcase_add_test (tc, test_gst_subtract_mean_xBGR);
  tcase_add_test (tc, test_gst_subtract_mean_ABGR);
  tcase_add_test (tc, test_gst_subtract_mean_invalid_format);
  tcase_add_test (tc, test_gst_subtract_mean_odd_width);
  tcase_add_test (tc, test_gst_subtract_mean_odd_height);
  tcase_add_test (tc, test_gst_subtract_mean_null_inframe);
  tcase_add_test (tc, test_gst_subtract_mean_null_outframe);

  return suite;
}

GST_CHECK_MAIN (gst_subtract_mean);
