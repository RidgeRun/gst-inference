/*
 * GStreamer
 * Copyright (C) 2018 RidgeRun
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

#ifndef _GST_INCEPTIONV1_H_
#define _GST_INCEPTIONV1_H_

#include <gst/r2inference/gstvideoinference.h>

G_BEGIN_DECLS

#define GST_TYPE_INCEPTIONV1 gst_inceptionv1_get_type ()
G_DECLARE_FINAL_TYPE (GstInceptionv1, gst_inceptionv1, GST, INCEPTIONV1, GstVideoInference)

G_END_DECLS

#endif
