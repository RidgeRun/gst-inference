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
static GstFlowReturn gst_video_inference_pop_buffer (GstVideoInference * self,
    GstCollectPads * cpads, GstCollectData * data, GstBuffer ** buffer);
static GstFlowReturn gst_video_inference_forward_buffer (GstVideoInference *
    self, GstBuffer * buffer, GstPad * pad);
static gboolean gst_video_inference_model_buffer_process (GstVideoInference *
    self, GstVideoInferencePrivate * priv, GstBuffer * buffer, GstMeta ** meta);
static gboolean gst_video_inference_bypass_buffer_process (GstVideoInference *
    self, GstBuffer * buffer_bypass, GstBuffer * buffer_model);
static gboolean gst_video_inference_preprocess (GstVideoInference * self,
    GstVideoInferenceClass * klass, GstVideoFrame * inframe,
    GstVideoFrame * outframe);
static gboolean gst_video_inference_predict (GstVideoInference * self,
    GstVideoInferencePrivate * priv, GstVideoFrame * frame, gpointer * pred,
    gsize * pred_size);
static gboolean gst_video_inference_postprocess (GstVideoInference * self,
    GstVideoInferenceClass * klass, GstVideoFrame * frame, gpointer pred,
    gsize pred_size);


static gboolean gst_video_inference_sink_event (GstCollectPads * pads,
    GstCollectData * pad, GstEvent * event, gpointer user_data);
static gboolean gst_video_inference_src_event (GstPad * pad, GstObject * parent,
    GstEvent * event);
static GstPad *gst_video_inference_get_src_pad (GstVideoInference * self,
    GstVideoInferencePrivate * priv, GstPad * pad);
static void
gst_video_inference_set_backend (GstVideoInference * self, gint backend);
static guint gst_video_inference_get_backend_type (GstVideoInference * self);

static void video_inference_map_buffers (GstPad * pad, GstBuffer * buffer,
    GstVideoFrame * inframe, GstVideoFrame * outframe);

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

  GST_INFO_OBJECT (self, "Removing %" GST_PTR_FORMAT, pad);

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
    GstBuffer * buffer, GstPad * pad)
{
  GstFlowReturn ret = GST_FLOW_OK;

  /* User didn't request this pad */
  if (NULL == pad) {
    GST_LOG_OBJECT (self, "Dropping buffer %" GST_PTR_FORMAT, buffer);
    gst_buffer_unref (buffer);
    return ret;
  }

  GST_LOG_OBJECT (self,
      "Forwarding buffer %" GST_PTR_FORMAT " to %" GST_PTR_FORMAT, buffer, pad);
  ret = gst_pad_push (pad, gst_buffer_ref (buffer));

  if (GST_FLOW_OK != ret) {
    GST_ERROR_OBJECT (self, "Pad %" GST_PTR_FORMAT " returned: (%d) %s",
        pad, ret, gst_flow_get_name (ret));
  }

  return ret;
}

static void
video_inference_map_buffers (GstPad * pad, GstBuffer * inbuf,
    GstVideoFrame * inframe, GstVideoFrame * outframe)
{
  GstCaps *caps;
  GstVideoInfo info;
  GstAllocationParams params;
  GstBuffer *outbuf;
  gsize size;

  g_return_if_fail (pad);
  g_return_if_fail (inbuf);
  g_return_if_fail (inframe);
  g_return_if_fail (outframe);

  /* Generate a new video info based on our caps */
  gst_video_info_init (&info);
  caps = gst_pad_get_current_caps (pad);
  gst_video_info_from_caps (&info, caps);
  gst_caps_unref (caps);

  /* Allocate an output buffer for the pre-processed data */
  gst_allocation_params_init (&params);
  size = gst_buffer_get_size (inbuf);
  outbuf = gst_buffer_new_allocate (NULL, size * sizeof (float), &params);

  /* Map buffers into their respective output frames */
  gst_video_frame_map (inframe, &info, inbuf, GST_MAP_READ);
  gst_video_frame_map (outframe, &info, outbuf, GST_MAP_WRITE);
}

