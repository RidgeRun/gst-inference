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

#include "gstinferencebackends.h"
#include "gstchildinspector.h"
#include "gstncsdk.h"
#include "gsttensorflow.h"
#include "gsttflite.h"
#include "gsttensorrt.h"
#include "gstbackend.h"
#include <r2i/r2i.h>
#include <unordered_map>
#include <string>

#define DEFAULT_ALIGNMENT 32

static std::unordered_map <int,GType>
backend_types ({
  {r2i::FrameworkCode::NCSDK, GST_TYPE_NCSDK},
  {r2i::FrameworkCode::TENSORFLOW, GST_TYPE_TENSORFLOW},
  {r2i::FrameworkCode::TFLITE, GST_TYPE_TFLITE},
  {r2i::FrameworkCode::TENSORRT, GST_TYPE_TENSORRT},
  {r2i::FrameworkCode::MAX_FRAMEWORK, G_TYPE_INVALID}
});

static void
gst_inference_backends_add_frameworkmeta (r2i::FrameworkMeta meta,
    gchar ** backends_parameters, r2i::RuntimeError error,
    guint alignment);

GType
gst_inference_backends_get_type (void)
{
  static GType backend_type = 0;
  static const GEnumValue
      backend_desc[] = {
    {r2i::FrameworkCode::NCSDK, "Intel Movidius Neural Compute SDK", "ncsdk"},
    {r2i::FrameworkCode::TENSORFLOW, "TensorFlow Machine Learning Framework",
          "tensorflow"},
    {r2i::FrameworkCode::TFLITE, "Tensorflow Lite Machine Learning Framework",
          "tflite"},
    {r2i::FrameworkCode::TENSORRT, "NVIDIA's TensorRT Framework",
          "tensorrt"},
    {0, NULL, NULL}
  };
  if (!backend_type) {
    backend_type =
        g_enum_register_static ("GstInferenceBackends",
        (GEnumValue *) backend_desc);
  }
  return backend_type;
}

GType
gst_inference_backends_search_type (guint id)
{
  auto search = backend_types.find (id);
  if (backend_types.end () == search) {
    search = backend_types.find (r2i::FrameworkCode::MAX_FRAMEWORK);
  }
  return search->second;
}

static void
gst_inference_backends_add_frameworkmeta (r2i::FrameworkMeta meta,
    gchar ** backends_parameters, r2i::RuntimeError error,
    guint alignment)
{
  GstBackend * backend = NULL;
  gchar * parameters, * backend_name;
  GType backend_type;

  backend_type = gst_inference_backends_search_type (meta.code);

  if (G_TYPE_INVALID == backend_type) {
    GST_ERROR_OBJECT (backend, "Failed to find Backend type: %s",
        meta.name.c_str ());
    return;
  }

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
