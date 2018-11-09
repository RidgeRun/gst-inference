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

#include "gstvideoinference.h"
#include "gstinferencebackends.h"

#include <gst/base/gstcollectpads.h>
#include <memory>

#include "gstncsdk.h"

static GstStaticPadTemplate sink_bypass_factory =
GST_STATIC_PAD_TEMPLATE ("sink_bypass",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS ("ANY")
    );

static GstStaticPadTemplate src_bypass_factory =
GST_STATIC_PAD_TEMPLATE ("src_bypass",
    GST_PAD_SRC,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS ("ANY")
    );

GST_DEBUG_CATEGORY_STATIC (gst_video_inference_debug_category);
#define GST_CAT_DEFAULT gst_video_inference_debug_category

#define DEFAULT_BACKEND          0
#define DEFAULT_MODEL_LOCATION   NULL

enum
{
  PROP_0,
  PROP_BACKEND,
  PROP_MODEL_LOCATION
};


typedef struct _GstVideoInferencePrivate GstVideoInferencePrivate;
struct _GstVideoInferencePrivate
{
  GstCollectPads *cpads;
  GstCollectData *sink_bypass_data;
  GstCollectData *sink_model_data;

  GstPad *sink_bypass;
  GstPad *src_bypass;
  GstPad *sink_model;
  GstPad *src_model;

  GstBackend *backend;

  std::shared_ptr < r2i::IEngine > engine;
  std::shared_ptr < r2i::ILoader > loader;
  std::shared_ptr < r2i::IModel > model;

  gchar *model_location;
};

/* GObject methods */
static void gst_video_inference_finalize (GObject * object);
static void gst_video_inference_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_video_inference_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);

/* GstElement methods */
static GstStateChangeReturn gst_video_inference_change_state (GstElement *
    element, GstStateChange transition);
static GstPad *gst_video_inference_request_new_pad (GstElement * element,
    GstPadTemplate * templ, const gchar * name, const GstCaps * caps);
static void gst_video_inference_release_pad (GstElement * element,
    GstPad * pad);

/* GstChildProxy methods */
static void gst_video_inference_child_proxy_init (GstChildProxyInterface *
    iface);
static GObject *gst_video_inference_get_child_by_name (GstChildProxy * parent,
    const gchar * name);
static GObject *gst_video_inference_get_child_by_index (GstChildProxy * parent,
    guint index);
static guint gst_video_inference_get_children_count (GstChildProxy * parent);

/* GstVideoInference methods */
static gboolean gst_video_inference_start (GstVideoInference * self);
static gboolean gst_video_inference_stop (GstVideoInference * self);
static GstPad *gst_video_inference_create_pad (GstVideoInference * self,
    GstPadTemplate * templ, const gchar * name, GstCollectData ** data);
static GstFlowReturn gst_video_inference_collected (GstCollectPads * pads,
    gpointer user_data);
static GstFlowReturn gst_video_inference_forward_buffer (GstVideoInference *
    self, GstVideoInferencePrivate * priv, GstCollectData * data, GstPad * pad);
static gboolean gst_video_inference_sink_event (GstCollectPads * pads,
    GstCollectData * pad, GstEvent * event, gpointer user_data);
static gboolean gst_video_inference_src_event (GstPad * pad, GstObject * parent,
    GstEvent * event);
static GstPad *gst_video_inference_get_src_pad (GstVideoInference * self,
    GstVideoInferencePrivate * priv, GstPad * pad);

G_DEFINE_TYPE_WITH_CODE (GstVideoInference, gst_video_inference,
    GST_TYPE_ELEMENT,
    GST_DEBUG_CATEGORY_INIT (gst_video_inference_debug_category,
        "videoinference", 0, "debug category for videoinference base class");
    G_ADD_PRIVATE (GstVideoInference);
    G_IMPLEMENT_INTERFACE (GST_TYPE_CHILD_PROXY,
        gst_video_inference_child_proxy_init));

