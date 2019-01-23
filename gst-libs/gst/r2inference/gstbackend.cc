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

#include "gstbackend.h"
#include "gstbackendsubclass.h"

#include <r2i/r2i.h>

#include <cstring>
#include <memory>
#include <list>

GST_DEBUG_CATEGORY_STATIC (gst_backend_debug_category);
#define GST_CAT_DEFAULT gst_backend_debug_category

typedef struct {
  GValue *value;
  GParamSpec *pspec;
} InferenceProperty;


typedef struct _GstBackendPrivate GstBackendPrivate;
struct _GstBackendPrivate {
  r2i::FrameworkCode code;
  std::shared_ptr < r2i::IEngine > engine;
  std::shared_ptr < r2i::ILoader > loader;
  std::shared_ptr < r2i::IModel > model;
  std::shared_ptr < r2i::IParameters > params;
  std::unique_ptr < r2i::IFrameworkFactory > factory;
  GMutex backend_mutex;
  gboolean backend_started;
  std::shared_ptr < std::list<InferenceProperty> > property_list;
};

G_DEFINE_TYPE_WITH_CODE (GstBackend, gst_backend, G_TYPE_OBJECT,
                         GST_DEBUG_CATEGORY_INIT (gst_backend_debug_category, "backend", 0,
                             "debug category for backend parameters"); G_ADD_PRIVATE (GstBackend));

#define GST_BACKEND_PRIVATE(self) \
  (GstBackendPrivate *)(gst_backend_get_instance_private (self))

static GParamSpec *gst_backend_param_to_spec (r2i::ParameterMeta *param);
static int gst_backend_param_flags (int flags);
static void gst_backend_finalize (GObject *obj);

#define GST_BACKEND_ERROR gst_backend_error_quark()

static void
gst_backend_class_init (GstBackendClass *klass) {
  GObjectClass *oclass = G_OBJECT_CLASS (klass);

  oclass->set_property = gst_backend_set_property;
  oclass->get_property = gst_backend_get_property;
  oclass->finalize = gst_backend_finalize;

}

static void
gst_backend_init (GstBackend *self) {
  GstBackendPrivate *priv = GST_BACKEND_PRIVATE (self);
  g_mutex_init(&priv->backend_mutex);
  priv->backend_started = false;
  priv->property_list = std::make_shared<std::list<InferenceProperty>>();
}

static void
gst_backend_finalize (GObject *obj) {
  GstBackend *self = GST_BACKEND (obj);
  GstBackendPrivate *priv = GST_BACKEND_PRIVATE (self);
  g_mutex_clear (&priv->backend_mutex);
  G_OBJECT_CLASS (gst_backend_parent_class)->finalize (obj);
}

void
gst_backend_install_properties (GstBackendClass *klass,
                                r2i::FrameworkCode code) {
  GObjectClass *oclass = G_OBJECT_CLASS (klass);
  r2i::RuntimeError error;
  static std::vector < r2i::ParameterMeta > params;
  static gint nprop = 1;

  auto factory = r2i::IFrameworkFactory::MakeFactory (code, error);
  auto pfactory = factory->MakeParameters (error);
  error = pfactory->List (params);

  for (auto &param : params) {
    GParamSpec *spec = gst_backend_param_to_spec (&param);
    g_object_class_install_property (oclass, nprop, spec);
    nprop++;
  }
}

static int
gst_backend_param_flags (int flags) {
  int pflags = G_PARAM_STATIC_STRINGS;

  if (r2i::ParameterMeta::Flags::READ & flags) {
    pflags += G_PARAM_READABLE;
  }

  if (r2i::ParameterMeta::Flags::WRITE & flags) {
    pflags += G_PARAM_WRITABLE;
  }

  return pflags;
}

static GParamSpec *
gst_backend_param_to_spec (r2i::ParameterMeta *param) {
  GParamSpec *spec = NULL;

  switch (param->type) {
    case (r2i::ParameterMeta::Type::INTEGER): {
      spec = g_param_spec_int (param->name.c_str (),
                               param->name.c_str (),
                               param->description.c_str (),
                               G_MININT,
                               G_MAXINT, 0, (GParamFlags) gst_backend_param_flags (param->flags));
      break;
    }
    case (r2i::ParameterMeta::Type::STRING): {
      spec = g_param_spec_string (param->name.c_str (),
                                  param->name.c_str (),
                                  param->description.c_str (),
                                  NULL, (GParamFlags) gst_backend_param_flags (param->flags));
      break;
    }
    default:
      g_return_val_if_reached (NULL);
  }

  return spec;
}

