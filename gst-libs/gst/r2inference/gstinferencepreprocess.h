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

#ifndef __GST_INFERENCE_PREPROCESS_H__
#define __GST_INFERENCE_PREPROCESS_H__

#include <gst/video/video.h>

G_BEGIN_DECLS

/**
 * \brief Normalization with values between 0 and 1
 *
 * \param inframe The input frame
 * \param outframe The output frame after preprocess
 * \param mean The mean value of the channel
 * \param std  The standart deviation of the channel
 * \param model_channels The number of channels of the model
 */

gboolean gst_normalize(GstVideoFrame * inframe, GstVideoFrame * outframe, gdouble mean, gdouble std, gint model_channels);

/**
 * \brief Substract the mean value to every pixel
 *
 * \param inframe The input frame
 * \param outframe The output frame after preprocess
 * \param mean_red The mean value of the channel red
 * \param mean_green The mean value of the channel green
 * \param mean_blue The mean value of the channel blue
 * \param model_channels The number of channels of the model
 */

gboolean gst_subtract_mean(GstVideoFrame * inframe, GstVideoFrame * outframe, gdouble mean_red, gdouble mean_green, gdouble mean_blue, gint model_channels);

/**
 * \brief Change every pixel value to float
 *
 * \param inframe The input frame
 * \param outframe The output frame after preprocess
 * \param model_channels The number of channels of the model
 */

gboolean gst_pixel_to_float(GstVideoFrame * inframe, GstVideoFrame * outframe, gint model_channels);

/**
 * \brief Normalize the image to match the Rosetta input tensor requirements.
 * 
 * \param inframe The input frame
 * \param outframe The output frame after preprocess
 * \param mean The mean value of the image
 * \param offset The value that will be substracted to every pixel
 * \param model_channels The number of channels of the model
 */
gboolean
gst_normalize_gray_image (GstVideoFrame * inframe, GstVideoFrame * outframe,
    gdouble mean, gint offset, gint model_channels);
G_END_DECLS

#endif
