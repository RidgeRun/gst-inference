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

/**
 * SECTION:element-gstinferencedebug
 *
 * The inferencedebug element prints the inferencemeta predictions tree
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 v4l2src device=$CAMERA ! "video/x-raw, width=1280, height=720" ! videoconvert ! tee name=t \
   t. ! videoscale ! queue ! net.sink_model t. ! queue ! net.sink_bypass \
   tinyyolov2 name=net model-location=$MODEL_LOCATION backend=tensorflow backend::input-layer=$INPUT_LAYER \
   backend::output-layer=$OUTPUT_LAYER  net.src_model ! inferencedebug ! fakesink
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstinferencedebug.h"

#include <gst/r2inference/gstinferencemeta.h>
#include <gst/base/gstbasetransform.h>


GST_DEBUG_CATEGORY_STATIC (gst_inferencedebug_debug_category);
#define GST_CAT_DEFAULT gst_inferencedebug_debug_category

/* prototypes */

static void gst_inferencedebug_print_predictions (GstInferencedebug *
    inferencedebug, GstInferencePrediction * root);
static GstFlowReturn gst_inferencedebug_transform_ip (GstBaseTransform * trans,
    GstBuffer * buf);

struct _GstInferencedebug
{
  GstBaseTransform base_inferencedebug;
};


/* pad templates */

static GstStaticPadTemplate gst_inferencedebug_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate gst_inferencedebug_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);


/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstInferencedebug, gst_inferencedebug,
    GST_TYPE_BASE_TRANSFORM,
    GST_DEBUG_CATEGORY_INIT (gst_inferencedebug_debug_category,
        "inferencedebug", 0, "debug category for inferencedebug element"));

static void
gst_inferencedebug_class_init (GstInferencedebugClass * klass)
{
  GstBaseTransformClass *base_transform_class =
      GST_BASE_TRANSFORM_CLASS (klass);

  /* Setting up pads and setting metadata should be moved to
     base_class_init if you intend to subclass this class. */
  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS (klass),
      &gst_inferencedebug_src_template);
  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS (klass),
      &gst_inferencedebug_sink_template);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "Inference Debug", "Generic",
      "Prints InferenceMeta Predictions Tree",
      "<carlos.rodriguez@ridgerun.com>");

  base_transform_class->transform_ip =
      GST_DEBUG_FUNCPTR (gst_inferencedebug_transform_ip);
}

static void
gst_inferencedebug_init (GstInferencedebug * inferencedebug)
{
}


static void
gst_inferencedebug_print_predictions (GstInferencedebug * inferencedebug,
    GstInferencePrediction * root)
{
  gchar *prediction_tree = NULL;

  g_return_if_fail (inferencedebug);
  g_return_if_fail (root);

  prediction_tree = gst_inference_prediction_to_string (root);

  GST_DEBUG_OBJECT (inferencedebug, "Prediction Tree: \n %s", prediction_tree);

  g_free (prediction_tree);
}

static GstFlowReturn
gst_inferencedebug_transform_ip (GstBaseTransform * trans, GstBuffer * buf)
{
  GstInferencedebug *inferencedebug = GST_INFERENCEDEBUG (trans);
  GstInferenceMeta *meta;

  GST_DEBUG_OBJECT (inferencedebug, "transform_ip");

  meta = (GstInferenceMeta *) gst_buffer_get_meta (buf,
      GST_INFERENCE_META_API_TYPE);

  if (NULL == meta) {
    GST_LOG_OBJECT (inferencedebug,
        "No inference meta found. Buffer passthrough.");
    return GST_FLOW_OK;
  }

  g_return_val_if_fail (meta->prediction, GST_FLOW_ERROR);

  gst_inferencedebug_print_predictions (inferencedebug, meta->prediction);

  return GST_FLOW_OK;
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "inferencedebug", GST_RANK_NONE,
      GST_TYPE_INFERENCEDEBUG);
}


GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    inferencedebug,
    "Print InferenceMeta Predictions",
    plugin_init, VERSION, "LGPL", PACKAGE_NAME, GST_PACKAGE_ORIGIN)
