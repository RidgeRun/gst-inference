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

#ifndef __GST_BACKEND_H__
#define __GST_BACKEND_H__

#include <gst/gst.h>
#include <gst/video/video.h>

G_BEGIN_DECLS
#define GST_TYPE_BACKEND gst_backend_get_type ()
G_DECLARE_DERIVABLE_TYPE (GstBackend, gst_backend, GST, BACKEND, GObject);

struct _GstBackendClass
{
  GObjectClass parent_class;

  GHashTable *props;
};

GQuark gst_backend_error_quark (void);
gboolean gst_backend_start (GstBackend *, const gchar *, GError **);
guint gst_backend_get_framework_code (GstBackend *);
gboolean gst_backend_process_frame (GstBackend *, GstVideoFrame *,
                                    gpointer *, gsize *, GError **);

G_END_DECLS
#endif //__GST_BACKEND_H__
