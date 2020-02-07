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

#ifndef __PREPROCESS_FUNCTIONS_UTILS_H__
#define __PREPROCESS_FUNCTIONS_UTILS_H__

#include <gst/video/video.h>

G_BEGIN_DECLS

void gst_create_test_frames (GstVideoFrame * inframe,
    GstVideoFrame * outframe, guchar value_red, guchar value_green, guchar value_blue, gint buffer_size, gint width,
    gint height, gint offset, GstVideoFormat format);

void gst_check_output_pixels (GstVideoFrame * outframe,
    gfloat expected_value_red, gfloat expected_value_green,
    gfloat expected_value_blue, gint first_index, gint last_index,
    gint model_channels);

G_END_DECLS

#endif
