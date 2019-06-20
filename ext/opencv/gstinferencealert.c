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

#include "gstinferencealert.h"
#include "gst/r2inference/gstinferencemeta.h"

/* pad templates */

#define VIDEO_SRC_CAPS \
    GST_VIDEO_CAPS_MAKE("{RGB, RGBx, RGBA, BGR, BGRx, BGRA, xRGB, ARGB, xBGR, ABGR}")

#define VIDEO_SINK_CAPS \
    GST_VIDEO_CAPS_MAKE("{RGB, RGBx, RGBA, BGR, BGRx, BGRA, xRGB, ARGB, xBGR, ABGR}")

GST_DEBUG_CATEGORY_STATIC (gst_inference_alert_debug_category);
#define GST_CAT_DEFAULT gst_inference_alert_debug_category

#define MIN_LABEL_INDEX 0
#define DEFAULT_LABEL_INDEX 0
#define MAX_LABEL_INDEX G_MAXUINT

enum
{
  PROP_0,
  PROP_LABEL_INDEX
};

enum
{
  ALERT_SIGNAL,
  LAST_SIGNAL
};

struct _GstInferenceAlert
{
  GstVideoFilter parent;
};

struct _GstInferenceAlertClass
{
  GstVideoFilter parent;
};

typedef struct _GstInferenceAlertPrivate GstInferenceAlertPrivate;
struct _GstInferenceAlertPrivate
{
  gint label_index;
};

/* prototypes */
static void gst_inference_alert_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_inference_alert_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);

static GstFlowReturn
gst_inference_alert_transform_frame_ip (GstVideoFilter * trans,
    GstVideoFrame * frame);

static guint gst_inference_alert_signals[LAST_SIGNAL] = { 0 };

/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstInferenceAlert, gst_inference_alert,
    GST_TYPE_VIDEO_FILTER,
    GST_DEBUG_CATEGORY_INIT (gst_inference_alert_debug_category,
        "inferencealert", 0, "debug category for inferencealert class");
    G_ADD_PRIVATE (GstInferenceAlert));

#define GST_INFERENCE_ALERT_PRIVATE(self) \
  (GstInferenceAlertPrivate *)(gst_inference_alert_get_instance_private (self))

static void
gst_inference_alert_class_init (GstInferenceAlertClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstVideoFilterClass *video_filter_class = GST_VIDEO_FILTER_CLASS (klass);

  gst_element_class_add_pad_template (GST_ELEMENT_CLASS (klass),
      gst_pad_template_new ("src", GST_PAD_SRC, GST_PAD_ALWAYS,
          gst_caps_from_string (VIDEO_SRC_CAPS)));
  gst_element_class_add_pad_template (GST_ELEMENT_CLASS (klass),
      gst_pad_template_new ("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
          gst_caps_from_string (VIDEO_SINK_CAPS)));

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "inferencealert", "Filter",
      "Signals an alert if the metadata matches the configured parameters",
      "Miguel Taylor <miguel.taylor@ridgerun.com> \n\t\t\t");

  gobject_class->set_property = gst_inference_alert_set_property;
  gobject_class->get_property = gst_inference_alert_get_property;

  g_object_class_install_property (gobject_class, PROP_LABEL_INDEX,
      g_param_spec_uint ("label-index", "label",
          "Label index to generate the alert", MIN_LABEL_INDEX, MAX_LABEL_INDEX,
          DEFAULT_LABEL_INDEX, G_PARAM_READWRITE));

  gst_inference_alert_signals[ALERT_SIGNAL] =
      g_signal_new ("alert", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_FIRST, 0,
      NULL, NULL, NULL, G_TYPE_NONE, 0);

  video_filter_class->transform_frame_ip =
      GST_DEBUG_FUNCPTR (gst_inference_alert_transform_frame_ip);

}

static void
gst_inference_alert_init (GstInferenceAlert * inference_alert)
{
  GstInferenceAlertPrivate *priv =
      GST_INFERENCE_ALERT_PRIVATE (inference_alert);
  priv->label_index = DEFAULT_LABEL_INDEX;
}

void
gst_inference_alert_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstInferenceAlert *inference_alert = GST_INFERENCE_ALERT (object);
  GstInferenceAlertPrivate *priv =
      GST_INFERENCE_ALERT_PRIVATE (inference_alert);

  GST_DEBUG_OBJECT (inference_alert, "set_property");

  switch (property_id) {
    case PROP_LABEL_INDEX:
      priv->label_index = g_value_get_uint (value);
      GST_DEBUG_OBJECT (inference_alert, "Changed label index to to %d",
          priv->label_index);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_inference_alert_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstInferenceAlert *inference_alert = GST_INFERENCE_ALERT (object);
  GstInferenceAlertPrivate *priv =
      GST_INFERENCE_ALERT_PRIVATE (inference_alert);

  GST_DEBUG_OBJECT (inference_alert, "get_property");

  switch (property_id) {
    case PROP_LABEL_INDEX:
      g_value_set_uint (value, priv->label_index);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

/* transform */
static GstFlowReturn
gst_inference_alert_transform_frame_ip (GstVideoFilter * trans,
    GstVideoFrame * frame)
{
  GstInferenceAlert *inference_alert = GST_INFERENCE_ALERT (trans);
  GstInferenceAlertPrivate *priv =
      GST_INFERENCE_ALERT_PRIVATE (inference_alert);
  GstDetectionMeta *detect_meta;
  GstClassificationMeta *class_meta;
  GstMeta *meta;
  BBox box;
  gdouble max, current;
  gint i, index;

  meta = gst_buffer_get_meta (frame->buffer, GST_DETECTION_META_API_TYPE);
  if (NULL == meta) {
    GST_LOG_OBJECT (trans, "No detection meta found");
  } else {
    GST_LOG_OBJECT (trans, "Valid detection meta found");
    detect_meta = (GstDetectionMeta *) meta;
    for (i = 0; i < detect_meta->num_boxes; ++i) {
      box = detect_meta->boxes[i];
      if (priv->label_index == box.label) {
        g_signal_emit (inference_alert,
            gst_inference_alert_signals[ALERT_SIGNAL], 0);
        break;
      }
    }
  }

  meta = gst_buffer_get_meta (frame->buffer, GST_CLASSIFICATION_META_API_TYPE);
  if (NULL == meta) {
    GST_LOG_OBJECT (trans, "No classification meta found");
  } else {
    GST_LOG_OBJECT (trans, "Valid classification meta found");
    class_meta = (GstClassificationMeta *) meta;

    index = 0;
    max = -1;
    for (i = 0; i < class_meta->num_labels; ++i) {
      current = class_meta->label_probs[i];
      if (current > max) {
        max = current;
        index = i;
      }
    }
    if (priv->label_index == index) {
      g_signal_emit (inference_alert, gst_inference_alert_signals[ALERT_SIGNAL],
          0);
    }
  }

  return GST_FLOW_OK;
}
