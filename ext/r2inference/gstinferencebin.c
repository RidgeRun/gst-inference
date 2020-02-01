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
 * SECTION:element-gstinferencebin
 *
 * Helper element that simplifies inference by creating a bin with the
 * required elements in the typical inference configuration.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 v4l2src device=$CAMERA ! inferencebin architecture=tinyyolov2 model-location=$MODEL_LOCATION ! \
 * backend=tensorflow architecture::backend::input-layer=$INPUT_LAYER architecture::backend::output-layer=OUTPUT_LAYER ! \
 * videoconvert ! ximagesink sync=false
 * ]|
 * Detect object in a camera stream
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstinferencebin.h"

/* generic templates */
static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

GST_DEBUG_CATEGORY_STATIC (gst_inference_bin_debug_category);
#define GST_CAT_DEFAULT gst_inference_bin_debug_category

static void gst_inference_bin_finalize (GObject * object);
static void gst_inference_bin_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_inference_bin_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static GstStateChangeReturn gst_inference_bin_change_state (GstElement *
    element, GstStateChange transition);

static gboolean gst_inference_bin_start (GstInferenceBin * self);
static gboolean gst_inference_bin_stop (GstInferenceBin * self);


enum
{
  PROP_0,
};

struct _GstInferenceBin
{
  GstBin parent;
};

struct _GstInferenceBinClass
{
  GstBinClass parent;
};

/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstInferenceBin, gst_inference_bin,
    GST_TYPE_BIN,
    GST_DEBUG_CATEGORY_INIT (gst_inference_bin_debug_category, "inferencebin",
        0, "debug category for inferencebin element"));

static void
gst_inference_bin_class_init (GstInferenceBinClass * klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&src_template));

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "inferencebin", "Filter",
      "A bin with the inference element in their typical configuration",
      "Michael Gruner <michael.gruner@ridgerun.com>");

  element_class->change_state =
      GST_DEBUG_FUNCPTR (gst_inference_bin_change_state);

  object_class->finalize = gst_inference_bin_finalize;
  object_class->set_property = gst_inference_bin_set_property;
  object_class->get_property = gst_inference_bin_get_property;
}

static void
gst_inference_bin_init (GstInferenceBin * self)
{

}

static void
gst_inference_bin_finalize (GObject * object)
{
  G_OBJECT_CLASS (gst_inference_bin_parent_class)->finalize (object);
}

static void
gst_inference_bin_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstInferenceBin *self = GST_INFERENCE_BIN (object);

  GST_LOG_OBJECT (self, "set_property");

  GST_OBJECT_LOCK (self);

  switch (property_id) {
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }

  GST_OBJECT_UNLOCK (self);
}

static void
gst_inference_bin_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstInferenceBin *self = GST_INFERENCE_BIN (object);

  GST_LOG_OBJECT (self, "get_property");

  GST_OBJECT_LOCK (self);

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }

  GST_OBJECT_UNLOCK (self);
}

static gboolean
gst_inference_bin_start (GstInferenceBin * self)
{
  g_return_val_if_fail (self, FALSE);

  return TRUE;
}

static gboolean
gst_inference_bin_stop (GstInferenceBin * self)
{
  g_return_val_if_fail (self, FALSE);

  return TRUE;
}

static GstStateChangeReturn
gst_inference_bin_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstInferenceBin *self = GST_INFERENCE_BIN (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      if (FALSE == gst_inference_bin_start (self)) {
        GST_ERROR_OBJECT (self, "Failed to start");
        ret = GST_STATE_CHANGE_FAILURE;
        goto out;
      }
    default:
      break;
  }

  ret =
      GST_ELEMENT_CLASS (gst_inference_bin_parent_class)->change_state
      (element, transition);
  if (GST_STATE_CHANGE_FAILURE == ret) {
    GST_ERROR_OBJECT (self, "Parent failed to change state");
    goto out;
  }

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      if (FALSE == gst_inference_bin_stop (self)) {
        GST_ERROR_OBJECT (self, "Failed to stop");
        ret = GST_STATE_CHANGE_FAILURE;
        goto out;
      }
    default:
      break;
  }

out:
  return ret;
}
