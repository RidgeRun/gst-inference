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

#include "gstvideoinference.h"
#include "gstinferencebackends.h"
#include "gstinferencemeta.h"
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
#define DEFAULT_LABELS NULL
#define DEFAULT_NUM_LABELS 0
enum
{
  NEW_PREDICTION_SIGNAL,
  NEW_INFERENCE_SIGNAL,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_BACKEND,
  PROP_MODEL_LOCATION,
  PROP_LABELS,
};

GQuark _size_quark;
GQuark _orientation_quark;
GQuark _scale_quark;
GQuark _copy_quark;

typedef struct _GstVideoInferencePad GstVideoInferencePad;
struct _GstVideoInferencePad
{
  GstCollectData data;

  GstVideoInfo info;
};

typedef struct _GstVideoInferencePrivate GstVideoInferencePrivate;
struct _GstVideoInferencePrivate
{
  GstCollectPads *cpads;
  GstVideoInferencePad *sink_bypass_data;
  GstVideoInferencePad *sink_model_data;
  const GstMetaInfo *inference_meta_info;

  GstPad *sink_bypass;
  GstPad *src_bypass;
  GstPad *sink_model;
  GstPad *src_model;

  GstBackend *backend;

  gchar *model_location;

  GMutex mtx_model_queue;
  GMutex mtx_bypass_queue;

  GQueue *model_queue;
  GQueue *bypass_queue;

  gchar *labels;
  gchar **labels_list;
  gint num_labels;
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
    GstPadTemplate * templ, const gchar * name, GstVideoInferencePad ** data);
static GstFlowReturn gst_video_inference_process_bypass (GstVideoInference *
    self, GstBuffer * buffer, GstVideoInferencePad * pad);
static GstFlowReturn gst_video_inference_process_model (GstVideoInference *
    self, GstBuffer * buffer, GstVideoInferencePad * pad);
static GstFlowReturn gst_video_inference_buffer_function (GstCollectPads * pads,
    GstCollectData * data, GstBuffer * buffer, gpointer user_data);
static GstFlowReturn gst_video_inference_forward_buffer (GstVideoInference *
    self, GstBuffer * buffer, GstPad * pad);
static gboolean gst_video_inference_model_run_prediction (GstVideoInference *
    self, GstVideoInferenceClass * klass, GstVideoInferencePrivate * priv,
    GstBuffer * buffer, gpointer * prediction_data, gsize * prediction_size);

static gboolean gst_video_inference_preprocess (GstVideoInference * self,
    GstVideoInferenceClass * klass, GstVideoFrame * inframe,
    GstVideoFrame * outframe);
static gboolean gst_video_inference_predict (GstVideoInference * self,
    GstVideoInferencePrivate * priv, GstVideoFrame * frame, gpointer * pred,
    gsize * pred_size);

static GstIterator *gst_video_inference_iterate_internal_links (GstPad * pad,
    GstObject * parent);
static gboolean gst_video_inference_sink_event (GstCollectPads * pads,
    GstCollectData * pad, GstEvent * event, gpointer user_data);
static gboolean gst_video_inference_src_event (GstPad * pad, GstObject * parent,
    GstEvent * event);
static GstPad *gst_video_inference_get_src_pad (GstVideoInference * self,
    GstVideoInferencePrivate * priv, GstPad * pad);
static GstPad *gst_video_inference_get_sink_pad (GstVideoInference * self,
    GstVideoInferencePrivate * priv, GstPad * pad);
static void
gst_video_inference_set_backend (GstVideoInference * self, gint backend);
static guint gst_video_inference_get_backend_type (GstVideoInference * self);
static void gst_video_inference_set_caps (GstVideoInference * self,
    GstVideoInferencePrivate * priv, GstCollectData * pad, GstEvent * event);

static void video_inference_map_buffers (GstVideoInferencePad * data,
    GstBuffer * inbuf, GstVideoFrame * inframe, GstVideoFrame * outframe);
static gboolean video_inference_prepare_postprocess (const GstMetaInfo *
    meta_info, GstBuffer * buffer, GstVideoInfo * video_info,
    GstMeta ** out_meta);
