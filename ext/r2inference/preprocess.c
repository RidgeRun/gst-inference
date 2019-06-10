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


#include "preprocess.h"

gboolean
normalize_zero_mean (GstVideoInference * vi,
    GstVideoFrame * inframe, GstVideoFrame * outframe)
{
  gint i, j, pixel_stride, width, height, channels;
  gint first_index, last_index, offset;
  const gdouble mean = 128.0;
  const gdouble std = 1 / 128.0;
  const gint model_channels = 3;

  channels = 4;
  switch (GST_VIDEO_FRAME_FORMAT (inframe)) {
    case GST_VIDEO_FORMAT_RGB:
      channels = 3;
    case GST_VIDEO_FORMAT_RGBx:
    case GST_VIDEO_FORMAT_RGBA:
      first_index = 0;
      last_index = 2;
      offset = 0;
      break;
    case GST_VIDEO_FORMAT_BGR:
      channels = 3;
    case GST_VIDEO_FORMAT_BGRx:
    case GST_VIDEO_FORMAT_BGRA:
      first_index = 2;
      last_index = 0;
      offset = 0;
      break;
    case GST_VIDEO_FORMAT_xRGB:
    case GST_VIDEO_FORMAT_ARGB:
      first_index = 0;
      last_index = 2;
      offset = 1;
      break;
    case GST_VIDEO_FORMAT_xBGR:
    case GST_VIDEO_FORMAT_ABGR:
      first_index = 2;
      last_index = 0;
      offset = 1;
      break;
    default:
      GST_ERROR_OBJECT (vi, "Invalid format");
      return FALSE;
      break;
  }
  pixel_stride = GST_VIDEO_FRAME_COMP_STRIDE (inframe, 0) / channels;
  width = GST_VIDEO_FRAME_WIDTH (inframe);
  height = GST_VIDEO_FRAME_HEIGHT (inframe);

  for (i = 0; i < height; ++i) {
    for (j = 0; j < width; ++j) {
      ((gfloat *) outframe->data[0])[(i * width + j) * model_channels +
          first_index] =
          (((guchar *) inframe->data[0])[(i * pixel_stride + j) * channels + 0 +
              offset] - mean) * std;
      ((gfloat *) outframe->data[0])[(i * width + j) * model_channels + 1] =
          (((guchar *) inframe->data[0])[(i * pixel_stride + j) * channels + 1 +
              offset] - mean) * std;
      ((gfloat *) outframe->data[0])[(i * width + j) * model_channels +
          last_index] =
          (((guchar *) inframe->data[0])[(i * pixel_stride + j) * channels + 2 +
              offset] - mean) * std;
    }
  }

  return TRUE;
}

gboolean
normalize (GstVideoInference * vi,
    GstVideoFrame * inframe, GstVideoFrame * outframe)
{
  gint i, j, pixel_stride, width, height, channels;
  gint first_index, last_index, offset;
  const gdouble mean = 0;
  const gdouble std = 1 / 255.0;
  const gint model_channels = 3;

  channels = 4;
  switch (GST_VIDEO_FRAME_FORMAT (inframe)) {
    case GST_VIDEO_FORMAT_RGB:
      channels = 3;
    case GST_VIDEO_FORMAT_RGBx:
    case GST_VIDEO_FORMAT_RGBA:
      first_index = 2;
      last_index = 0;
      offset = 0;
      break;
    case GST_VIDEO_FORMAT_BGR:
      channels = 3;
    case GST_VIDEO_FORMAT_BGRx:
    case GST_VIDEO_FORMAT_BGRA:
      first_index = 0;
      last_index = 2;
      offset = 0;
      break;
    case GST_VIDEO_FORMAT_xRGB:
    case GST_VIDEO_FORMAT_ARGB:
      first_index = 2;
      last_index = 0;
      offset = 1;
      break;
    case GST_VIDEO_FORMAT_xBGR:
    case GST_VIDEO_FORMAT_ABGR:
      first_index = 0;
      last_index = 2;
      offset = 1;
      break;
    default:
      GST_ERROR_OBJECT (vi, "Invalid format");
      return FALSE;
      break;
  }
  pixel_stride = GST_VIDEO_FRAME_COMP_STRIDE (inframe, 0) / channels;
  width = GST_VIDEO_FRAME_WIDTH (inframe);
  height = GST_VIDEO_FRAME_HEIGHT (inframe);

  for (i = 0; i < height; ++i) {
    for (j = 0; j < width; ++j) {
      ((gfloat *) outframe->data[0])[(i * width + j) * model_channels +
          first_index] =
          (((guchar *) inframe->data[0])[(i * pixel_stride + j) * channels + 0 +
              offset] - mean) * std;
      ((gfloat *) outframe->data[0])[(i * width + j) * model_channels + 1] =
          (((guchar *) inframe->data[0])[(i * pixel_stride + j) * channels + 1 +
              offset] - mean) * std;
      ((gfloat *) outframe->data[0])[(i * width + j) * model_channels +
          last_index] =
          (((guchar *) inframe->data[0])[(i * pixel_stride + j) * channels + 2 +
              offset] - mean) * std;
    }
  }

  return TRUE;
}