#define GST_VIDEO_INFERENCE_PRIVATE(self) \
  (GstVideoInferencePrivate *)(gst_video_inference_get_instance_private (self))

static void
gst_video_inference_class_init (GstVideoInferenceClass * klass)
{
  GObjectClass *oclass = G_OBJECT_CLASS (klass);
  GstElementClass *eclass = GST_ELEMENT_CLASS (klass);
  gchar *backend_blurb, *backends_params = NULL;

  oclass->finalize = gst_video_inference_finalize;
  oclass->set_property = gst_video_inference_set_property;
  oclass->get_property = gst_video_inference_get_property;

  eclass->change_state = GST_DEBUG_FUNCPTR (gst_video_inference_change_state);
  eclass->request_new_pad =
      GST_DEBUG_FUNCPTR (gst_video_inference_request_new_pad);
  eclass->release_pad = GST_DEBUG_FUNCPTR (gst_video_inference_release_pad);
  gst_element_class_add_static_pad_template (eclass, &sink_bypass_factory);
  gst_element_class_add_static_pad_template (eclass, &src_bypass_factory);

  backends_params = gst_inference_backends_get_string_properties ();
  backend_blurb = g_strdup_printf ("Type of predefined backend to use.\n"
      "\t\t\tAccording to the selected backend "
      "different properties will be available.\n "
      "\t\t\tThese properties can be accessed using the "
      "\"backend::<property>\" syntax.\n"
      "\t\t\tThe following list details the properties "
      "for each backend\n%s", backends_params);

  g_free (backends_params);

  g_object_class_install_property (oclass, PROP_BACKEND,
      g_param_spec_enum ("backend", "Backend",
          backend_blurb, GST_TYPE_INFERENCE_BACKENDS, DEFAULT_BACKEND,
          G_PARAM_READWRITE));

  g_object_class_install_property (oclass, PROP_MODEL_LOCATION,
      g_param_spec_string ("model-location", "Model Location",
          "Path to the model to use", DEFAULT_MODEL_LOCATION,
	   G_PARAM_READWRITE));

  klass->start = NULL;
  klass->stop = NULL;
  klass->preprocess = NULL;
  klass->postprocess = NULL;
}

static void
gst_video_inference_init (GstVideoInference * self)
{
  GstVideoInferencePrivate *priv = GST_VIDEO_INFERENCE_PRIVATE (self);

  priv->sink_bypass_data = NULL;
  priv->sink_model_data = NULL;

  priv->sink_bypass = NULL;
  priv->src_bypass = NULL;
  priv->sink_model = NULL;
  priv->src_model = NULL;

  priv->cpads = gst_collect_pads_new ();
  gst_collect_pads_set_function (priv->cpads, gst_video_inference_collected,
      (gpointer) (self));
  gst_collect_pads_set_event_function (priv->cpads,
      gst_video_inference_sink_event, (gpointer) (self));

  // priv->backend = GST_BACKEND (g_object_new (GST_TYPE_NCSDK, NULL));

  priv->model_location = g_strdup (DEFAULT_MODEL_LOCATION);
}