static GstMeta *video_inference_transform_meta (GstBuffer * buffer_model,
    GstVideoInfo * info_model, GstMeta * meta_model, GstBuffer * buffer_bypass,
    GstVideoInfo * info_bypass);

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
  g_object_class_install_property (oclass, PROP_LABELS,
      g_param_spec_string ("labels", "labels",
          "Semicolon separated string containing inference labels",
          DEFAULT_LABELS, G_PARAM_READWRITE));

  gst_video_inference_signals[NEW_PREDICTION_SIGNAL] =
      g_signal_new ("new-prediction", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_FIRST, 0, NULL, NULL, NULL, G_TYPE_NONE, 4, G_TYPE_POINTER,
      G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_POINTER);

  gst_video_inference_signals[NEW_INFERENCE_SIGNAL] =
      g_signal_new ("new-inference", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_FIRST, 0, NULL, NULL, NULL, G_TYPE_NONE, 4, G_TYPE_POINTER,
      G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_POINTER);

  klass->start = NULL;
  klass->stop = NULL;
  klass->preprocess = NULL;
  klass->postprocess = NULL;

  _size_quark = g_quark_from_static_string (GST_META_TAG_VIDEO_SIZE_STR);
  _orientation_quark =
      g_quark_from_static_string (GST_META_TAG_VIDEO_ORIENTATION_STR);
  _scale_quark = gst_video_meta_transform_scale_get_quark ();
  _copy_quark = g_quark_from_static_string ("gst-copy");
}

