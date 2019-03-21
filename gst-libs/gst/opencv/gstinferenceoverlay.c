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

#include "gstinferenceoverlay.h"

/* pad templates */

#define VIDEO_SRC_CAPS \
    GST_VIDEO_CAPS_MAKE("{RGB, RGBx, RGBA, BGR, BGRx, BGRA, xRGB, ARGB, xBGR, ABGR}")

#define VIDEO_SINK_CAPS \
    GST_VIDEO_CAPS_MAKE("{RGB, RGBx, RGBA, BGR, BGRx, BGRA, xRGB, ARGB, xBGR, ABGR}")

GST_DEBUG_CATEGORY_STATIC (gst_inference_overlay_debug_category);
#define GST_CAT_DEFAULT gst_inference_overlay_debug_category

#define MIN_FONT_SCALE 0
#define DEFAULT_FONT_SCALE 2
#define MAX_FONT_SCALE 100
#define MIN_THICKNESS 1
#define DEFAULT_THICKNESS 2
#define MAX_THICKNESS 100
#define DEFAULT_LABELS NULL
#define DEFAULT_NUM_LABELS 0

enum
{
  PROP_0,
  PROP_FONT_SCALE,
  PROP_THICKNESS,
  PROP_LABELS
};

typedef struct _GstInferenceOverlayPrivate GstInferenceOverlayPrivate;
struct _GstInferenceOverlayPrivate
{
  gdouble font_scale;
  gint thickness;
  gchar *labels;
  gchar **labels_list;
  gint num_labels;
};
/* prototypes */
static void gst_inference_overlay_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_inference_overlay_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_inference_overlay_dispose (GObject * object);
static void gst_inference_overlay_finalize (GObject * object);

static gboolean gst_inference_overlay_start (GstBaseTransform * trans);
static gboolean gst_inference_overlay_stop (GstBaseTransform * trans);
static GstFlowReturn
gst_inference_overlay_transform_frame_ip (GstVideoFilter * trans,
    GstVideoFrame * frame);

/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstInferenceOverlay, gst_inference_overlay,
    GST_TYPE_VIDEO_FILTER,
    GST_DEBUG_CATEGORY_INIT (gst_inference_overlay_debug_category,
        "inferenceoverlay", 0, "debug category for inferenceoverlay class");
    G_ADD_PRIVATE (GstInferenceOverlay));

#define GST_INFERENCE_OVERLAY_PRIVATE(self) \
  (GstInferenceOverlayPrivate *)(gst_inference_overlay_get_instance_private (self))

