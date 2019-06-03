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

#include <gst/r2inference/gstvideoinference.h>
#include "gst/r2inference/gstinferencemeta.h"
#include <string.h>
#include <math.h>

#ifndef __POSTPROCESS_H__
#define __POSTPROCESS_H__

G_BEGIN_DECLS

gboolean fill_classification_meta(GstClassificationMeta *class_meta, const gpointer prediction,
    gsize predsize);
gboolean create_boxes(GstVideoInference * vi, const gpointer prediction,
    gsize predsize, GstMeta * meta_model, GstVideoInfo * info_model,
    gboolean * valid_prediction);

G_END_DECLS

#endif