static gboolean
gst_video_inference_preprocess (GstVideoInference * self,
    GstVideoInferenceClass * klass, GstVideoFrame * inframe,
    GstVideoFrame * outframe)
{
  g_return_val_if_fail (self, FALSE);
  g_return_val_if_fail (klass, FALSE);
  g_return_val_if_fail (inframe, FALSE);
  g_return_val_if_fail (outframe, FALSE);

  if (NULL == klass->preprocess) {
    GST_ELEMENT_ERROR (self, STREAM, FAILED,
        ("Subclass did not implement preprocess"), (NULL));
    return FALSE;
  }

  if (!klass->preprocess (self, inframe, outframe)) {
    GST_ELEMENT_ERROR (self, STREAM, FAILED,
        ("Subclass failed to preprocess"), (NULL));
    return FALSE;
  }

  return TRUE;
}

static gboolean
gst_video_inference_predict (GstVideoInference * self,
    GstVideoInferencePrivate * priv, GstVideoFrame * frame, gpointer * pred,
    gsize * pred_size)
{
  GError *error = NULL;

  g_return_val_if_fail (self, FALSE);
  g_return_val_if_fail (priv, FALSE);
  g_return_val_if_fail (frame, FALSE);
  g_return_val_if_fail (pred, FALSE);
  g_return_val_if_fail (pred_size, FALSE);

  GST_LOG_OBJECT (self, "Running prediction on frame");

  if (!gst_backend_process_frame (priv->backend, frame, pred, pred_size,
          &error)) {
    GST_ELEMENT_ERROR (self, STREAM, FAILED,
        ("Could not process using the selected backend: (%s)", error->message),
        (NULL));
    g_error_free (error);
    return FALSE;
  }

  return TRUE;
}

static gboolean
gst_video_inference_postprocess (GstVideoInference * self,
    GstVideoInferenceClass * klass, GstVideoFrame * frame, gpointer pred,
    gsize pred_size)
{
  GstMeta *meta;
  gboolean pred_valid;

  g_return_val_if_fail (self, FALSE);
  g_return_val_if_fail (klass, FALSE);
  g_return_val_if_fail (frame, FALSE);
  g_return_val_if_fail (pred, FALSE);
  g_return_val_if_fail (pred_size, FALSE);

  /* Subclass didn't implement a post-process, dont fail, just ignore */
  if (NULL != klass->postprocess) {
    return TRUE;
  }

  meta = gst_buffer_add_meta (frame->buffer, klass->inference_meta_info, NULL);

  if (!klass->postprocess (self, meta, frame, pred, pred_size, &pred_valid)) {
    GST_ELEMENT_ERROR (self, STREAM, FAILED,
        ("Subclass failed at preprocess"), (NULL));
    return FALSE;
  }

  if (pred_valid) {
    g_signal_emit (self, gst_video_inference_signals[NEW_PREDICTION_SIGNAL], 0,
        meta, frame->buffer);
  } else {
    gst_buffer_remove_meta (frame->buffer, meta);
  }

  return TRUE;
}

