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

#include "gstinferencepreprocess.h"
#include <math.h>

static gboolean gst_configure_format_values (GstVideoFrame * inframe,
    gint * first_index, gint * last_index, gint * offset, gint * channels);
static void gst_apply_means_std (GstVideoFrame * inframe,
    GstVideoFrame * outframe, gint first_index, gint last_index,
    gint offset, gint channels, const gdouble mean_red,
    const gdouble mean_green, const gdouble mean_blue,
    const gdouble std_r, const gdouble std_g, const gdouble std_b,
    const gint model_channels);

static void gst_apply_gray_normalization (GstVideoFrame * inframe,
    GstVideoFrame * outframe, gdouble std, gdouble offset);

static void
gst_apply_means_std (GstVideoFrame * inframe, GstVideoFrame * outframe,
    gint first_index, gint last_index, gint offset, gint channels,
    const gdouble mean_red, const gdouble mean_green,
    const gdouble mean_blue, const gdouble std_r, const gdouble std_g,
    const gdouble std_b, const gint model_channels)
{
  gint i, j, pixel_stride, width, height;

  g_return_if_fail (inframe != NULL);
  g_return_if_fail (outframe != NULL);

  pixel_stride = GST_VIDEO_FRAME_COMP_STRIDE (inframe, 0) / channels;
  width = GST_VIDEO_FRAME_WIDTH (inframe);
  height = GST_VIDEO_FRAME_HEIGHT (inframe);

  for (i = 0; i < height; ++i) {
    for (j = 0; j < width; ++j) {
      ((gfloat *) outframe->data[0])[(i * width + j) * model_channels +
          first_index] =
          (((guchar *) inframe->data[0])[(i * pixel_stride + j) * channels +
              0 + offset] - mean_red) * std_r;
      ((gfloat *) outframe->data[0])[(i * width + j) * model_channels + 1] =
          (((guchar *) inframe->data[0])[(i * pixel_stride + j) * channels +
              1 + offset] - mean_green) * std_g;
      ((gfloat *) outframe->data[0])[(i * width + j) * model_channels +
          last_index] =
          (((guchar *) inframe->data[0])[(i * pixel_stride + j) * channels +
              2 + offset] - mean_blue) * std_b;
    }
  }
}

static gboolean
gst_configure_format_values (GstVideoFrame * inframe, gint * first_index,
    gint * last_index, gint * offset, gint * channels)
{
  g_return_val_if_fail (inframe != NULL, FALSE);
  g_return_val_if_fail (first_index != NULL, FALSE);
  g_return_val_if_fail (last_index != NULL, FALSE);
  g_return_val_if_fail (offset != NULL, FALSE);
  g_return_val_if_fail (channels != NULL, FALSE);

  *channels = GST_VIDEO_FRAME_COMP_PSTRIDE (inframe, 0);
  switch (GST_VIDEO_FRAME_FORMAT (inframe)) {
    case GST_VIDEO_FORMAT_RGB:
    case GST_VIDEO_FORMAT_RGBx:
    case GST_VIDEO_FORMAT_RGBA:
      *first_index = 0;
      *last_index = 2;
      *offset = 0;
      break;
    case GST_VIDEO_FORMAT_BGR:
    case GST_VIDEO_FORMAT_BGRx:
    case GST_VIDEO_FORMAT_BGRA:
      *first_index = 2;
      *last_index = 0;
      *offset = 0;
      break;
    case GST_VIDEO_FORMAT_xRGB:
    case GST_VIDEO_FORMAT_ARGB:
      *first_index = 0;
      *last_index = 2;
      *offset = 1;
      break;
    case GST_VIDEO_FORMAT_xBGR:
    case GST_VIDEO_FORMAT_ABGR:
      *first_index = 2;
      *last_index = 0;
      *offset = 1;
      break;
    case GST_VIDEO_FORMAT_GRAY8:
      *first_index = 0;
      *last_index = 0;
      *offset = 0;
      break;
    default:
      return FALSE;
      break;
  }
  return TRUE;
}

gboolean
gst_normalize (GstVideoFrame * inframe, GstVideoFrame * outframe, gdouble mean,
    gdouble std, gint model_channels)
{
  gint first_index = 0, last_index = 0, offset = 0, channels = 0;
  g_return_val_if_fail (inframe != NULL, FALSE);
  g_return_val_if_fail (outframe != NULL, FALSE);
  if (gst_configure_format_values (inframe, &first_index, &last_index, &offset,
          &channels) == FALSE) {
    return FALSE;
  }

  gst_apply_means_std (inframe, outframe, first_index, last_index, offset,
      channels, mean, mean, mean, std, std, std, model_channels);

  return TRUE;
}

gboolean
gst_subtract_mean (GstVideoFrame * inframe, GstVideoFrame * outframe,
    gdouble mean_red, gdouble mean_green, gdouble mean_blue,
    gint model_channels)
{
  gint first_index = 0, last_index = 0, offset = 0, channels = 0;
  const gdouble std = 1;
  g_return_val_if_fail (inframe != NULL, FALSE);
  g_return_val_if_fail (outframe != NULL, FALSE);
  if (gst_configure_format_values (inframe, &first_index, &last_index, &offset,
          &channels) == FALSE) {
    return FALSE;
  }

  gst_apply_means_std (inframe, outframe, first_index, last_index, offset,
      channels, mean_red, mean_green, mean_blue, std, std, std, model_channels);
  return TRUE;
}

gboolean
gst_pixel_to_float (GstVideoFrame * inframe, GstVideoFrame * outframe,
    gint model_channels)
{
  gint first_index = 0, last_index = 0, offset = 0, channels = 0;
  const gdouble std = 1, mean = 0;
  g_return_val_if_fail (inframe != NULL, FALSE);
  g_return_val_if_fail (outframe != NULL, FALSE);
  if (gst_configure_format_values (inframe, &first_index, &last_index, &offset,
          &channels) == FALSE) {
    return FALSE;
  }

  gst_apply_means_std (inframe, outframe, first_index, last_index, offset,
      channels, mean, mean, mean, std, std, std, model_channels);
  return TRUE;
}

gboolean
gst_normalize_gray_image (GstVideoFrame * inframe, GstVideoFrame * outframe,
    gdouble mean, gint offset, gint model_channels)
{
  gint first_index = 0, last_index = 0;
  g_return_val_if_fail (inframe != NULL, FALSE);
  g_return_val_if_fail (outframe != NULL, FALSE);
  if (gst_configure_format_values (inframe, &first_index, &last_index, &offset,
          &model_channels) == FALSE) {
    return FALSE;
  }

  gst_apply_gray_normalization (inframe, outframe, mean, offset);

  return TRUE;
}

static void
gst_apply_gray_normalization (GstVideoFrame * inframe, GstVideoFrame * outframe,
    gdouble mean, gdouble offset)
{
  gint i = 0, j = 0, pixel_stride = 0, width = 0, height = 0;
  const gdouble rcp_mean = 1. / mean;

  g_return_if_fail (inframe != NULL);
  g_return_if_fail (outframe != NULL);

  pixel_stride = GST_VIDEO_FRAME_COMP_STRIDE (inframe, 0);
  width = GST_VIDEO_FRAME_WIDTH (inframe);
  height = GST_VIDEO_FRAME_HEIGHT (inframe);

  for (i = 0; i < height; ++i) {
    for (j = 0; j < width; ++j) {
      ((gfloat *) outframe->data[0])[(i * width + j)] =
          (((guchar *) inframe->data[0])[(i * pixel_stride +
                  j)] * rcp_mean - offset);
    }
  }
}