void
gst_backend_set_property (GObject *object, guint property_id,
                          const GValue *value, GParamSpec *pspec) {
  GstBackend *self = GST_BACKEND (object);
  GstBackendPrivate *priv = GST_BACKEND_PRIVATE (self);
  GValue *value_copy;

  GST_DEBUG_OBJECT (self, "set_property");

  g_mutex_lock (&priv->backend_mutex);
  if (priv->backend_started) {
    switch (pspec->value_type) {
      case G_TYPE_STRING:
        priv->params->Set(pspec->name, g_value_get_string(value));
        break;
      case G_TYPE_INT:
        priv->params->Set(pspec->name, g_value_get_int(value));
        break;
      default:
        GST_WARNING_OBJECT (self, "Invalid property type");
        break;
    }
  } else {
    value_copy = (GValue *) g_malloc(sizeof(GValue));
    *value_copy = G_VALUE_INIT;
    value_copy = g_value_init (value_copy, pspec->value_type);
    g_value_copy (value, value_copy);
    g_param_spec_ref (pspec);
    priv->property_list->push_back({value_copy, pspec});
    GST_INFO_OBJECT (self, "Queueing property: %s\n", pspec->name);
  }
  g_mutex_unlock (&priv->backend_mutex);

}

void
gst_backend_get_property (GObject *object, guint property_id,
                          GValue *value, GParamSpec *pspec) {
  GstBackend *self = GST_BACKEND (object);

  GST_DEBUG_OBJECT (self, "get_property");

}

gboolean
gst_backend_start (GstBackend *self, const gchar *model_location, GError **err) {
  GstBackendPrivate *priv = GST_BACKEND_PRIVATE (self);
  r2i::RuntimeError error;
  InferenceProperty property;
  std::list<InferenceProperty>::iterator property_it;
  static std::vector<r2i::ParameterMeta> params;
  static std::vector<r2i::ParameterMeta>::iterator param_it;

  g_return_val_if_fail (priv, FALSE);
  g_return_val_if_fail (model_location, FALSE);
  g_return_val_if_fail (err, FALSE);

  priv->factory = r2i::IFrameworkFactory::MakeFactory (priv->code,
                  error);
  if (error.IsError ()) {
    goto error;
  }

  priv->engine = priv->factory->MakeEngine (error);
  if (error.IsError ()) {
    goto error;
  }

  priv->loader = priv->factory->MakeLoader (error);
  if (error.IsError ()) {
    goto error;
  }

  priv->model = priv->loader->Load (model_location, error);
  if (error.IsError ()) {
    goto error;
  }

  error = priv->engine->SetModel (priv->model);
  if (error.IsError ()) {
    goto error;
  }

  priv->params = priv->factory->MakeParameters (error);
  if (error.IsError ()) {
    goto error;
  }
  error = priv->params->Configure(priv->engine, priv->model);
  if (error.IsError ()) {
    goto error;
  }
  error = priv->params->List (params);
  if (error.IsError ()) {
    goto error;
  }

  g_mutex_lock (&priv->backend_mutex);
  for (property_it = priv->property_list->begin();
       property_it != priv->property_list->end(); ++property_it) {
    for (param_it = params.begin(); param_it != params.end(); ++param_it) {
      if (!g_strcmp0(property_it->pspec->name, param_it->name.c_str())) {
        if (r2i::ParameterMeta::Flags::WRITE_BEFORE_START & param_it->flags) {
          switch (property_it->pspec->value_type) {
            case G_TYPE_STRING:
              GST_INFO_OBJECT (self, "Setting property: %s=%s\n", property_it->pspec->name,
                               g_value_get_string(property_it->value));
              priv->params->Set(property_it->pspec->name,
                                g_value_get_string(property_it->value));
              break;
            case G_TYPE_INT:
              GST_INFO_OBJECT (self, "Setting property: %s=%d\n", property_it->pspec->name,
                               g_value_get_int(property.value));
              priv->params->Set(property_it->pspec->name,
                                g_value_get_int(property_it->value));
              break;
            default:
              GST_WARNING_OBJECT (self, "Invalid property type");
              break;
          }
          priv->property_list->erase(property_it--);
        }
        break;
      }
    }
  }

  error = priv->engine->Start ();
  if (error.IsError ()) {
    goto error;
  }

  while (!priv->property_list->empty()) {
    property = priv->property_list->front();
    switch (property.pspec->value_type) {
      case G_TYPE_STRING:
        GST_INFO_OBJECT (self, "Setting property: %s=%s\n", property.pspec->name,
                         g_value_get_string(property.value));
        priv->params->Set(property.pspec->name, g_value_get_string(property.value));
        break;
      case G_TYPE_INT:
        GST_INFO_OBJECT (self, "Setting property: %s=%d\n", property.pspec->name,
                         g_value_get_int(property.value));
        priv->params->Set(property.pspec->name, g_value_get_int(property.value));
        break;
      default:
        GST_WARNING_OBJECT (self, "Invalid property type");
        break;
    }
    g_param_spec_unref (property.pspec);
    g_free(property.value);
    priv->property_list->pop_front();
  }
  priv->backend_started = true;
  g_mutex_unlock (&priv->backend_mutex);

  return TRUE;
error:
  g_set_error (err, GST_BACKEND_ERROR, error.GetCode (),
      "R2Inference Error: (Code:%d) %s", error.GetCode (),
      error.GetDescription ().c_str ());
  return FALSE;
}

