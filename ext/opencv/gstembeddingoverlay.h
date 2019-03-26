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

#ifndef _GST_EMBEDDING_OVERLAY_H_
#define _GST_EMBEDDING_OVERLAY_H_

#include <gst/opencv/gstinferenceoverlay.h>

G_BEGIN_DECLS

#define GST_TYPE_EMBEDDING_OVERLAY   (gst_embedding_overlay_get_type())
G_DECLARE_FINAL_TYPE (GstEmbeddingOverlay, gst_embedding_overlay, GST, EMBEDDING_OVERLAY, GstInferenceOverlay)

G_END_DECLS

#endif
