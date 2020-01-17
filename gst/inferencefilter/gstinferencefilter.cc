/*
 * GStreamer
 * Copyright (C) 2019 RidgeRun
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

/**
 * SECTION:element-gstinferencefilter
 *
 * The inferencefilter element modifies metadata on input buffer to select
 * which prediction on the inference meta should be processed by modifying
 * its enable property.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 v4l2src device=$CAMERA ! "video/x-raw, width=1280, height=720" ! videoconvert ! tee name=t \
   t. ! videoscale ! queue ! net.sink_model t. ! queue ! net.sink_bypass \
   tinyyolov2 name=net model-location=$MODEL_LOCATION backend=tensorflow backend::input-layer=$INPUT_LAYER \
   backend::output-layer=$OUTPUT_LAYER  net.src_model ! inferencefilter filter-class=1 ! fakesink
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstinferencefilter.h"

#include <gst/r2inference/gstinferencemeta.h>
#include <gst/base/gstbasetransform.h>


GST_DEBUG_CATEGORY_STATIC (gst_inferencefilter_debug_category);
#define GST_CAT_DEFAULT gst_inferencefilter_debug_category

#define GST_INFERENCEFILTER_PROPERTY_FLAGS (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
#define PROP_FILTER_CLASS_LABEL_DEFAULT -1
#define PROP_FILTER_CLASS_LABEL_MIN -1
#define PROP_RESET_ENABLE_DEFAULT FALSE

/* prototypes */

static void gst_inferencefilter_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_inferencefilter_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_inferencefilter_filter_enable (GstInferencefilter *
    inferencefilter, GstInferencePrediction * rot, gint class_id, gboolean reset);
static GstFlowReturn gst_inferencefilter_transform_ip (GstBaseTransform * trans,
    GstBuffer * buf);

enum
{
  PROP_0,
  PROP_FILTER_CLASS_LABEL,
  PROP_RESET_ENABLE,
};

struct _GstInferencefilter
{
  GstBaseTransform base_inferencefilter;
  gint filter_class;
  gboolean reset_enable;
};


/* pad templates */

static GstStaticPadTemplate gst_inferencefilter_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate gst_inferencefilter_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);


/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstInferencefilter, gst_inferencefilter,
    GST_TYPE_BASE_TRANSFORM,
    GST_DEBUG_CATEGORY_INIT (gst_inferencefilter_debug_category,
        "inferencefilter", 0, "debug category for inferencefilter element"));

static void
gst_inferencefilter_class_init (GstInferencefilterClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseTransformClass *base_transform_class =
      GST_BASE_TRANSFORM_CLASS (klass);

  /* Setting up pads and setting metadata should be moved to
     base_class_init if you intend to subclass this class. */
  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS (klass),
      &gst_inferencefilter_src_template);
  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS (klass),
      &gst_inferencefilter_sink_template);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "Inference Filter", "Generic",
      "Enables/disables specific classes contained on the inference metadata to be processed",
      "<carolina.trejos@ridgerun.com>");

  gobject_class->set_property = gst_inferencefilter_set_property;
  gobject_class->get_property = gst_inferencefilter_get_property;

  g_object_class_install_property (gobject_class, PROP_FILTER_CLASS_LABEL,
      g_param_spec_int ("filter-class", "filter-class", "Filter class",
          PROP_FILTER_CLASS_LABEL_MIN, G_MAXINT,
          PROP_FILTER_CLASS_LABEL_DEFAULT, GST_INFERENCEFILTER_PROPERTY_FLAGS));
  g_object_class_install_property (gobject_class, PROP_RESET_ENABLE,
      g_param_spec_boolean ("reset-enable", "Reset enable",
          "Enables all inference meta to be processed",
          PROP_RESET_ENABLE_DEFAULT, GST_INFERENCEFILTER_PROPERTY_FLAGS));

  base_transform_class->transform_ip =
      GST_DEBUG_FUNCPTR (gst_inferencefilter_transform_ip);

}

static void
gst_inferencefilter_init (GstInferencefilter * inferencefilter)
{
  inferencefilter->filter_class = -1;
  inferencefilter->reset_enable = FALSE;
}