static void
gst_video_inference_init (GstVideoInference * self)
{
  GstVideoInferencePrivate *priv = GST_VIDEO_INFERENCE_PRIVATE (self);

  priv->labels = DEFAULT_LABELS;
  priv->labels_list = DEFAULT_LABELS;
  priv->num_labels = DEFAULT_NUM_LABELS;

  priv->sink_bypass_data = NULL;
  priv->sink_model_data = NULL;

  priv->sink_bypass = NULL;
  priv->src_bypass = NULL;
  priv->sink_model = NULL;
  priv->src_model = NULL;
  priv->inference_meta_info = gst_inference_meta_get_info ();

  priv->cpads = gst_collect_pads_new ();

  g_mutex_init (&priv->mtx_model_queue);
  g_mutex_init (&priv->mtx_bypass_queue);

  priv->model_queue = g_queue_new ();
  priv->bypass_queue = g_queue_new ();

  /* Use buffer function to handle each pad buffer independently */
  gst_collect_pads_set_buffer_function (priv->cpads,
      gst_video_inference_buffer_function, (gpointer) self);

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
    case PROP_LABELS:
      if (priv->labels != NULL) {
        g_free (priv->labels);
      }
      if (priv->labels_list != NULL) {
        g_strfreev (priv->labels_list);
      }
      priv->labels = g_value_dup_string (value);
      priv->labels_list = g_strsplit (g_value_get_string (value), ";", 0);
      priv->num_labels = g_strv_length (priv->labels_list);
      GST_DEBUG_OBJECT (self, "Changed inference labels %s", priv->labels);
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
    case PROP_LABELS:
      g_value_set_string (value, priv->labels);
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
    GstPadTemplate * templ, const gchar * name, GstVideoInferencePad ** data)
{
  GstVideoInferencePrivate *priv = GST_VIDEO_INFERENCE_PRIVATE (self);
  GstElement *element = GST_ELEMENT (self);
  GstPad *pad;

  GST_INFO_OBJECT (self, "Requested pad %s", name);
  pad = gst_pad_new_from_template (templ, name);

  if (GST_PAD_IS_SINK (pad)) {
    g_return_val_if_fail (data, NULL);

    *data =
        (GstVideoInferencePad *) gst_collect_pads_add_pad (priv->cpads, pad,
        sizeof (GstVideoInferencePad), NULL, TRUE);
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

  gst_pad_set_iterate_internal_links_function (pad,
      gst_video_inference_iterate_internal_links);

  GST_PAD_SET_PROXY_CAPS (pad);
  GST_PAD_SET_PROXY_ALLOCATION (pad);
  GST_PAD_SET_PROXY_SCHEDULING (pad);

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
  GstVideoInferencePad **data;

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
  GstVideoInferencePad **data;

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
  GstDebugLevel level = GST_LEVEL_LOG;

  g_return_val_if_fail (self, GST_FLOW_ERROR);

  /* No buffer to forward */
  if (NULL == buffer) {
    return ret;
  }

  /* User didn't request this pad */
  if (NULL == pad) {
    GST_LOG_OBJECT (self, "Dropping buffer %" GST_PTR_FORMAT, buffer);
    gst_buffer_unref (buffer);
    return ret;
  }

  GST_LOG_OBJECT (self,
      "Forwarding buffer %" GST_PTR_FORMAT " to %" GST_PTR_FORMAT, buffer, pad);
  ret = gst_pad_push (pad, buffer);

  if (GST_FLOW_OK != ret && GST_FLOW_FLUSHING != ret && GST_FLOW_EOS != ret) {
    level = GST_LEVEL_ERROR;
  }

  GST_CAT_LEVEL_LOG (GST_CAT_DEFAULT, level, self,
      "Pad %" GST_PTR_FORMAT " returned: (%d) %s", pad, ret,
      gst_flow_get_name (ret));

  return ret;
}

static void
video_inference_map_buffers (GstVideoInferencePad * cpad, GstBuffer * inbuf,
    GstVideoFrame * inframe, GstVideoFrame * outframe)
{
  GstVideoInfo *info;
  GstAllocationParams params;
  GstBuffer *outbuf;
  gsize size;
  GstMapFlags inflags;
  GstMapFlags outflags;

  g_return_if_fail (cpad);
  g_return_if_fail (inbuf);
  g_return_if_fail (inframe);
  g_return_if_fail (outframe);

  info = &(cpad->info);

  /* Allocate an output buffer for the pre-processed data */
  gst_allocation_params_init (&params);
  size = gst_buffer_get_size (inbuf);
  outbuf = gst_buffer_new_allocate (NULL, size * sizeof (float), &params);

  /* Map buffers into their respective output frames but dont increase
   * the refcount so we can add metas later on.
   */
  inflags = (GstMapFlags) (GST_MAP_READ | GST_VIDEO_FRAME_MAP_FLAG_NO_REF);
  gst_video_frame_map (inframe, info, inbuf, inflags);

  outflags = (GstMapFlags) (GST_MAP_WRITE | GST_VIDEO_FRAME_MAP_FLAG_NO_REF);
  gst_video_frame_map (outframe, info, outbuf, outflags);
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

  GST_LOG_OBJECT (self, "Calling frame preprocess");

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
gst_video_inference_model_run_prediction (GstVideoInference * self,
    GstVideoInferenceClass * klass, GstVideoInferencePrivate * priv,
    GstBuffer * buffer, gpointer * prediction_data, gsize * prediction_size)
{
  GstVideoFrame inframe, outframe;
  GstBuffer *outbuf;
  gboolean ret;

  g_return_val_if_fail (self, FALSE);
  g_return_val_if_fail (klass, FALSE);
  g_return_val_if_fail (priv, FALSE);
  g_return_val_if_fail (buffer, FALSE);
  g_return_val_if_fail (prediction_data, FALSE);
  g_return_val_if_fail (prediction_size, FALSE);

  video_inference_map_buffers (priv->sink_model_data, buffer, &inframe,
      &outframe);
  outbuf = outframe.buffer;

  if (!gst_video_inference_preprocess (self, klass, &inframe, &outframe)) {
    ret = FALSE;
    goto free_frames;
  }

  if (!gst_video_inference_predict (self, priv, &outframe, prediction_data,
          prediction_size)) {
    ret = FALSE;
    goto free_frames;
  }

  ret = TRUE;

free_frames:
  gst_video_frame_unmap (&inframe);
  gst_video_frame_unmap (&outframe);
  gst_buffer_unref (outbuf);

  return ret;
}

static gboolean
video_inference_prepare_postprocess (const GstMetaInfo * meta_info,
    GstBuffer * buffer, GstVideoInfo * video_info, GstMeta ** out_meta)
{
  GstInferenceMeta *imeta = NULL;

  g_return_val_if_fail (meta_info, FALSE);
  g_return_val_if_fail (buffer, FALSE);
  g_return_val_if_fail (video_info, FALSE);
  g_return_val_if_fail (out_meta, FALSE);

  g_return_val_if_fail (gst_buffer_is_writable (buffer), FALSE);
  out_meta[0] = gst_buffer_add_meta (buffer, meta_info, NULL);

  /* Create new meta only if the buffer didn't have one */
  if (!out_meta[1]) {
    out_meta[1] =
        gst_buffer_add_meta (buffer, gst_inference_meta_get_info (), NULL);

    imeta = (GstInferenceMeta *) out_meta[1];
    imeta->prediction->bbox.width = video_info->width;
    imeta->prediction->bbox.height = video_info->height;
  }

  return TRUE;
}

static GstMeta *
video_inference_transform_meta (GstBuffer * buffer_model,
    GstVideoInfo * info_model, GstMeta * meta_model, GstBuffer * buffer_bypass,
    GstVideoInfo * info_bypass)
{
  GstMeta *meta_bypass = NULL;
  const GstMetaInfo *info;
  GstVideoMetaTransform trans = { info_model, info_bypass };

  g_return_val_if_fail (buffer_model, NULL);
  g_return_val_if_fail (info_model, NULL);
  g_return_val_if_fail (meta_model, NULL);

  if (NULL == info_bypass || NULL == buffer_bypass) {
    return meta_bypass;
  }

  info = meta_model->info;

  /* TODO: elements such as videoscale will drop metas that have tags
     such as video-size. Until this is fixed, tread all
     transformations as scaling transformations
   */
  info->transform_func (buffer_bypass, meta_model, buffer_model,
      _scale_quark, &trans);
  meta_bypass = gst_buffer_get_meta (buffer_bypass, info->api);

  return meta_bypass;
}

static GstFlowReturn
gst_video_inference_process_model (GstVideoInference * self, GstBuffer * buffer,
    GstVideoInferencePad * pad)
{
  GstVideoInferencePrivate *priv = GST_VIDEO_INFERENCE_PRIVATE (self);
  GstVideoInferenceClass *klass = GST_VIDEO_INFERENCE_GET_CLASS (self);
  GstFlowReturn ret = GST_FLOW_OK;
  GstMeta *current_meta = NULL;
  GstMeta *meta_model[2] = { NULL };
  GstVideoInfo *info_model = NULL;
  GstBuffer *buffer_model = NULL;
  gpointer prediction_data = NULL;
  gsize prediction_size;
  gboolean pred_valid = FALSE;

  g_return_val_if_fail (self != NULL, GST_FLOW_ERROR);
  g_return_val_if_fail (buffer != NULL, GST_FLOW_ERROR);
  g_return_val_if_fail (pad != NULL, GST_FLOW_ERROR);

  GST_LOG_OBJECT (self, "Processing model buffer");

  if (NULL == klass->postprocess) {
    GST_ELEMENT_ERROR (self, STREAM, FAILED,
        ("Subclass didn't implement post-process"), (NULL));
    ret = GST_FLOW_ERROR;
    goto buffer_free;
  }

  buffer_model = gst_buffer_make_writable (buffer);
  current_meta =
      gst_buffer_get_meta (buffer_model, gst_inference_meta_api_get_type ());
  if (current_meta) {
    /* Check if root is enabled to be processed, if not, just forward buffer */
    GstInferenceMeta *inference_meta = (GstInferenceMeta *) current_meta;
    GstInferencePrediction *root = inference_meta->prediction;
    if (!root->enabled) {
      GST_INFO_OBJECT (self,
          "Current Prediction is not enabled, bypassing processing...");
      goto forward_buffer;
    }
  }

  /* Run preprocess and inference on the model and generate prediction */
  if (!gst_video_inference_model_run_prediction (self, klass, priv,
          buffer_model, &prediction_data, &prediction_size)) {
    ret = GST_FLOW_ERROR;
    goto buffer_free;
  }

  /* Assign already created inferencemeta, no need to create a new one */
  meta_model[1] = current_meta;

  /* Prepare postprocess */
  info_model = &(pad->info);
  if (!video_inference_prepare_postprocess (klass->inference_meta_info,
          buffer_model, info_model, meta_model)) {
    ret = GST_FLOW_ERROR;
    goto prediction_free;
  }

  /* Subclass Processing */
  if (!klass->postprocess (self, prediction_data, prediction_size,
          meta_model, info_model, &pred_valid, priv->labels_list,
          priv->num_labels)) {
    GST_ELEMENT_ERROR (self, STREAM, FAILED, ("Subclass failed at preprocess"),
        (NULL));
    ret = GST_FLOW_ERROR;
    goto prediction_free;
  }

  /* Check if bypass pad was requested, if not, forward buffer */
  if (NULL == priv->sink_bypass) {
    GST_LOG_OBJECT (self,
        "There is no sinkpad for bypass, forwarding model buffer...");
    goto forward_buffer;
  } else {
    /* Queue buffer */
    GST_LOG_OBJECT (self, "Queue model buffer");
    g_mutex_lock (&priv->mtx_model_queue);
    g_queue_push_head (priv->model_queue, (gpointer) buffer_model);
    g_mutex_unlock (&priv->mtx_model_queue);
    goto out;
  }

forward_buffer:
  ret = gst_video_inference_forward_buffer (self, gst_buffer_ref (buffer_model),
      priv->src_model);

prediction_free:
  g_free (prediction_data);

buffer_free:
  gst_buffer_unref (buffer_model);

out:
  return ret;
}

static gboolean
gst_inference_is_prediction_enabled (GNode * node, gpointer data)
{
  GstInferencePrediction *root = (GstInferencePrediction *) node->data;
  gboolean *enabled;

  g_return_val_if_fail (root != NULL, TRUE);
  g_return_val_if_fail (data != NULL, TRUE);

  enabled = (gboolean *) data;

  if (root->enabled) {
    /* This prediction is enabled, mark enabled flag and terminate traverse */
    *enabled = TRUE;
    return TRUE;
  }
  return FALSE;
}

static void
video_inference_notify (GstVideoInference * self, GstBuffer * model_buffer,
    GstMeta * meta_model[2], GstBuffer * bypass_buffer,
    GstMeta * meta_bypass[2])
{
  GstVideoInferencePrivate *priv = GST_VIDEO_INFERENCE_PRIVATE (self);
  GstVideoFrame frame_model;
  GstVideoFrame frame_bypass;
  GstVideoInfo *info_model = NULL;
  GstVideoInfo *info_bypass = NULL;
  GstMapFlags flags;

  g_return_if_fail (model_buffer);
  g_return_if_fail (meta_model);

  info_model = &(priv->sink_model_data->info);
  info_bypass = &(priv->sink_bypass_data->info);

  flags = (GstMapFlags) (GST_MAP_READ | GST_VIDEO_FRAME_MAP_FLAG_NO_REF);
  gst_video_frame_map (&frame_model, info_model, model_buffer, flags);
  gst_video_frame_map (&frame_bypass, info_bypass, bypass_buffer, flags);

  /* Emit inference signal */
  g_signal_emit (self, gst_video_inference_signals[NEW_PREDICTION_SIGNAL], 0,
      meta_model[0], &frame_model, meta_bypass[0], &frame_bypass);
  g_signal_emit (self, gst_video_inference_signals[NEW_INFERENCE_SIGNAL], 0,
      meta_model[1], &frame_model, meta_bypass[1], &frame_bypass);

  gst_video_frame_unmap (&frame_model);
  gst_video_frame_unmap (&frame_bypass);
}

static GstFlowReturn
gst_video_inference_process_bypass (GstVideoInference * self,
    GstBuffer * buffer, GstVideoInferencePad * pad)
{
  GstVideoInferencePrivate *priv = GST_VIDEO_INFERENCE_PRIVATE (self);
  GstVideoInferenceClass *klass = GST_VIDEO_INFERENCE_GET_CLASS (self);
  GstFlowReturn ret = GST_FLOW_OK;
  GstMeta *current_meta = NULL;
  GstBuffer *bypass_buffer = NULL;
  GstBuffer *model_buffer = NULL;
  gboolean model_empty = FALSE;

  g_return_val_if_fail (self != NULL, GST_FLOW_ERROR);
  g_return_val_if_fail (buffer != NULL, GST_FLOW_ERROR);
  g_return_val_if_fail (pad != NULL, GST_FLOW_ERROR);

  GST_LOG_OBJECT (self, "Processing bypass buffer");

  /* Check if model pad was requested, if not, forward buffer */
  if (NULL == priv->sink_model) {
    GST_LOG_OBJECT (self,
        "There is no sinkpad for model, forwarding bypass buffer...");
    goto forward_buffer;
  }

  bypass_buffer = gst_buffer_make_writable (buffer);
  current_meta = gst_buffer_get_meta (bypass_buffer,
      gst_inference_meta_api_get_type ());
  if (current_meta) {
    /* Check if at least one node is enabled to be processed, if not, just forward buffer */
    GstInferenceMeta *imeta = (GstInferenceMeta *) current_meta;
    gboolean enabled = FALSE;

    g_node_traverse (imeta->prediction->predictions, G_LEVEL_ORDER,
        G_TRAVERSE_ALL, -1, gst_inference_is_prediction_enabled, &enabled);
    if (!enabled) {
      GST_INFO_OBJECT (self,
          "There is no predictions enabled, bypassing processing...");
      goto forward_buffer;
    }
  }

  /* Queue this new buffer at the head and dequeue the older one */
  GST_LOG_OBJECT (self, "Queue bypass buffer and get older one");
  g_mutex_lock (&priv->mtx_bypass_queue);
  g_queue_push_head (priv->bypass_queue, (gpointer) bypass_buffer);
  bypass_buffer = (GstBuffer *) g_queue_pop_tail (priv->bypass_queue);
  g_mutex_unlock (&priv->mtx_bypass_queue);

  while (!model_empty) {
    /* Dequeue oldest model buffer from tail */
    g_mutex_lock (&priv->mtx_model_queue);
    GST_LOG_OBJECT (self, "Dequeue model buffer");
    model_buffer = (GstBuffer *) g_queue_pop_tail (priv->model_queue);
    g_mutex_unlock (&priv->mtx_model_queue);

    if (NULL == model_buffer) {
      /* Model queue is empty */
      model_empty = TRUE;
    } else {
      GstInferencePrediction *root_model = NULL;
      GstInferencePrediction *root_bypass = NULL;
      GstMeta *meta_model[2] = { NULL };
      GstMeta *meta_bypass[2] = { NULL };
      GstVideoInfo *info_model = &(priv->sink_model_data->info);
      GstVideoInfo *info_bypass = &(priv->sink_bypass_data->info);

      /* Get both metas from model buffer */
      meta_model[0] = gst_buffer_get_meta (model_buffer,
          klass->inference_meta_info->api);
      meta_model[1] = gst_buffer_get_meta (model_buffer,
          gst_inference_meta_api_get_type ());

      root_model = ((GstInferenceMeta *) meta_model[1])->prediction;

      /* If bypass doesn't have meta, just transfer the model meta */
      current_meta = gst_buffer_get_meta (bypass_buffer,
          gst_inference_meta_api_get_type ());

      if (current_meta) {
        /* Check if model and bypass IDs match */
        GST_LOG_OBJECT (self, "Checking if model and bypass IDs match");
        root_bypass = gst_inference_prediction_find (((GstInferenceMeta *)
                current_meta)->prediction, root_model->prediction_id);
        if (NULL == root_bypass) {
          /* Queue model buffer to tail again */
          g_mutex_lock (&priv->mtx_model_queue);
          GST_LOG_OBJECT (self, "Queue model buffer");
          g_queue_push_tail (priv->model_queue, (gpointer) model_buffer);
          g_mutex_unlock (&priv->mtx_model_queue);
          goto forward_buffer;
        }
      }

      /* Transfer meta from model to bypass */
      GST_LOG_OBJECT (self, "Transfering meta from model to bypass");

      meta_bypass[0] =
          video_inference_transform_meta (model_buffer, info_model,
          (GstMeta *) meta_model[0], bypass_buffer, info_bypass);
      meta_bypass[1] =
          video_inference_transform_meta (model_buffer, info_model,
          (GstMeta *) meta_model[1], bypass_buffer, info_bypass);

      /* Notify prediction */
      video_inference_notify (self, model_buffer, meta_model, bypass_buffer,
          meta_bypass);

      /* Forward buffer to model src pad */
      GST_LOG_OBJECT (self, "Forward model buffer");
      ret = gst_video_inference_forward_buffer (self, model_buffer,
          priv->src_model);
      model_buffer = NULL;
    }
  }

  /* Queue old bypass buffer at the tail */
  GST_LOG_OBJECT (self, "Queue bypass buffer");
  g_mutex_lock (&priv->mtx_bypass_queue);
  g_queue_push_tail (priv->bypass_queue, (gpointer) bypass_buffer);
  g_mutex_unlock (&priv->mtx_bypass_queue);

  return ret;

forward_buffer:
  /* Forward buffer to bypass src pad */
  GST_LOG_OBJECT (self, "Forward bypass buffer");
  ret =
      gst_video_inference_forward_buffer (self, bypass_buffer,
      priv->src_bypass);

  return ret;
}

static GstFlowReturn
gst_video_inference_buffer_function (GstCollectPads * pads,
    GstCollectData * data, GstBuffer * buffer, gpointer user_data)
{
  GstVideoInference *self = (GstVideoInference *) user_data;
  GstVideoInferencePrivate *priv = NULL;
  GstVideoInferencePad *pad = (GstVideoInferencePad *) data;
  GstFlowReturn ret = GST_FLOW_OK;

  if (!buffer) {
    /* EOS reached the pad */
    ret = GST_FLOW_EOS;
    goto out;
  }

  g_return_val_if_fail (pads != NULL, GST_FLOW_ERROR);
  g_return_val_if_fail (self != NULL, GST_FLOW_ERROR);

  priv = GST_VIDEO_INFERENCE_PRIVATE (self);

  if (data->pad == priv->sink_model) {
    GST_LOG_OBJECT (self, "Model buffer arrived, processing it...");
    ret = gst_video_inference_process_model (self, buffer, pad);
    goto out;
  } else {
    GST_LOG_OBJECT (self, "Bypass buffer arrived, processing it...");
    ret = gst_video_inference_process_bypass (self, buffer, pad);
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

static GstPad *
gst_video_inference_get_sink_pad (GstVideoInference * self,
    GstVideoInferencePrivate * priv, GstPad * srcpad)
{
  GstPad *pad;

  if (srcpad == priv->src_model) {
    pad = priv->sink_model;
  } else if (srcpad == priv->src_bypass) {
    pad = priv->sink_bypass;
  } else {
    g_return_val_if_reached (NULL);
  }

  return pad;
}

static void
gst_video_inference_set_caps (GstVideoInference * self,
    GstVideoInferencePrivate * priv, GstCollectData * data, GstEvent * event)
{
  GstCaps *caps;
  GstVideoInferencePad *cpad;

  g_return_if_fail (self);
  g_return_if_fail (priv);
  g_return_if_fail (data);
  g_return_if_fail (event);

  cpad = (GstVideoInferencePad *) data;

  gst_event_parse_caps (event, &caps);

  if (gst_caps_is_fixed (caps)) {
    GstVideoInfo *info = &(cpad->info);

    GST_INFO_OBJECT (self,
        "Updating caps in %" GST_PTR_FORMAT " to %" GST_PTR_FORMAT, data->pad,
        caps);
    gst_video_info_init (info);
    gst_video_info_from_caps (info, caps);
  }
}

static GstIterator *
gst_video_inference_iterate_internal_links (GstPad * pad, GstObject * parent)
{
  GstIterator *it = NULL;
  GstPad *opad;
  GValue val = { 0, };
  GstVideoInference *self = GST_VIDEO_INFERENCE (parent);
  GstVideoInferencePrivate *priv = GST_VIDEO_INFERENCE_PRIVATE (self);

  GST_LOG_OBJECT (self, "Internal links");

  GST_OBJECT_LOCK (parent);

  if (GST_PAD_IS_SINK (pad)) {
    opad = gst_video_inference_get_src_pad (self, priv, pad);
    if (opad == NULL) {
      goto out;
    }
  } else {
    opad = gst_video_inference_get_sink_pad (self, priv, pad);
    if (opad == NULL) {
      goto out;
    }
  }

  g_value_init (&val, GST_TYPE_PAD);
  g_value_set_object (&val, opad);
  it = gst_iterator_new_single (GST_TYPE_PAD, &val);
  g_value_unset (&val);

out:
  GST_OBJECT_UNLOCK (parent);

  return it;
}

static gboolean
gst_video_inference_sink_event (GstCollectPads * pads, GstCollectData * pad,
    GstEvent * event, gpointer user_data)
{
  GstVideoInference *self = GST_VIDEO_INFERENCE (user_data);
  GstVideoInferencePrivate *priv = GST_VIDEO_INFERENCE_PRIVATE (self);
  gboolean ret = FALSE;
  GstPad *srcpad;

  GST_LOG_OBJECT (self, "Received event %s from %" GST_PTR_FORMAT,
      GST_EVENT_TYPE_NAME (event), pad->pad);

  srcpad = gst_video_inference_get_src_pad (self, priv, pad->pad);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
      gst_video_inference_set_caps (self, priv, pad, event);
      break;
    default:
      break;
  }

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
  g_free (priv->labels);
  priv->labels = NULL;
  g_free (priv->labels_list);
  priv->labels_list = NULL;

  g_mutex_clear (&priv->mtx_model_queue);
  g_mutex_clear (&priv->mtx_bypass_queue);

  g_queue_free (priv->model_queue);
  g_queue_free (priv->bypass_queue);

  g_clear_object (&priv->backend);

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