gboolean
normalize_face (GstVideoInference * vi,
    GstVideoFrame * inframe, GstVideoFrame * outframe)
{
  gint i, j, pixel_stride, width, height, channels;
  gfloat mean, std, variance, sum, normalized, R, G, B;

  channels = GST_VIDEO_FRAME_N_COMPONENTS (inframe);
  pixel_stride = GST_VIDEO_FRAME_COMP_STRIDE (inframe, 0) / channels;
  width = GST_VIDEO_FRAME_WIDTH (inframe);
  height = GST_VIDEO_FRAME_HEIGHT (inframe);

  sum = 0;
  normalized = 0;

  for (i = 0; i < height; ++i) {
    for (j = 0; j < width; ++j) {
      sum =
          sum + (((guchar *) inframe->data[0])[(i * pixel_stride +
                  j) * channels + 0]);
      sum =
          sum + (((guchar *) inframe->data[0])[(i * pixel_stride +
                  j) * channels + 1]);
      sum =
          sum + (((guchar *) inframe->data[0])[(i * pixel_stride +
                  j) * channels + 2]);
    }
  }

  mean = sum / (float) (width * height * channels);

  for (i = 0; i < height; ++i) {
    for (j = 0; j < width; ++j) {
      R = (gfloat) ((((guchar *) inframe->data[0])[(i * pixel_stride +
                      j) * channels + 0])) - mean;
      G = (gfloat) ((((guchar *) inframe->data[0])[(i * pixel_stride +
                      j) * channels + 1])) - mean;
      B = (gfloat) ((((guchar *) inframe->data[0])[(i * pixel_stride +
                      j) * channels + 2])) - mean;
      normalized = normalized + pow (R, 2);
      normalized = normalized + pow (G, 2);
      normalized = normalized + pow (B, 2);
    }
  }

  variance = normalized / (float) (width * height * channels);
  std = sqrt (variance);

  for (i = 0; i < height; ++i) {
    for (j = 0; j < width; ++j) {
      ((gfloat *) outframe->data[0])[(i * width + j) * channels + 0] =
          (((guchar *) inframe->data[0])[(i * pixel_stride + j) * channels +
              0] - mean) / std;
      ((gfloat *) outframe->data[0])[(i * width + j) * channels + 1] =
          (((guchar *) inframe->data[0])[(i * pixel_stride + j) * channels +
              1] - mean) / std;
      ((gfloat *) outframe->data[0])[(i * width + j) * channels + 2] =
          (((guchar *) inframe->data[0])[(i * pixel_stride + j) * channels +
              2] - mean) / std;
    }
  }

  return TRUE;
}


