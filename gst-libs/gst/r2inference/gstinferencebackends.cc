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
#include "gstchildinspector.h"
#include "gstinferencebackends.h"

#include <r2i/r2i.h>
#include <unordered_map>
#include <string>

#define DEFAULT_ALIGNMENT 32

static std::unordered_map <int,GType>
backend_types ({
  {r2i::FrameworkCode::EDGETPU, GST_TYPE_EDGETPU},
  {r2i::FrameworkCode::NCSDK, GST_TYPE_NCSDK},
  {r2i::FrameworkCode::TENSORFLOW, GST_TYPE_TENSORFLOW},
  {r2i::FrameworkCode::TFLITE, GST_TYPE_TFLITE},
  {r2i::FrameworkCode::TENSORRT, GST_TYPE_TENSORRT},
  {r2i::FrameworkCode::MAX_FRAMEWORK, G_TYPE_INVALID}
});

const gchar *gst_inference_backends_search_type_name (guint id);

static void
gst_inference_backends_add_frameworkmeta (r2i::FrameworkMeta meta,
    gchar ** backends_parameters, r2i::RuntimeError error, guint alignment);

static void
gst_inference_backends_enum_register_item (const guint id,
    const gchar * desc, const gchar * shortname);

static GEnumValue *backend_enum_desc = NULL;

GType
gst_inference_backends_get_type (void)
{
  static GType backend_type = 0;
  if (!backend_type) {
    backend_type =
        g_enum_register_static ("GstInferenceBackends",
        (GEnumValue *) backend_enum_desc);
  }
  return backend_type;
}

static void
gst_inference_backends_enum_register_item (const guint id,
    const gchar * desc, const gchar * shortname)
{
  static guint backend_enum_count = 0;
  GEnumValue *backend_desc;

  backend_enum_desc =
      (GEnumValue *) g_realloc_n (backend_enum_desc, backend_enum_count + 2,
      sizeof (GEnumValue));

  backend_desc = &backend_enum_desc[backend_enum_count];
  backend_desc->value = id;
  backend_desc->value_name = g_strdup (desc);
  backend_desc->value_nick = g_strdup (shortname);

  backend_enum_count++;

  /* Sentinel */
  backend_desc = &backend_enum_desc[backend_enum_count];
  backend_desc->value = 0;
  backend_desc->value_name = NULL;
  backend_desc->value_nick = NULL;
}

const gchar *
gst_inference_backends_search_type_name (guint id)
{
  auto search = backend_types.find (id);
  if (backend_types.end () == search) {
    search = backend_types.find (r2i::FrameworkCode::MAX_FRAMEWORK);
  }
  return search->second;
}


GType
gst_inference_backends_search_type (guint id)
{
  const gchar *backend_type_name;
  GType backend_type;

  backend_type_name = gst_inference_backends_search_type_name (id);

  if (!backend_type_name) {
    backend_type = G_TYPE_INVALID;
  } else {
    backend_type = g_type_from_name (backend_type_name);
  }

  return backend_type;
}

static void
gst_inference_backends_add_frameworkmeta (r2i::FrameworkMeta meta,
    gchar ** backends_parameters, r2i::RuntimeError error, guint alignment)
{
  GstBackend *backend = NULL;
  gchar *parameters, *backend_name;
  const gchar *backend_type_name;
  GType backend_type;

  gst_inference_backends_enum_register_item (meta.code,
      meta.description.c_str (), meta.name.c_str ());

  backend_type_name = g_strdup_printf ("Gst%s", meta.name.c_str ());
  if (NULL == backend_type_name) {
    GST_ERROR_OBJECT (backend, "Failed to find Backend type: %s",
        meta.name.c_str ());
    return;
  }
  gst_inference_backend_register (backend_type_name, meta.code);
  backend_type = g_type_from_name (backend_type_name);
  backend = (GstBackend *) g_object_new (backend_type, NULL);

  backend_name =
      g_strdup_printf ("%*s: %s. Version: %s\n", alignment, meta.name.c_str (),
      meta.description.c_str (), meta.version.c_str ());

  parameters =
      gst_child_inspector_properties_to_string (G_OBJECT (backend), alignment,
      backend_name);

  if (NULL == *backends_parameters)
    *backends_parameters = parameters;
  else
    *backends_parameters =
        g_strconcat (*backends_parameters, "\n", parameters, NULL);

  g_object_unref (backend);
}

gchar *
gst_inference_backends_get_string_properties (void)
{
  gchar * backends_parameters = NULL;
  r2i::RuntimeError error;

  for (auto & meta:r2i::IFrameworkFactory::List (error)) {
    gst_inference_backends_add_frameworkmeta (meta, &backends_parameters, error,
        DEFAULT_ALIGNMENT);
  }

  return backends_parameters;
}

guint16
gst_inference_backends_get_default_backend (void)
{
  std::vector<r2i::FrameworkMeta> backends;
  r2i::FrameworkCode code;
  r2i::RuntimeError error;

  backends = r2i::IFrameworkFactory::List (error);
  code = backends.front().code;

  return code;
}
