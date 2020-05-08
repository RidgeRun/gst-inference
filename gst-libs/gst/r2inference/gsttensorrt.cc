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

#include "gsttensorrt.h"

#include <r2i/r2i.h>

GST_DEBUG_CATEGORY_STATIC (gst_tensorrt_debug_category);
#define GST_CAT_DEFAULT gst_tensorrt_debug_category

struct _GstTensorrt
{
  GstBackend parent;
};

G_DEFINE_TYPE_WITH_CODE (GstTensorrt, gst_tensorrt, GST_TYPE_BACKEND,
    GST_DEBUG_CATEGORY_INIT (gst_tensorrt_debug_category, "tensorrt", 0,
        "debug category for tensorrt parameters"));

static void
gst_tensorrt_class_init (GstTensorrtClass * klass)
{
  GstBackendClass *bclass = GST_BACKEND_CLASS (klass);
  GObjectClass *oclass = G_OBJECT_CLASS (klass);

  oclass->set_property = gst_backend_set_property;
  oclass->get_property = gst_backend_get_property;
  gst_backend_install_properties (bclass, r2i::FrameworkCode::TENSORRT);
}

static void
gst_tensorrt_init (GstTensorrt * self)
{
  gst_backend_set_framework_code (GST_BACKEND (self),
      r2i::FrameworkCode::TENSORRT);
}
