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

#include "gstclassificationoverlay.h"

GST_DEBUG_CATEGORY_STATIC (gst_classification_overlay_debug_category);
#define GST_CAT_DEFAULT gst_classification_overlay_debug_category

/* prototypes */


static void gst_classification_overlay_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_classification_overlay_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_classification_overlay_dispose (GObject * object);
static void gst_classification_overlay_finalize (GObject * object);

static gboolean gst_classification_overlay_start (GstBaseTransform * trans);
static gboolean gst_classification_overlay_stop (GstBaseTransform * trans);
static gboolean gst_classification_overlay_set_info (GstVideoFilter * filter,
    GstCaps * incaps, GstVideoInfo * in_info, GstCaps * outcaps,
    GstVideoInfo * out_info);
static GstFlowReturn gst_classification_overlay_transform_frame (GstVideoFilter
    * filter, GstVideoFrame * inframe, GstVideoFrame * outframe);
static GstFlowReturn
gst_classification_overlay_transform_frame_ip (GstVideoFilter * filter,
    GstVideoFrame * frame);

enum
{
  PROP_0
};

/* pad templates */

#define VIDEO_SRC_CAPS \
    GST_VIDEO_CAPS_MAKE("{ RGB }")

#define VIDEO_SINK_CAPS \
    GST_VIDEO_CAPS_MAKE("{ RGB }")


/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstClassificationOverlay, gst_classification_overlay,
    GST_TYPE_VIDEO_FILTER,
    GST_DEBUG_CATEGORY_INIT (gst_classification_overlay_debug_category,
        "classification_overlay", 0,
        "debug category for classification_overlay element"));

static void
gst_classification_overlay_class_init (GstClassificationOverlayClass * klass)
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

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "classificationoverlay", "Filter",
      "Overlays classification metadata on input buffer",
      "Carlos Rodriguez <carlos.rodriguez@ridgerun.com> \n\t\t\t"
      "   Jose Jimenez <jose.jimenez@ridgerun.com> \n\t\t\t"
      "   Michael Gruner <michael.gruner@ridgerun.com> \n\t\t\t"
      "   Carlos Aguero <carlos.aguero@ridgerun.com> \n\t\t\t"
      "   Miguel Taylor <miguel.taylor@ridgerun.com> \n\t\t\t"
      "   Greivin Fallas <greivin.fallas@ridgerun.com>");

  gobject_class->set_property = gst_classification_overlay_set_property;
  gobject_class->get_property = gst_classification_overlay_get_property;
  gobject_class->dispose = gst_classification_overlay_dispose;
  gobject_class->finalize = gst_classification_overlay_finalize;
  base_transform_class->start =
      GST_DEBUG_FUNCPTR (gst_classification_overlay_start);
  base_transform_class->stop =
      GST_DEBUG_FUNCPTR (gst_classification_overlay_stop);
  video_filter_class->set_info =
      GST_DEBUG_FUNCPTR (gst_classification_overlay_set_info);
  video_filter_class->transform_frame =
      GST_DEBUG_FUNCPTR (gst_classification_overlay_transform_frame);
  video_filter_class->transform_frame_ip =
      GST_DEBUG_FUNCPTR (gst_classification_overlay_transform_frame_ip);

}

static void
gst_classification_overlay_init (GstClassificationOverlay *
    classification_overlay)
{
}

void
gst_classification_overlay_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstClassificationOverlay *classification_overlay =
      GST_CLASSIFICATION_OVERLAY (object);

  GST_DEBUG_OBJECT (classification_overlay, "set_property");

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_classification_overlay_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstClassificationOverlay *classification_overlay =
      GST_CLASSIFICATION_OVERLAY (object);

  GST_DEBUG_OBJECT (classification_overlay, "get_property");

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_classification_overlay_dispose (GObject * object)
{
  GstClassificationOverlay *classification_overlay =
      GST_CLASSIFICATION_OVERLAY (object);

  GST_DEBUG_OBJECT (classification_overlay, "dispose");

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (gst_classification_overlay_parent_class)->dispose (object);
}

void
gst_classification_overlay_finalize (GObject * object)
{
  GstClassificationOverlay *classification_overlay =
      GST_CLASSIFICATION_OVERLAY (object);

  GST_DEBUG_OBJECT (classification_overlay, "finalize");

  /* clean up object here */

  G_OBJECT_CLASS (gst_classification_overlay_parent_class)->finalize (object);
}

static gboolean
gst_classification_overlay_start (GstBaseTransform * trans)
{
  GstClassificationOverlay *classification_overlay =
      GST_CLASSIFICATION_OVERLAY (trans);

  GST_DEBUG_OBJECT (classification_overlay, "start");

  return TRUE;
}

static gboolean
gst_classification_overlay_stop (GstBaseTransform * trans)
{
  GstClassificationOverlay *classification_overlay =
      GST_CLASSIFICATION_OVERLAY (trans);

  GST_DEBUG_OBJECT (classification_overlay, "stop");

  return TRUE;
}

static gboolean
gst_classification_overlay_set_info (GstVideoFilter * filter, GstCaps * incaps,
    GstVideoInfo * in_info, GstCaps * outcaps, GstVideoInfo * out_info)
{
  GstClassificationOverlay *classification_overlay =
      GST_CLASSIFICATION_OVERLAY (filter);

  GST_DEBUG_OBJECT (classification_overlay, "set_info");

  return TRUE;
}

/* transform */
static GstFlowReturn
gst_classification_overlay_transform_frame (GstVideoFilter * filter,
    GstVideoFrame * inframe, GstVideoFrame * outframe)
{
  GstClassificationOverlay *classification_overlay =
      GST_CLASSIFICATION_OVERLAY (filter);

  GST_DEBUG_OBJECT (classification_overlay, "transform_frame");

  return GST_FLOW_OK;
}

static GstFlowReturn
gst_classification_overlay_transform_frame_ip (GstVideoFilter * filter,
    GstVideoFrame * frame)
{
  GstClassificationOverlay *classification_overlay =
      GST_CLASSIFICATION_OVERLAY (filter);

  GST_DEBUG_OBJECT (classification_overlay, "transform_frame_ip");

  return GST_FLOW_OK;
}