gboolean
gst_backend_process_frame (GstBackend *self, GstVideoFrame *input_frame,
                           gpointer *prediction_data, gsize *prediction_size, GError **err) {
  GstBackendPrivate *priv = GST_BACKEND_PRIVATE (self);
  std::shared_ptr < r2i::IPrediction > prediction;
  std::shared_ptr < r2i::IFrame > frame;
  r2i::RuntimeError error;

  g_return_val_if_fail (priv, FALSE);
  g_return_val_if_fail (input_frame, FALSE);
  g_return_val_if_fail (prediction_data, FALSE);
  g_return_val_if_fail (prediction_size, FALSE);
  g_return_val_if_fail (err, FALSE);

  frame = priv->factory->MakeFrame (error);
  if (error.IsError ()) {
    goto error;
  }

  GST_LOG_OBJECT (self, "Processing Frame of size %d x %d",
                  input_frame->info.width, input_frame->info.height);

  error =
    frame->Configure (input_frame->data[0], input_frame->info.width,
                      input_frame->info.height, r2i::ImageFormat::Id::RGB);
  if (error.IsError ()) {
    goto error;
  }

  prediction = priv->engine->Predict (frame, error);
  if (error.IsError ()) {
    goto error;
  }

  *prediction_size = prediction->GetResultSize ();

  /*could we avoid memory copy ?*/
  *prediction_data = g_malloc(*prediction_size);
  memcpy(*prediction_data, prediction->GetResultData(), *prediction_size);

  GST_LOG_OBJECT (self, "Size of prediction %p is %lu",
                  *prediction_data, *prediction_size);

  return TRUE;
error:
  g_set_error (err, GST_BACKEND_ERROR, error.GetCode (),
               "R2Inference Error: (Code:%d) %s", error.GetCode (),
               error.GetDescription ().c_str ());
  return FALSE;
}

gboolean
gst_backend_set_framework_code (GstBackend *backend, r2i::FrameworkCode code) {
  GstBackendPrivate *priv = GST_BACKEND_PRIVATE (backend);
  g_return_val_if_fail (priv, FALSE);

  priv->code = code;
  return TRUE;

}

guint
gst_backend_get_framework_code (GstBackend *backend) {
  GstBackendPrivate *priv = GST_BACKEND_PRIVATE (backend);
  g_return_val_if_fail (priv, -1);

  return priv->code;
}

GQuark
gst_backend_error_quark(void)
{
  static GQuark q = 0;

  if (0 == q) {
    q = g_quark_from_static_string("gst-backend-error-quark");
  }
  
  return q;
}