gboolean
subtract_mean (GstVideoInference * vi,
    GstVideoFrame * inframe, GstVideoFrame * outframe, gdouble mean_red,
    gdouble mean_green, gdouble mean_blue)
{
  gint i, j, pixel_stride, width, height, channels;
  gint first_index, last_index, offset;
  const gint model_channels = 3;

  channels = inframe->info.finfo->n_components;
  switch (GST_VIDEO_FRAME_FORMAT (inframe)) {
    case GST_VIDEO_FORMAT_RGB:
    case GST_VIDEO_FORMAT_RGBx:
    case GST_VIDEO_FORMAT_RGBA:
      first_index = 0;
      last_index = 2;
      offset = 0;
      break;
    case GST_VIDEO_FORMAT_BGR:
    case GST_VIDEO_FORMAT_BGRx:
    case GST_VIDEO_FORMAT_BGRA:
      first_index = 2;
      last_index = 0;
      offset = 0;
      break;
    case GST_VIDEO_FORMAT_xRGB:
    case GST_VIDEO_FORMAT_ARGB:
      first_index = 0;
      last_index = 2;
      offset = 1;
      break;
    case GST_VIDEO_FORMAT_xBGR:
    case GST_VIDEO_FORMAT_ABGR:
      first_index = 2;
      last_index = 0;
      offset = 1;
      break;
    default:
      GST_ERROR_OBJECT (vi, "Invalid format");
      return FALSE;
      break;
  }
  pixel_stride = GST_VIDEO_FRAME_COMP_STRIDE (inframe, 0) / channels;
  width = GST_VIDEO_FRAME_WIDTH (inframe);
  height = GST_VIDEO_FRAME_HEIGHT (inframe);

  for (i = 0; i < height; ++i) {
    for (j = 0; j < width; ++j) {
      ((gfloat *) outframe->data[0])[(i * width + j) * model_channels +
          first_index] =
          (((guchar *) inframe->data[0])[(i * pixel_stride + j) * channels + 0 +
              offset] - mean_red);
      ((gfloat *) outframe->data[0])[(i * width + j) * model_channels + 1] =
          (((guchar *) inframe->data[0])[(i * pixel_stride + j) * channels + 1 +
              offset] - mean_green);
      ((gfloat *) outframe->data[0])[(i * width + j) * model_channels +
          last_index] =
          (((guchar *) inframe->data[0])[(i * pixel_stride + j) * channels + 2 +
              offset] - mean_blue);
    }
  }

  return TRUE;
}


gboolean
pixel_to_float (GstVideoInference * vi,
    GstVideoFrame * inframe, GstVideoFrame * outframe)
{

  gint i, j, pixel_stride, width, height, channels;
  gint first_index, last_index, offset;
  const gint model_channels = 3;

  channels = 4;
  switch (GST_VIDEO_FRAME_FORMAT (inframe)) {
    case GST_VIDEO_FORMAT_RGB:
      channels = 3;
    case GST_VIDEO_FORMAT_RGBx:
    case GST_VIDEO_FORMAT_RGBA:
      first_index = 2;
      last_index = 0;
      offset = 0;
      break;
    case GST_VIDEO_FORMAT_BGR:
      channels = 3;
    case GST_VIDEO_FORMAT_BGRx:
    case GST_VIDEO_FORMAT_BGRA:
      first_index = 0;
      last_index = 2;
      offset = 0;
      break;
    case GST_VIDEO_FORMAT_xRGB:
    case GST_VIDEO_FORMAT_ARGB:
      first_index = 2;
      last_index = 0;
      offset = 1;
      break;
    case GST_VIDEO_FORMAT_xBGR:
    case GST_VIDEO_FORMAT_ABGR:
      first_index = 0;
      last_index = 2;
      offset = 1;
      break;
    default:
      GST_ERROR_OBJECT (vi, "Invalid format");
      return FALSE;
      break;
  }
  pixel_stride = GST_VIDEO_FRAME_COMP_STRIDE (inframe, 0) / channels;
  width = GST_VIDEO_FRAME_WIDTH (inframe);
  height = GST_VIDEO_FRAME_HEIGHT (inframe);

  for (i = 0; i < height; ++i) {
    for (j = 0; j < width; ++j) {
      ((gfloat *) outframe->data[0])[(i * width + j) * model_channels +
          first_index] =
          (((guchar *) inframe->data[0])[(i * pixel_stride + j) * channels + 2 +
              offset]);
      ((gfloat *) outframe->data[0])[(i * width + j) * model_channels + 1] =
          (((guchar *) inframe->data[0])[(i * pixel_stride + j) * channels + 1 +
              offset]);
      ((gfloat *) outframe->data[0])[(i * width + j) * model_channels +
          last_index] =
          (((guchar *) inframe->data[0])[(i * pixel_stride + j) * channels + 0 +
              offset]);
    }
  }

  return TRUE;
}
