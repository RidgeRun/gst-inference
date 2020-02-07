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

#ifndef __GST_CHILD_INSPECTOR_H__
#define __GST_CHILD_INSPECTOR_H__

#include <gst/gst.h>

G_BEGIN_DECLS

gchar * gst_child_inspector_property_to_string (GObject * object,
    GParamSpec * param, guint alignment);
gchar * gst_child_inspector_properties_to_string (GObject * object,
    guint alignment, gchar * title);

G_END_DECLS

#endif /* __GST_CHILD_INSPECTOR_H__ */
