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
 * SECTION:element-gsttinyyolo
 *
 * The tinyyolo element allows the user to infer/execute a pretrained model
 * based on the TinyYolo architecture on incoming image frames.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v videotestsrc ! tinyyolo ! xvimagesink
 * ]|
 * Process video frames from the camera using a TinyYolo model.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gsttinyyolo.h"
#include <string.h>

GST_DEBUG_CATEGORY_STATIC (gst_tinyyolo_debug_category);
#define GST_CAT_DEFAULT gst_tinyyolo_debug_category

/* prototypes */


static void gst_tinyyolo_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_tinyyolo_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_tinyyolo_dispose (GObject * object);
static void gst_tinyyolo_finalize (GObject * object);

static gboolean gst_tinyyolo_preprocess (GstVideoInference * vi,
    GstBuffer * inbuf, GstBuffer * outbuf);
static gboolean gst_tinyyolo_postprocess (GstVideoInference * vi,
    GstBuffer * buf, const gpointer prediction, gsize predsize);
static gboolean gst_tinyyolo_start (GstVideoInference * vi);
static gboolean gst_tinyyolo_stop (GstVideoInference * vi);

enum
{
  PROP_0
};

/* pad templates */

#define CAPS "video/x-raw,format=BGR,width=448,height=448"

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

struct _GstTinyyolo
{
  GstVideoInference parent;
};

struct _GstTinyyoloClass
{
  GstVideoInferenceClass parent;
};

/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstTinyyolo, gst_tinyyolo, GST_TYPE_VIDEO_INFERENCE,
    GST_DEBUG_CATEGORY_INIT (gst_tinyyolo_debug_category, "tinyyolo", 0,
        "debug category for tinyyolo element"));

static void
gst_tinyyolo_class_init (GstTinyyoloClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  GstVideoInferenceClass *vi_class = GST_VIDEO_INFERENCE_CLASS (klass);

  gst_element_class_add_static_pad_template (element_class,
      &sink_model_factory);
  gst_element_class_add_static_pad_template (element_class, &src_model_factory);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "tinyyolo", "Filter",
      "Infers incoming image frames using a pretrained TinyYolo model",
      "Carlos Rodriguez <carlos.rodriguez@ridgerun.com> \n\t\t\t"
      "Jose Jimenez <jose.jimenez@ridgerun.com> \n\t\t\t"
      "Michael Gruner <michael.gruner@ridgerun.com> \n\t\t\t"
      "Carlos Aguero <carlos.aguero@ridgerun.com> \n\t\t\t"
      "Miguel Taylor <miguel.taylor@ridgerun.com> \n\t\t\t"
      "Greivin Fallas <greivin.fallas@ridgerun.com>");

  gobject_class->set_property = gst_tinyyolo_set_property;
  gobject_class->get_property = gst_tinyyolo_get_property;
  gobject_class->dispose = gst_tinyyolo_dispose;
  gobject_class->finalize = gst_tinyyolo_finalize;

  vi_class->start = GST_DEBUG_FUNCPTR (gst_tinyyolo_start);
  vi_class->stop = GST_DEBUG_FUNCPTR (gst_tinyyolo_stop);
  vi_class->preprocess = GST_DEBUG_FUNCPTR (gst_tinyyolo_preprocess);
  vi_class->postprocess = GST_DEBUG_FUNCPTR (gst_tinyyolo_postprocess);
}

static void
gst_tinyyolo_init (GstTinyyolo * tinyyolo)
{
}

void
gst_tinyyolo_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstTinyyolo *tinyyolo = GST_TINYYOLO (object);

  GST_DEBUG_OBJECT (tinyyolo, "set_property");

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_tinyyolo_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstTinyyolo *tinyyolo = GST_TINYYOLO (object);

  GST_DEBUG_OBJECT (tinyyolo, "get_property");

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_tinyyolo_dispose (GObject * object)
{
  GstTinyyolo *tinyyolo = GST_TINYYOLO (object);

  GST_DEBUG_OBJECT (tinyyolo, "dispose");

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (gst_tinyyolo_parent_class)->dispose (object);
}

void
gst_tinyyolo_finalize (GObject * object)
{
  GstTinyyolo *tinyyolo = GST_TINYYOLO (object);

  GST_DEBUG_OBJECT (tinyyolo, "finalize");

  /* clean up object here */

  G_OBJECT_CLASS (gst_tinyyolo_parent_class)->finalize (object);
}

static gboolean
gst_tinyyolo_preprocess (GstVideoInference * vi,
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
gst_tinyyolo_postprocess (GstVideoInference * vi,
    GstBuffer * buf, const gpointer prediction, gsize predsize)
{
  GST_LOG_OBJECT (vi, "Postprocess");

  return TRUE;
}

static gboolean
gst_tinyyolo_start (GstVideoInference * vi)
{
  GST_INFO_OBJECT (vi, "Starting TinyYolo");

  return TRUE;
}

static gboolean
gst_tinyyolo_stop (GstVideoInference * vi)
{
  GST_INFO_OBJECT (vi, "Stopping TinyYolo");

  return TRUE;
}