static gboolean
gst_video_inference_model_buffer_process (GstVideoInference * self,
    GstVideoInferencePrivate * priv, GstBuffer * buffer, GstMeta ** meta)
{
  GstVideoInferenceClass *klass;
  GstVideoFrame inframe, outframe;
  gboolean ret;
  gpointer prediction_data;
  gsize prediction_size;

  g_return_val_if_fail (self, FALSE);
  g_return_val_if_fail (priv, FALSE);
  g_return_val_if_fail (buffer, FALSE);
  g_return_val_if_fail (meta, FALSE);

  g_return_val_if_fail (gst_buffer_is_writable (buffer), FALSE);

  klass = GST_VIDEO_INFERENCE_GET_CLASS (self);
  *meta = NULL;

  video_inference_map_buffers (priv->sink_model, buffer, &inframe, &outframe);

  if (!gst_video_inference_preprocess (self, klass, &inframe, &outframe)) {
    ret = FALSE;
    goto free_frames;
  }

  if (!gst_video_inference_predict (self, priv, &outframe, &prediction_data,
          &prediction_size)) {
    ret = FALSE;
    goto free_frames;
  }

  if (!gst_video_inference_postprocess (self, klass, &inframe, prediction_data,
          prediction_size)) {
    ret = FALSE;
    goto free_frames;
  }

  ret = TRUE;
  g_free (prediction_data);

free_frames:
  gst_video_frame_unmap (&outframe);
  gst_video_frame_unmap (&inframe);

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
gst_video_inference_pop_buffer (GstVideoInference * self,
    GstCollectPads * cpads, GstCollectData * data, GstBuffer ** buffer)
{
  g_return_val_if_fail (self, GST_FLOW_ERROR);
  g_return_val_if_fail (buffer, GST_FLOW_ERROR);

  *buffer = NULL;

  if (NULL == data) {
    return GST_FLOW_OK;
  }

  *buffer = gst_collect_pads_pop (cpads, data);
  if (NULL == *buffer) {
    GST_INFO_OBJECT (self, "EOS requested on %" GST_PTR_FORMAT, data->pad);
    return GST_FLOW_EOS;
  }

  GST_LOG_OBJECT (self, "Popped %" GST_PTR_FORMAT " from %" GST_PTR_FORMAT,
      *buffer, data->pad);
  return GST_FLOW_OK;
}

static GstFlowReturn
gst_video_inference_collected (GstCollectPads * pads, gpointer user_data)
{
  GstVideoInference *self = GST_VIDEO_INFERENCE (user_data);
  GstVideoInferencePrivate *priv = GST_VIDEO_INFERENCE_PRIVATE (self);
  GstFlowReturn ret = GST_FLOW_OK;
  GstBuffer *buffer_model = NULL;
  GstBuffer *buffer_bypass = NULL;
  GstMeta *meta = NULL;

  ret =
      gst_video_inference_pop_buffer (self, pads, priv->sink_model_data,
      &buffer_model);
  if (GST_FLOW_OK != ret) {
    goto out;
  }

  if (buffer_model) {
    buffer_model = gst_buffer_make_writable (buffer_model);

    if (!gst_video_inference_model_buffer_process (self, priv, buffer_model,
            &meta)) {
      ret = GST_FLOW_ERROR;
      goto model_free;
    }
  }

  ret =
      gst_video_inference_pop_buffer (self, pads, priv->sink_bypass_data,
      &buffer_bypass);
  if (GST_FLOW_OK != ret) {
    goto model_free;
  }

  if (buffer_bypass) {
    if (!gst_video_inference_bypass_buffer_process (self, buffer_bypass,
            buffer_model)) {
      ret = GST_FLOW_ERROR;
      goto bypass_free;
    }
  }

  /* Forward buffers to src pads */
  if (NULL != buffer_model) {
    ret =
        gst_video_inference_forward_buffer (self, buffer_model,
        priv->src_model);

    /* We don't own this buffer anymore, don't free it */
    buffer_model = NULL;

    if (GST_FLOW_OK != ret) {
      goto bypass_free;
    }
  }

  if (NULL != buffer_bypass) {
    ret =
        gst_video_inference_forward_buffer (self,
        buffer_bypass, priv->src_bypass);
    if (GST_FLOW_OK != ret) {
      /* No need to free bypass buffer */
      goto model_free;
    }
  }

  goto out;

bypass_free:
  if (buffer_bypass) {
    gst_buffer_unref (buffer_bypass);
  }

model_free:
  if (buffer_model) {
    gst_buffer_unref (buffer_model);
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
    GST_LOG_OBJECT (self, "Forwarding event %s from %" GST_PTR_FORMAT,
        GST_EVENT_TYPE_NAME (event), pad->pad);
    /* Collect pads will decrease the refcount of the event when we return */
    gst_event_ref (event);
    if (FALSE == gst_pad_push_event (srcpad, event)) {
      GST_ERROR_OBJECT (self, "Event %s failed in %" GST_PTR_FORMAT,
          GST_EVENT_TYPE_NAME (event), srcpad);
      goto out;
    }
  } else {
    GST_LOG_OBJECT (self, "Dropping event %s from %" GST_PTR_FORMAT,
        GST_EVENT_TYPE_NAME (event), pad->pad);
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
