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

#ifndef __PREPROCESS_H__
#define __PREPROCESS_H__

#include <gst/r2inference/gstvideoinference.h>
#include <math.h>

G_BEGIN_DECLS

/**
 * \brief Normalization with values between 0 and 1
 *
 * \param vi Father object of every architecture
 * \param inframe The input frame
 * \param outframe The output frame after preprocess
 */

gboolean normalize(GstVideoInference * vi,
    GstVideoFrame * inframe, GstVideoFrame * outframe);

/**
 * \brief Normalization with values between -1 and 1
 *
 * \param vi Father object of every architecture
 * \param inframe The input frame
 * \param outframe The output frame after preprocess
 */

gboolean normalize_zero_mean(GstVideoInference * vi,
    GstVideoFrame * inframe, GstVideoFrame * outframe);

/**
 * \brief Especial normalization used for facenet
 *
 * \param vi Father object of every architecture
 * \param inframe The input frame
 * \param outframe The output frame after preprocess
 */

gboolean normalize_face(GstVideoInference * vi,
    GstVideoFrame * inframe, GstVideoFrame * outframe);

/**
 * \brief Substract the mean value to every pixel
 *
 * \param vi Father object of every architecture
 * \param inframe The input frame
 * \param outframe The output frame after preprocess
 */

gboolean subtract_mean(GstVideoInference * vi,
    GstVideoFrame * inframe, GstVideoFrame * outframe, gdouble mean_red, gdouble mean_green, gdouble mena_blue);

/**
 * \brief Change every pixel value to float
 *
 * \param vi Father object of every architecture
 * \param inframe The input frame
 * \param outframe The output frame after preprocess
 */

gboolean pixel_to_float(GstVideoInference * vi,
    GstVideoFrame * inframe, GstVideoFrame * outframe);

G_END_DECLS

#endif