static void
gst_inference_overlay_class_init (GstInferenceOverlayClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseTransformClass *base_transform_class =
      GST_BASE_TRANSFORM_CLASS (klass);
  GstVideoFilterClass *video_filter_class = GST_VIDEO_FILTER_CLASS (klass);

  gst_element_class_add_pad_template (GST_ELEMENT_CLASS (klass),
      gst_pad_template_new ("src", GST_PAD_SRC, GST_PAD_ALWAYS,
          gst_caps_from_string (VIDEO_SRC_CAPS)));
  gst_element_class_add_pad_template (GST_ELEMENT_CLASS (klass),
      gst_pad_template_new ("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
          gst_caps_from_string (VIDEO_SINK_CAPS)));

  gobject_class->set_property = gst_inference_overlay_set_property;
  gobject_class->get_property = gst_inference_overlay_get_property;
  gobject_class->dispose = gst_inference_overlay_dispose;
  gobject_class->finalize = gst_inference_overlay_finalize;

  g_object_class_install_property (gobject_class, PROP_FONT_SCALE,
      g_param_spec_double ("font-scale", "font", "Font scale", MIN_FONT_SCALE,
          MAX_FONT_SCALE, DEFAULT_FONT_SCALE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_THICKNESS,
      g_param_spec_int ("thickness", "thickness",
          "Box line thickness in pixels", MIN_THICKNESS, MAX_THICKNESS,
          DEFAULT_THICKNESS, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_LABELS,
      g_param_spec_string ("labels", "labels",
          "Semicolon separated string containing inference labels",
          DEFAULT_LABELS, G_PARAM_READWRITE));

  base_transform_class->start = GST_DEBUG_FUNCPTR (gst_inference_overlay_start);
  base_transform_class->stop = GST_DEBUG_FUNCPTR (gst_inference_overlay_stop);
  video_filter_class->transform_frame_ip =
      GST_DEBUG_FUNCPTR (gst_inference_overlay_transform_frame_ip);

}

static void
gst_inference_overlay_init (GstInferenceOverlay * inference_overlay)
{
  GstInferenceOverlayPrivate *priv =
      GST_INFERENCE_OVERLAY_PRIVATE (inference_overlay);
  priv->font_scale = DEFAULT_FONT_SCALE;
  priv->thickness = DEFAULT_THICKNESS;
  priv->labels = DEFAULT_LABELS;
  priv->labels_list = DEFAULT_LABELS;
  priv->num_labels = DEFAULT_NUM_LABELS;
}

void
gst_inference_overlay_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstInferenceOverlay *inference_overlay = GST_INFERENCE_OVERLAY (object);
  GstInferenceOverlayPrivate *priv =
      GST_INFERENCE_OVERLAY_PRIVATE (inference_overlay);

  GST_DEBUG_OBJECT (inference_overlay, "set_property");

  switch (property_id) {
    case PROP_FONT_SCALE:
      priv->font_scale = g_value_get_double (value);
      GST_DEBUG_OBJECT (inference_overlay, "Changed font scale to %lf",
          priv->font_scale);
      break;
    case PROP_THICKNESS:
      priv->thickness = g_value_get_int (value);
      GST_DEBUG_OBJECT (inference_overlay, "Changed box thickness to %d",
          priv->thickness);
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
      GST_DEBUG_OBJECT (inference_overlay, "Changed inference labels %s",
          priv->labels);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_inference_overlay_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstInferenceOverlay *inference_overlay = GST_INFERENCE_OVERLAY (object);
  GstInferenceOverlayPrivate *priv =
      GST_INFERENCE_OVERLAY_PRIVATE (inference_overlay);

  GST_DEBUG_OBJECT (inference_overlay, "get_property");

  switch (property_id) {
    case PROP_FONT_SCALE:
      g_value_set_double (value, priv->font_scale);
      break;
    case PROP_THICKNESS:
      g_value_set_int (value, priv->thickness);
      break;
    case PROP_LABELS:
      g_value_set_string (value, priv->labels);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_inference_overlay_dispose (GObject * object)
{
  GstInferenceOverlay *inference_overlay = GST_INFERENCE_OVERLAY (object);
  GstInferenceOverlayPrivate *priv =
      GST_INFERENCE_OVERLAY_PRIVATE (inference_overlay);

  GST_DEBUG_OBJECT (inference_overlay, "dispose");

  /* clean up as possible.  may be called multiple times */
  if (priv->labels_list != NULL) {
    g_strfreev (priv->labels_list);
  }
  if (priv->labels != NULL) {
    g_free (priv->labels);
  }

  G_OBJECT_CLASS (gst_inference_overlay_parent_class)->dispose (object);
}

void
gst_inference_overlay_finalize (GObject * object)
{
  GstInferenceOverlay *inference_overlay = GST_INFERENCE_OVERLAY (object);

  GST_DEBUG_OBJECT (inference_overlay, "finalize");

  /* clean up object here */

  G_OBJECT_CLASS (gst_inference_overlay_parent_class)->finalize (object);
}

static gboolean
gst_inference_overlay_start (GstBaseTransform * trans)
{
  GstInferenceOverlay *inference_overlay = GST_INFERENCE_OVERLAY (trans);

  GST_DEBUG_OBJECT (inference_overlay, "start");

  return TRUE;
}

static gboolean
gst_inference_overlay_stop (GstBaseTransform * trans)
{
  GstInferenceOverlay *inference_overlay = GST_INFERENCE_OVERLAY (trans);

  GST_DEBUG_OBJECT (inference_overlay, "stop");

  return TRUE;
}

/* transform */
static GstFlowReturn
gst_inference_overlay_transform_frame_ip (GstVideoFilter * trans,
    GstVideoFrame * frame)
{
  GstInferenceOverlayClass *io_class = GST_INFERENCE_OVERLAY_GET_CLASS (trans);
  GstInferenceOverlay *inference_overlay = GST_INFERENCE_OVERLAY (trans);
  GstInferenceOverlayPrivate *priv =
      GST_INFERENCE_OVERLAY_PRIVATE (inference_overlay);
  GstMeta *meta;
  GstFlowReturn ret = GST_FLOW_ERROR;

  meta = gst_buffer_get_meta (frame->buffer, io_class->meta_type);
  if (NULL == meta) {
    GST_LOG_OBJECT (trans, "No inference meta found");
    ret = GST_FLOW_OK;
  } else {
    GST_LOG_OBJECT (trans, "Valid inference meta found");
    if (io_class->process_meta != NULL) {
      ret =
          io_class->process_meta (frame, meta, priv->font_scale,
          priv->thickness, priv->labels_list, priv->num_labels);
    }
  }

  return ret;
}