static void
gst_video_inference_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec)
{
  GstVideoInference *self = GST_VIDEO_INFERENCE (object);
  GstVideoInferencePrivate *priv = GST_VIDEO_INFERENCE_PRIVATE (self);

  GST_LOG_OBJECT (self, "Set Property");

  switch (property_id) {
    case PROP_BACKEND:
      break;
    case PROP_MODEL_LOCATION:
      GstState actual_state;
      gst_element_get_state (GST_ELEMENT(self), &actual_state, NULL, GST_SECOND);
      GST_OBJECT_LOCK (self);
      if (actual_state <= GST_STATE_READY) {
        g_free (priv->model_location);
        priv->model_location = g_value_dup_string (value);
      } else {
	  GST_ERROR_OBJECT (self, "Model location can only be set in the NULL or READY states");
      }
      GST_OBJECT_UNLOCK (self);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
gst_video_inference_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec)
{
  GstVideoInference *self = GST_VIDEO_INFERENCE (object);
  GstVideoInferencePrivate *priv = GST_VIDEO_INFERENCE_PRIVATE (self);

  GST_LOG_OBJECT (self, "Get Property");

  switch (property_id) {
    case PROP_BACKEND:
      break;
    case PROP_MODEL_LOCATION:
      g_value_set_string (value, priv->model_location);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
gst_video_inference_child_proxy_init (GstChildProxyInterface * iface)
{
  iface->get_child_by_name = gst_video_inference_get_child_by_name;
  iface->get_child_by_index = gst_video_inference_get_child_by_index;
  iface->get_children_count = gst_video_inference_get_children_count;
}

static GObject *
gst_video_inference_get_child_by_name (GstChildProxy * parent,
    const gchar * name)
{
  GstVideoInference *self = GST_VIDEO_INFERENCE (parent);
  GstVideoInferencePrivate *priv = GST_VIDEO_INFERENCE_PRIVATE (self);

  GST_DEBUG_OBJECT (self, "Requested for child %s", name);

  if (0 == g_strcmp0 (name, "backend")) {
    return G_OBJECT (g_object_ref (priv->backend));
  } else {
    GST_ERROR_OBJECT (self, "No such child %s", name);
    return NULL;
  }
}

static GObject *
gst_video_inference_get_child_by_index (GstChildProxy * parent, guint index)
{
  GstVideoInference *self = GST_VIDEO_INFERENCE (parent);
  GstVideoInferencePrivate *priv = GST_VIDEO_INFERENCE_PRIVATE (self);

  GST_DEBUG_OBJECT (self, "Requested for child %d", index);

  if (0 == index) {
    return G_OBJECT (g_object_ref (priv->backend));
  } else {
    GST_DEBUG_OBJECT (self, "No such child %d", index);
    return NULL;
  }
}

static guint
gst_video_inference_get_children_count (GstChildProxy * parent)
{
  return 1;
}

static gboolean
gst_video_inference_start (GstVideoInference * self)
{
  GstVideoInferenceClass *klass = GST_VIDEO_INFERENCE_GET_CLASS (self);
  GstVideoInferencePrivate *priv = GST_VIDEO_INFERENCE_PRIVATE (self);
  gboolean ret = TRUE;
  r2i::RuntimeError error;

  /* TODO: Avoid hardcoded NCSDK, it should be queried from the backend */
  auto factory = r2i::IFrameworkFactory::MakeFactory (r2i::FrameworkCode::NCSDK,
      error);

  GST_INFO_OBJECT (self, "Starting video inference");

  if (NULL == priv->model_location) {
    GST_ELEMENT_ERROR (self, RESOURCE, NOT_FOUND,
      ("Model Location has not been set"), (NULL));
    ret = FALSE;
    goto out;
  }

  priv->engine = factory->MakeEngine (error);
  if (error.IsError ()) {
    goto error;
  }

  priv->loader = factory->MakeLoader (error);
  if (error.IsError ()) {
    goto error;
  }

  priv->model = priv->loader->Load (priv->model_location, error);
  if (error.IsError ()) {
    goto error;
  }

  error = priv->engine->SetModel (priv->model);
  if (error.IsError ()) {
    goto error;
  }

  error = priv->engine->Start ();
  if (error.IsError ()) {
    goto error;
  }

  if (klass->start != NULL) {
    ret = klass->start (self);
  }

 out:
  return ret;
 error:
  GST_ELEMENT_ERROR (self, STREAM, FAILED,
    ("R2Inference Error: (Code:%d) %s", error.GetCode(),
     error.GetDescription ().c_str()), (NULL));
  return FALSE;
}

static gboolean
gst_video_inference_stop (GstVideoInference * self)
{
  GstVideoInferenceClass *klass = GST_VIDEO_INFERENCE_GET_CLASS (self);
  gboolean ret = TRUE;

  GST_INFO_OBJECT (self, "Stopping video inference");

  if (klass->stop != NULL) {
    ret = klass->stop (self);
  }

  return ret;
}

static GstStateChangeReturn
gst_video_inference_change_state (GstElement * element,
    GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstVideoInference *self = GST_VIDEO_INFERENCE (element);
  GstVideoInferencePrivate *priv = GST_VIDEO_INFERENCE_PRIVATE (self);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      if (FALSE == gst_video_inference_start (self)) {
        GST_ERROR_OBJECT (self, "Subclass failed to start");
        ret = GST_STATE_CHANGE_FAILURE;
        goto out;
      }

      gst_collect_pads_start (priv->cpads);
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_collect_pads_stop (priv->cpads);
      break;
    default:
      break;
  }

  ret =
      GST_ELEMENT_CLASS (gst_video_inference_parent_class)->change_state
      (element, transition);
  if (GST_STATE_CHANGE_FAILURE == ret) {
    GST_ERROR_OBJECT (self, "Parent failed to change state");
    goto out;
  }

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      if (FALSE == gst_video_inference_stop (self)) {
        GST_ERROR_OBJECT (self, "Subclass failed to stop");
        ret = GST_STATE_CHANGE_FAILURE;
        goto out;
      }
      break;
    default:
      break;
  }

out:
  return ret;
}

static GstPad *
gst_video_inference_create_pad (GstVideoInference * self,
    GstPadTemplate * templ, const gchar * name, GstCollectData ** data)
{
  GstVideoInferencePrivate *priv = GST_VIDEO_INFERENCE_PRIVATE (self);
  GstElement *element = GST_ELEMENT (self);
  GstPad *pad;

  GST_INFO_OBJECT (self, "Requested pad %s", name);
  pad = gst_pad_new_from_template (templ, name);

  if (GST_PAD_IS_SINK (pad)) {
    g_return_val_if_fail (data, NULL);

    *data =
        gst_collect_pads_add_pad (priv->cpads, pad, sizeof (GstCollectData),
        NULL, TRUE);
    if (NULL == *data) {
      GST_ERROR_OBJECT (self, "Unable to add pad %s to collect pads", name);
      goto free_pad;
    }
  } else {
    gst_pad_set_event_function (pad, gst_video_inference_src_event);
  }

  if (FALSE == gst_element_add_pad (element, pad)) {
    GST_ERROR_OBJECT (self, "Unable to add pad %s to element", name);
    goto remove_pad;
  }

  return GST_PAD_CAST (gst_object_ref (pad));

remove_pad:
  gst_collect_pads_remove_pad (priv->cpads, pad);

free_pad:
  gst_object_unref (pad);
  return NULL;
}

static GstPad *
gst_video_inference_request_new_pad (GstElement * element,
    GstPadTemplate * templ, const gchar * name, const GstCaps * caps)
{
  GstVideoInference *self = GST_VIDEO_INFERENCE (element);
  GstVideoInferencePrivate *priv = GST_VIDEO_INFERENCE_PRIVATE (self);
  const gchar *tname;
  GstPad **pad;
  GstCollectData **data;

  tname = GST_PAD_TEMPLATE_NAME_TEMPLATE (templ);

  if (0 == g_strcmp0 (tname, "sink_bypass")) {
    pad = &priv->sink_bypass;
    data = &priv->sink_bypass_data;
  } else if (0 == g_strcmp0 (tname, "sink_model")) {
    pad = &priv->sink_model;
    data = &priv->sink_model_data;
  } else if (0 == g_strcmp0 (tname, "src_bypass")) {
    pad = &priv->src_bypass;
    data = NULL;
  } else if (0 == g_strcmp0 (tname, "src_model")) {
    pad = &priv->src_model;
    data = NULL;
  } else {
    g_return_val_if_reached (NULL);
  }

  if (NULL == *pad) {
    *pad = gst_video_inference_create_pad (self, templ, name, data);
  } else {
    GST_ERROR_OBJECT (self, "Pad %s already exists", name);
    return NULL;
  }

  return *pad;
}

static void
gst_video_inference_release_pad (GstElement * element, GstPad * pad)
{
  GstVideoInference *self = GST_VIDEO_INFERENCE (element);
  GstVideoInferencePrivate *priv = GST_VIDEO_INFERENCE_PRIVATE (self);
  GstPad **ourpad;
  GstCollectData **data;

  GST_INFO_OBJECT (self, "Removing %s:%s", GST_DEBUG_PAD_NAME (pad));

  if (pad == priv->sink_bypass) {
    ourpad = &priv->sink_bypass;
    data = &priv->sink_bypass_data;
  } else if (pad == priv->src_bypass) {
    ourpad = &priv->src_bypass;
    data = NULL;
  } else if (pad == priv->sink_model) {
    ourpad = &priv->sink_model;
    data = &priv->sink_model_data;
  } else if (pad == priv->src_model) {
    ourpad = &priv->src_model;
    data = NULL;
  } else {
    g_return_if_reached ();
  }

  if (GST_PAD_IS_SINK (pad)) {
    *data = NULL;
    gst_collect_pads_remove_pad (priv->cpads, pad);
  }

  g_clear_object (ourpad);
}

static GstFlowReturn
gst_video_inference_forward_buffer (GstVideoInference * self,
    GstVideoInferencePrivate * priv, GstCollectData * data, GstPad * pad)
{
  GstVideoInferenceClass *klass = GST_VIDEO_INFERENCE_GET_CLASS (self);
  GstBuffer *buffer = NULL;
  GstFlowReturn ret = GST_FLOW_OK;
  gboolean subret = FALSE;
  GstBuffer *prepared;
  gpointer prediction;
  gsize predsize;

  /* User didn't request this pad */
  if (NULL == data) {
    goto out;
  }

  buffer = gst_collect_pads_pop (priv->cpads, data);
  if (NULL == buffer) {
    GST_INFO_OBJECT (self, "EOS requested on %s:%s",
        GST_DEBUG_PAD_NAME (data->pad));
    ret = GST_FLOW_EOS;
    goto out;
  }

  if (data->pad == priv->sink_model && NULL != klass->preprocess) {
    /* Allocate a new buffer for the preprocessing stage */
    GstAllocationParams params;
    gsize size = gst_buffer_get_size (buffer);
    GstBufferCopyFlags flags =
        (GstBufferCopyFlags) (GST_BUFFER_COPY_FLAGS | GST_BUFFER_COPY_TIMESTAMPS
        | GST_BUFFER_COPY_META);

    gst_allocation_params_init (&params);
    prepared = gst_buffer_new_allocate (NULL, size, &params);
    gst_buffer_copy_into (prepared, buffer, flags, 0, size);
    subret = klass->preprocess (self, buffer, prepared);
  } else {
    prepared = gst_buffer_ref (buffer);
    subret = TRUE;
  }

  /* This reference is no longer needed because the pre-process copied
     it or now its called prepared */
  gst_buffer_unref (buffer);

  if (FALSE == subret) {
    GST_ERROR_OBJECT (self, "Subclass failed preprocess");
    goto suberror;
  }
  // TODO: Do processing here
  predsize = 1000 * sizeof (float);
  prediction = g_malloc (predsize);

  if (NULL != klass->postprocess) {
    subret = klass->postprocess (self, prepared, prediction, predsize);
  }

  /* Prediction is no longer needed */
  g_free (prediction);

  if (FALSE == subret) {
    GST_ERROR_OBJECT (self, "Subclass failed postprocess");
    goto suberror;
  }

  if (NULL != pad) {
    GST_LOG_OBJECT (self, "Forwarding buffer to %s:%s",
        GST_DEBUG_PAD_NAME (pad));
    ret = gst_pad_push (pad, prepared);
  } else {
    GST_LOG_OBJECT (self, "Dropping buffer from %s:%s",
        GST_DEBUG_PAD_NAME (data->pad));
    gst_buffer_unref (prepared);
    goto out;
  }

  if (GST_FLOW_OK != ret) {
    GST_ERROR_OBJECT (self, "Pad %s:%s returned: (%d) %s",
        GST_DEBUG_PAD_NAME (pad), ret, gst_flow_get_name (ret));
  }

out:
  return ret;

suberror:
  gst_buffer_unref (prepared);
  return GST_FLOW_ERROR;
}

static GstFlowReturn
gst_video_inference_collected (GstCollectPads * pads, gpointer user_data)
{
  GstVideoInference *self = GST_VIDEO_INFERENCE (user_data);
  GstVideoInferencePrivate *priv = GST_VIDEO_INFERENCE_PRIVATE (self);
  GstFlowReturn ret;

  ret = gst_video_inference_forward_buffer (self, priv, priv->sink_bypass_data,
      priv->src_bypass);
  if (GST_FLOW_OK != ret) {
    goto out;
  }

  ret = gst_video_inference_forward_buffer (self, priv, priv->sink_model_data,
      priv->src_model);
  if (GST_FLOW_OK != ret) {
    goto out;
  }

out:
  return ret;
}

static GstPad *
gst_video_inference_get_src_pad (GstVideoInference * self,
    GstVideoInferencePrivate * priv, GstPad * sinkpad)
{
  GstPad *pad;

  if (sinkpad == priv->sink_model) {
    pad = priv->src_model;
  } else if (sinkpad == priv->sink_bypass) {
    pad = priv->src_bypass;
  } else {
    g_return_val_if_reached (NULL);
  }

  return pad;
}

static gboolean
gst_video_inference_sink_event (GstCollectPads * pads, GstCollectData * pad,
    GstEvent * event, gpointer user_data)
{
  GstVideoInference *self = GST_VIDEO_INFERENCE (user_data);
  GstVideoInferencePrivate *priv = GST_VIDEO_INFERENCE_PRIVATE (self);
  gboolean ret = FALSE;
  GstPad *srcpad;

  srcpad = gst_video_inference_get_src_pad (self, priv, pad->pad);

  if (NULL != srcpad) {
    GST_LOG_OBJECT (self, "Forwarding event %s from %s:%s",
	GST_EVENT_TYPE_NAME (event), GST_DEBUG_PAD_NAME (pad->pad));
    /* Collect pads will decrease the refcount of the event when we return */
    gst_event_ref (event);
    if (FALSE == gst_pad_push_event (srcpad, event)) {
      GST_ERROR_OBJECT (self, "Event %s failed in %s:%s",
	  GST_EVENT_TYPE_NAME (event), GST_DEBUG_PAD_NAME (srcpad));
      goto out;
    }
  }
  else {
    GST_LOG_OBJECT (self, "Dropping event %s from %s:%s",
       GST_EVENT_TYPE_NAME (event), GST_DEBUG_PAD_NAME (pad->pad));
  }

  ret = gst_collect_pads_event_default (priv->cpads, pad, event, FALSE);

out:
  return ret;
}

static gboolean
gst_video_inference_src_event (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  GstVideoInference *self = GST_VIDEO_INFERENCE (parent);
  GstVideoInferencePrivate *priv = GST_VIDEO_INFERENCE_PRIVATE (self);

  return gst_collect_pads_src_event_default (priv->cpads, pad, event);
}

static void
gst_video_inference_finalize (GObject * object)
{
  GstVideoInference *self = GST_VIDEO_INFERENCE (object);
  GstVideoInferencePrivate *priv = GST_VIDEO_INFERENCE_PRIVATE (self);

  g_clear_object (&(priv->cpads));
  g_clear_object (&(priv->sink_bypass));
  g_clear_object (&(priv->sink_model));
  g_clear_object (&(priv->src_bypass));
  g_clear_object (&(priv->src_model));

  priv->sink_bypass_data = NULL;
  priv->sink_model_data = NULL;
  g_free (priv->model_location);
  priv->model_location = NULL;

  g_clear_object (&priv->backend);

  G_OBJECT_CLASS (gst_video_inference_parent_class)->finalize (object);
}
