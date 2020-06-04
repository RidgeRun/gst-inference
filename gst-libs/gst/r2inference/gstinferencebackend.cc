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

#include "gstbackend.h"
#include "gstbackendsubclass.h"

#include <glib-object.h>
#include <r2i/r2i.h>

typedef struct _GstInferenceBackend GstInferenceBackend;
struct _GstInferenceBackend
{
  GstBackend parent;
};

typedef struct _GstInferenceBackendClass GstInferenceBackendClass;
struct _GstInferenceBackendClass
{
  GstBackendClass parent_class;
    r2i::FrameworkCode code;
};

#define GST_BACKEND_CODE_QDATA g_quark_from_static_string("backend-code")

static GstBackendClass *parent_class = NULL;

static void
gst_inference_backend_class_init (GstInferenceBackendClass * klass)
{
  GstBackendClass *bclass = GST_BACKEND_CLASS (klass);
  GObjectClass *oclass = G_OBJECT_CLASS (klass);

  guint code;
  parent_class = GST_BACKEND_CLASS (g_type_class_peek_parent (klass));

  code =
      GPOINTER_TO_UINT (g_type_get_qdata (G_OBJECT_CLASS_TYPE (klass),
          GST_BACKEND_CODE_QDATA));

  klass->code = (r2i::FrameworkCode) code;
  oclass->set_property = gst_backend_set_property;
  oclass->get_property = gst_backend_get_property;
  gst_backend_install_properties (bclass, klass->code);
}

static void
gst_inference_backend_init (GstInferenceBackend * self)
{
  GstInferenceBackendClass *klass =
      (GstInferenceBackendClass *) G_OBJECT_GET_CLASS (self);
  gst_backend_set_framework_code (GST_BACKEND (self), klass->code);
}

gboolean
gst_inference_backend_register (const gchar * type_name,
    r2i::FrameworkCode code)
{
  GTypeInfo typeinfo = {
    sizeof (GstInferenceBackendClass),
    NULL,
    NULL,
    (GClassInitFunc) gst_inference_backend_class_init,
    NULL,
    NULL,
    sizeof (GstInferenceBackend),
    0,
    (GInstanceInitFunc) gst_inference_backend_init,
  };
  GType type;

  type = g_type_from_name (type_name);
  if (!type) {
    type = g_type_register_static (GST_TYPE_BACKEND,
        type_name, &typeinfo, (GTypeFlags) 0);

    g_type_set_qdata (type, GST_BACKEND_CODE_QDATA, GUINT_TO_POINTER (code));
  }

  return TRUE;
}
