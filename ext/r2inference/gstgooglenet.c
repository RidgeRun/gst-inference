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

/**
 * SECTION:element-gstgooglenet
 *
 * The googlenet element allows the user to infer/execute a pretrained model
 * based on the GoogLeNet architecture on incoming image frames.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v videotestsrc ! googlenet ! xvimagesink
 * ]|
 * Process video frames from the camera using a GoogLeNet model.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstgooglenet.h"
#include <string.h>

GST_DEBUG_CATEGORY_STATIC (gst_googlenet_debug_category);
#define GST_CAT_DEFAULT gst_googlenet_debug_category

/* prototypes */


static void gst_googlenet_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_googlenet_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_googlenet_dispose (GObject * object);
static void gst_googlenet_finalize (GObject * object);

static gboolean gst_googlenet_preprocess (GstVideoInference * vi,
    GstBuffer * inbuf, GstBuffer * outbuf);
static gboolean gst_googlenet_postprocess (GstVideoInference * vi,
    GstBuffer * buf, const gpointer prediction, gsize predsize);
static gboolean gst_googlenet_start (GstVideoInference * vi);
static gboolean gst_googlenet_stop (GstVideoInference * vi);

enum
{
  PROP_0
};

/* pad templates */

#define CAPS "video/x-raw,format=BGR,width=224,height=224"

static GstStaticPadTemplate sink_model_factory =
GST_STATIC_PAD_TEMPLATE ("sink_model",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS (CAPS)
    );

static GstStaticPadTemplate src_model_factory =
GST_STATIC_PAD_TEMPLATE ("src_model",
    GST_PAD_SRC,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS (CAPS)
    );

struct _GstGooglenet
{
  GstVideoInference parent;
};

struct _GstGooglenetClass
{
  GstVideoInferenceClass parent;
};

/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstGooglenet, gst_googlenet, GST_TYPE_VIDEO_INFERENCE,
    GST_DEBUG_CATEGORY_INIT (gst_googlenet_debug_category, "googlenet", 0,
        "debug category for googlenet element"));

static void
gst_googlenet_class_init (GstGooglenetClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  GstVideoInferenceClass *vi_class = GST_VIDEO_INFERENCE_CLASS (klass);

  gst_element_class_add_static_pad_template (element_class,
      &sink_model_factory);
  gst_element_class_add_static_pad_template (element_class, &src_model_factory);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "googlenet", "Filter",
      "Infers incoming image frames using a pretrained GoogLeNet model",
      "Carlos Rodriguez <carlos.rodriguez@ridgerun.com> \n\t\t\t"
      "   Jose Jimenez <jose.jimenez@ridgerun.com> \n\t\t\t"
      "   Michael Gruner <michael.gruner@ridgerun.com>");

  gobject_class->set_property = gst_googlenet_set_property;
  gobject_class->get_property = gst_googlenet_get_property;
  gobject_class->dispose = gst_googlenet_dispose;
  gobject_class->finalize = gst_googlenet_finalize;

  vi_class->start = GST_DEBUG_FUNCPTR (gst_googlenet_start);
  vi_class->stop = GST_DEBUG_FUNCPTR (gst_googlenet_stop);
  vi_class->preprocess = GST_DEBUG_FUNCPTR (gst_googlenet_preprocess);
  vi_class->postprocess = GST_DEBUG_FUNCPTR (gst_googlenet_postprocess);
}

static void
gst_googlenet_init (GstGooglenet * googlenet)
{
}

void
gst_googlenet_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstGooglenet *googlenet = GST_GOOGLENET (object);

  GST_DEBUG_OBJECT (googlenet, "set_property");

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_googlenet_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstGooglenet *googlenet = GST_GOOGLENET (object);

  GST_DEBUG_OBJECT (googlenet, "get_property");

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_googlenet_dispose (GObject * object)
{
  GstGooglenet *googlenet = GST_GOOGLENET (object);

  GST_DEBUG_OBJECT (googlenet, "dispose");

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (gst_googlenet_parent_class)->dispose (object);
}

void
gst_googlenet_finalize (GObject * object)
{
  GstGooglenet *googlenet = GST_GOOGLENET (object);

  GST_DEBUG_OBJECT (googlenet, "finalize");

  /* clean up object here */

  G_OBJECT_CLASS (gst_googlenet_parent_class)->finalize (object);
}

static gboolean
gst_googlenet_preprocess (GstVideoInference * vi,
    GstBuffer * inbuf, GstBuffer * outbuf)
{
  GstMapInfo ininfo;
  GstMapInfo outinfo;

  GST_LOG_OBJECT (vi, "Preprocess");

  gst_buffer_map (inbuf, &ininfo, GST_MAP_READ);
  gst_buffer_map (outbuf, &outinfo, GST_MAP_WRITE);

  memcpy (outinfo.data, ininfo.data, ininfo.size);

  gst_buffer_unmap (inbuf, &ininfo);
  gst_buffer_unmap (outbuf, &outinfo);

  return TRUE;
}

static gboolean
gst_googlenet_postprocess (GstVideoInference * vi,
    GstBuffer * buf, const gpointer prediction, gsize predsize)
{
  GST_LOG_OBJECT (vi, "Postprocess");

  return TRUE;
}

static gboolean
gst_googlenet_start (GstVideoInference * vi)
{
  GST_INFO_OBJECT (vi, "Starting GoogLeNet");

  return TRUE;
}

static gboolean
gst_googlenet_stop (GstVideoInference * vi)
{
  GST_INFO_OBJECT (vi, "Stopping GoogLeNet");

  return TRUE;
}