void
gst_inferencefilter_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstInferencefilter *inferencefilter = GST_INFERENCEFILTER (object);

  GST_DEBUG_OBJECT (inferencefilter, "set_property");

  switch (property_id) {
    case PROP_FILTER_CLASS_LABEL:
      GST_OBJECT_LOCK (inferencefilter);
      inferencefilter->filter_class = g_value_get_int (value);
      GST_OBJECT_UNLOCK (inferencefilter);
      break;
    case PROP_RESET_ENABLE:
      GST_OBJECT_LOCK (inferencefilter);
      inferencefilter->reset_enable = g_value_get_boolean (value);
      GST_OBJECT_UNLOCK (inferencefilter);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_inferencefilter_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstInferencefilter *inferencefilter = GST_INFERENCEFILTER (object);

  GST_DEBUG_OBJECT (inferencefilter, "get_property");

  switch (property_id) {
    case PROP_FILTER_CLASS_LABEL:
      GST_OBJECT_LOCK (inferencefilter);
      g_value_set_int (value, inferencefilter->filter_class);
      GST_OBJECT_UNLOCK (inferencefilter);
      break;
    case PROP_RESET_ENABLE:
      GST_OBJECT_LOCK (inferencefilter);
      g_value_set_boolean (value, inferencefilter->reset_enable);
      GST_OBJECT_UNLOCK (inferencefilter);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
gst_inferencefilter_filter_enable (GstInferencefilter * inferencefilter,
    GstInferencePrediction * root, gint class_id, gboolean reset)
{
  GList *iter = NULL;
  GSList *iter_child = NULL;

  g_return_if_fail(inferencefilter);
  g_return_if_fail(root);

  for (iter = root->classifications; iter != NULL; iter = g_list_next (iter)) {
    GstInferenceClassification *classification =
        (GstInferenceClassification *) iter->data;
    if (classification->class_id == class_id || reset) {
      GST_DEBUG_OBJECT (inferencefilter, "Enabling classification id %d",
          classification->class_id);
      root->enabled = TRUE;
      break;
    } else {
      GST_DEBUG_OBJECT (inferencefilter, "Disabling classification id %d",
          classification->class_id);
      root->enabled = FALSE;
    }
  }

  for (iter_child = gst_inference_prediction_get_children (root); iter_child != NULL; iter_child = g_slist_next (iter_child)) {
    GstInferencePrediction *predict =
        (GstInferencePrediction *) iter_child->data;
    gst_inferencefilter_filter_enable (inferencefilter, predict, class_id, reset);
  }
}

static GstFlowReturn
gst_inferencefilter_transform_ip (GstBaseTransform * trans, GstBuffer * buf)
{
  GstInferencefilter *inferencefilter = GST_INFERENCEFILTER (trans);
  gboolean reset = FALSE;
  gint filter = -1;
  GstInferenceMeta *meta;
  GST_DEBUG_OBJECT (inferencefilter, "transform_ip");

  meta = (GstInferenceMeta *) gst_buffer_get_meta (buf,
      GST_INFERENCE_META_API_TYPE);

  if (NULL == meta) {
    GST_LOG_OBJECT (inferencefilter,
        "No inference meta found. Buffer passthrough.");
    return GST_FLOW_OK;
  }

  g_return_val_if_fail(meta->prediction, GST_FLOW_ERROR);

  GST_OBJECT_LOCK (inferencefilter);
  reset = inferencefilter->reset_enable;
  filter = inferencefilter->filter_class;
  GST_OBJECT_UNLOCK (inferencefilter);

  if (filter < 0 && !reset) {
    GST_ERROR_OBJECT (inferencefilter, "Invalid filter-class value");
    return GST_FLOW_ERROR;
  } else {
    if (meta->prediction == NULL) {
      GST_LOG_OBJECT (inferencefilter, "Inferece meta has no predictions");
      return GST_FLOW_ERROR;
    }
    gst_inferencefilter_filter_enable (inferencefilter, meta->prediction,
        filter, reset);
    return GST_FLOW_OK;
  }
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "inferencefilter", GST_RANK_NONE,
      GST_TYPE_INFERENCEFILTER);
}


GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    inferencefilter,
    "Enables/disables selected classes on inference meta to be processed",
    plugin_init, VERSION, "LGPL", PACKAGE_NAME, GST_PACKAGE_ORIGIN)
