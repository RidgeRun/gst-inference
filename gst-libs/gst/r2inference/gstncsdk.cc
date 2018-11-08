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

#include "gstncsdk.h"

#include <r2i/r2i.h>

GST_DEBUG_CATEGORY_STATIC (gst_ncsdk_debug_category);
#define GST_CAT_DEFAULT gst_ncsdk_debug_category

struct _GstNcsdk
{
  GstBackend parent;
};

G_DEFINE_TYPE_WITH_CODE (GstNcsdk, gst_ncsdk, GST_TYPE_BACKEND,
    GST_DEBUG_CATEGORY_INIT (gst_ncsdk_debug_category, "ncsdk", 0,
        "debug category for ncsdk parameters"));

static void
gst_ncsdk_class_init (GstNcsdkClass * klass)
{
  GstBackendClass *bclass = GST_BACKEND_CLASS (klass);

  bclass->backend = r2i::FrameworkCode::NCSDK;
}

static void
gst_ncsdk_init (GstNcsdk * self)
{

}
