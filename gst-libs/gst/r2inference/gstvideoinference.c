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
#include "gstbackend.h"

#include <gst/base/gstcollectpads.h>


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

#define DEFAULT_MODEL_LOCATION   NULL

enum
{
  NEW_PREDICTION_SIGNAL,
  LAST_SIGNAL
};

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
    self, GstBuffer * buffer, GstCollectData * data, GstPad * pad);
static gboolean gst_video_inference_model_buffer_process (GstVideoInference *
    self, GstBuffer * buffer);
static gboolean gst_video_inference_bypass_buffer_process (GstVideoInference *
    self, GstBuffer * buffer_bypass, GstBuffer * buffer_model);
static gboolean gst_video_inference_sink_event (GstCollectPads * pads,
    GstCollectData * pad, GstEvent * event, gpointer user_data);
static gboolean gst_video_inference_src_event (GstPad * pad, GstObject * parent,
    GstEvent * event);
static GstPad *gst_video_inference_get_src_pad (GstVideoInference * self,
    GstVideoInferencePrivate * priv, GstPad * pad);
static void
gst_video_inference_set_backend (GstVideoInference * self, gint backend);
static guint gst_video_inference_get_backend_type (GstVideoInference * self);

static guint gst_video_inference_signals[LAST_SIGNAL] = { 0 };

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
      g_param_spec_enum ("backend", "Backend", backend_blurb,
          GST_TYPE_INFERENCE_BACKENDS,
          gst_inference_backends_get_default_backend (), G_PARAM_READWRITE));

  g_object_class_install_property (oclass, PROP_MODEL_LOCATION,
      g_param_spec_string ("model-location", "Model Location",
          "Path to the model to use", DEFAULT_MODEL_LOCATION,
          G_PARAM_READWRITE));

  gst_video_inference_signals[NEW_PREDICTION_SIGNAL] =
      g_signal_new ("new-prediction", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_FIRST, 0, NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_POINTER,
      GST_TYPE_BUFFER);

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

  priv->model_location = g_strdup (DEFAULT_MODEL_LOCATION);

  gst_video_inference_set_backend (self,
      gst_inference_backends_get_default_backend ());
}

static void
gst_video_inference_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec)
{
  GstVideoInference *self = GST_VIDEO_INFERENCE (object);
  GstVideoInferencePrivate *priv = GST_VIDEO_INFERENCE_PRIVATE (self);
  GstState actual_state;

  GST_LOG_OBJECT (self, "Set Property");

  switch (property_id) {
    case PROP_BACKEND:
      GST_OBJECT_LOCK (self);
      gst_video_inference_set_backend (self, g_value_get_enum (value));
      GST_OBJECT_UNLOCK (self);
      break;
    case PROP_MODEL_LOCATION:
      gst_element_get_state (GST_ELEMENT (self), &actual_state, NULL,
          GST_SECOND);
      GST_OBJECT_LOCK (self);
      if (actual_state <= GST_STATE_READY) {
        g_free (priv->model_location);
        priv->model_location = g_value_dup_string (value);
      } else {
        GST_ERROR_OBJECT (self,
            "Model location can only be set in the NULL or READY states");
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
      g_value_set_enum (value, gst_video_inference_get_backend_type (self));
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
  GError *err = NULL;

  GST_INFO_OBJECT (self, "Starting video inference");
  if (NULL == priv->model_location) {
    GST_ELEMENT_ERROR (self, RESOURCE, NOT_FOUND,
        ("Model Location has not been set"), (NULL));
    ret = FALSE;
    goto out;
  }

  if (!gst_backend_start (priv->backend, priv->model_location, &err)) {
    GST_ELEMENT_ERROR (self, LIBRARY, INIT,
        ("Could not start the selected backend: (%s)", err->message), (NULL));
    ret = FALSE;
  }

  if (klass->start != NULL) {
    ret = klass->start (self);
  }

out:
  if (err)
    g_error_free (err);
  return ret;
}

static gboolean
gst_video_inference_stop (GstVideoInference * self)
{
  GstVideoInferenceClass *klass = GST_VIDEO_INFERENCE_GET_CLASS (self);
  GstVideoInferencePrivate *priv = GST_VIDEO_INFERENCE_PRIVATE (self);
  gboolean ret = TRUE;
  GError *err = NULL;

  GST_INFO_OBJECT (self, "Stopping video inference");

  if (!gst_backend_stop (priv->backend, &err)) {
    GST_ELEMENT_ERROR (self, LIBRARY, INIT,
        ("Could not stop the selected backend: (%s)", err->message), (NULL));
    ret = FALSE;
  }

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
    GstBuffer * buffer, GstCollectData * data, GstPad * pad)
{
  GstFlowReturn ret = GST_FLOW_OK;

  /* User didn't request this pad */
  if (NULL == data || pad == NULL) {
    GST_LOG_OBJECT (self, "Dropping buffer from %s:%s",
        GST_DEBUG_PAD_NAME (data->pad));
    goto out;
  }

  GST_LOG_OBJECT (self, "Forwarding buffer to %s:%s", GST_DEBUG_PAD_NAME (pad));
  ret = gst_pad_push (pad, gst_buffer_ref (buffer));

  if (GST_FLOW_FLUSHING == ret || GST_FLOW_EOS == ret) {
    GST_INFO_OBJECT (self, "Pad %s:%s returned: (%d) %s",
        GST_DEBUG_PAD_NAME (pad), ret, gst_flow_get_name (ret));
  }

  else if (GST_FLOW_OK != ret) {
    GST_ERROR_OBJECT (self, "Pad %s:%s returned: (%d) %s",
        GST_DEBUG_PAD_NAME (pad), ret, gst_flow_get_name (ret));
  }

out:
  gst_buffer_unref (buffer);
  return ret;
}

static gboolean
gst_video_inference_model_buffer_process (GstVideoInference * self,
    GstBuffer * buffer)
{
  GstVideoInferenceClass *klass;
  GstVideoInferencePrivate *priv;
  GstBuffer *outbuf;
  GstVideoFrame inframe, outframe;
  GstAllocationParams params;
  GstVideoInfo vininfo;
  GstCaps *pad_caps;
  gboolean ret;
  gpointer prediction_data = NULL;
  gsize prediction_size;
  GError *err = NULL;
  GstMeta *inference_meta;
  gboolean valid_prediction;

  klass = GST_VIDEO_INFERENCE_GET_CLASS (self);
  priv = GST_VIDEO_INFERENCE_PRIVATE (self);

  outbuf = NULL;
  ret = TRUE;
  valid_prediction = FALSE;

  if (NULL == klass->preprocess) {
    GST_ELEMENT_ERROR (self, STREAM, FAILED,
        ("Subclass did not implement preprocess"), (NULL));
    ret = FALSE;
    goto out;
  }

  gst_video_info_init (&vininfo);
  pad_caps = gst_pad_get_current_caps (priv->sink_model);
  gst_video_info_from_caps (&vininfo, pad_caps);
  gst_caps_unref (pad_caps);

/*
 * FIXME:
 *
 * This is a temporal hack in order to avoid buffers being unecessarily
 * copied. It looks like a bug in GstCollectPads that needs to be submited.
 * After a lot of debugging, I found that it is impossible for this buffer
 * to have a refcount of 1 at this point since it is referenced by the core
 * just before calling this function. What this means is that GStreamer will
 * think another element besides us is using this buffer and, hence, is not
 * writable. We temporaly unref the buffer to force it to 1,(no copy under
 * this condition) and return the refcount to its original value.
 *
 * Note that this only occurs if we are trying to use the buffer that was
 * just pushed into the pad. If we are queueing more than one buffer, we are
 * safe since the buffer has the normal refcount of 1. On the other hand, if
 * the buffer's refcount is really other than 2, then another element is
 * indeed using the buffer and hence data will be copied.
 *
 * Really, please fix me!
 *
 */
  if (GST_MINI_OBJECT_REFCOUNT_VALUE (buffer) == 2) {
    gst_buffer_unref (buffer);
    inference_meta =
        gst_buffer_add_meta (buffer, klass->inference_meta_info, NULL);
    gst_buffer_ref (buffer);
  } else {
    buffer = gst_buffer_make_writable (buffer);
    inference_meta =
        gst_buffer_add_meta (buffer, klass->inference_meta_info, NULL);
  }

  gst_allocation_params_init (&params);
  outbuf =
      gst_buffer_new_allocate (NULL,
      gst_buffer_get_size (buffer) * sizeof (gfloat), &params);
  gst_video_frame_map (&inframe, &vininfo, buffer, GST_MAP_READ);
  gst_video_frame_map (&outframe, &vininfo, outbuf, GST_MAP_WRITE);

  if (!klass->preprocess (self, &inframe, &outframe)) {
    GST_ELEMENT_ERROR (self, STREAM, FAILED,
        ("Subclass failed to preprocess"), (NULL));
    ret = FALSE;
    goto free_frames;
  }

  GST_LOG_OBJECT (self, "Processing frame using selected Backend");

  if (!gst_backend_process_frame (priv->backend, &outframe,
          &prediction_data, &prediction_size, &err)) {
    GST_ELEMENT_ERROR (self, STREAM, FAILED,
        ("Could not process using the selected backend: (%s)",
            err->message), (NULL));
    ret = FALSE;
    goto free_frames;
  }

  if (NULL != klass->postprocess) {
    if (!klass->postprocess (self, inference_meta, &outframe, prediction_data,
            prediction_size, &valid_prediction)) {
      ret = FALSE;
      GST_ELEMENT_ERROR (self, STREAM, FAILED,
          ("Subclass failed at preprocess"), (NULL));
      goto free_frames;
    }
  }

free_frames:
  if (prediction_data)
    g_free (prediction_data);
  gst_video_frame_unmap (&outframe);
  gst_video_frame_unmap (&inframe);
  if (valid_prediction) {
    g_signal_emit (self, gst_video_inference_signals[NEW_PREDICTION_SIGNAL], 0,
        buffer, inference_meta);
  } else {
    if (GST_MINI_OBJECT_REFCOUNT_VALUE (buffer) == 2) {
      gst_buffer_unref (buffer);
      gst_buffer_remove_meta (buffer, inference_meta);
      gst_buffer_ref (buffer);
    } else {
      gst_buffer_remove_meta (buffer, inference_meta);
    }
  }
  gst_buffer_unref (outbuf);
out:
  if (err)
    g_error_free (err);
  return ret;
}

static gboolean
gst_video_inference_bypass_buffer_process (GstVideoInference * self,
    GstBuffer * buffer_bypass, GstBuffer * buffer_model)
{
  /* TODO: Get GstMetas from Model buffer and attach them to the
     Bypass Buffer */

  return TRUE;
}

static GstFlowReturn
gst_video_inference_collected (GstCollectPads * pads, gpointer user_data)
{
  GstVideoInference *self = GST_VIDEO_INFERENCE (user_data);
  GstVideoInferencePrivate *priv = GST_VIDEO_INFERENCE_PRIVATE (self);
  GstFlowReturn ret = GST_FLOW_OK;
  GstBuffer *buffer_model = NULL;
  GstBuffer *buffer_bypass = NULL;

  /* Process Model Buffer */
  if (priv->sink_model_data) {
    buffer_model = gst_collect_pads_pop (priv->cpads, priv->sink_model_data);
    if (NULL == buffer_model) {
      GST_INFO_OBJECT (self, "EOS requested on %s:%s",
          GST_DEBUG_PAD_NAME (priv->sink_model_data->pad));
      ret = GST_FLOW_EOS;
      goto out;
    }
    if (!gst_video_inference_model_buffer_process (self, buffer_model)) {
      ret = GST_FLOW_ERROR;
      goto model_free;
    }
  }

  /* Process Bypass Buffer */
  if (priv->sink_bypass_data) {
    buffer_bypass = gst_collect_pads_pop (priv->cpads, priv->sink_bypass_data);
    if (NULL == buffer_bypass) {
      GST_INFO_OBJECT (self, "EOS requested on %s:%s",
          GST_DEBUG_PAD_NAME (priv->sink_bypass_data->pad));
      ret = GST_FLOW_EOS;
      goto model_free;
    }
    if (!gst_video_inference_bypass_buffer_process (self, buffer_bypass,
            buffer_model)) {
      ret = GST_FLOW_ERROR;
      goto bypass_free;
    }
  }

  /* Forward buffers to src pads */
  if (NULL != buffer_model) {
    ret =
        gst_video_inference_forward_buffer (self, gst_buffer_ref (buffer_model),
        priv->sink_model_data, priv->src_model);
    if (GST_FLOW_OK != ret) {
      goto bypass_free;
    }
  }

  if (NULL != buffer_bypass) {
    ret =
        gst_video_inference_forward_buffer (self,
        gst_buffer_ref (buffer_bypass), priv->sink_bypass_data,
        priv->src_bypass);
    if (GST_FLOW_OK != ret) {
      goto bypass_free;
    }
  }

bypass_free:
  gst_buffer_unref (buffer_bypass);

model_free:
  gst_buffer_unref (buffer_model);

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
  } else {
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

  if (priv->backend)
    g_object_unref (priv->backend);

  G_OBJECT_CLASS (gst_video_inference_parent_class)->finalize (object);
}

static void
gst_video_inference_set_backend (GstVideoInference * self, gint backend)
{
  GstBackend *backend_new;
  GType backend_type;
  GstVideoInferencePrivate *priv = GST_VIDEO_INFERENCE_PRIVATE (self);

  g_return_if_fail (priv);

  if (GST_STATE (self) != GST_STATE_NULL) {
    g_warning ("Can't set backend property  if not on NULL state");
    return;
  }

  if (priv->backend)
    g_object_unref (priv->backend);

  backend_type = gst_inference_backends_search_type (backend);
  backend_new = (GstBackend *) g_object_new (backend_type, NULL);
  priv->backend = backend_new;

  return;
}

static guint
gst_video_inference_get_backend_type (GstVideoInference * self)
{
  GstVideoInferencePrivate *priv = GST_VIDEO_INFERENCE_PRIVATE (self);

  g_return_val_if_fail (priv, -1);

  return gst_backend_get_framework_code (priv->backend);
}
