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
 * SECTION:element-gstinceptionv1
 *
 * The inceptionv1 element allows the user to infer/execute a pretrained model
 * based on the GoogLeNet (Inception v1) architecture on incoming image frames.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v videotestsrc ! inceptionv1 ! xvimagesink
 * ]|
 * Process video frames from the camera using a GoogLeNet (Inception v1) model.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstinceptionv1.h"
#include "gst/r2inference/gstinferencemeta.h"
#include <string.h>

GST_DEBUG_CATEGORY_STATIC (gst_inceptionv1_debug_category);
#define GST_CAT_DEFAULT gst_inceptionv1_debug_category

/* prototypes */
static void gst_inceptionv1_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_inceptionv1_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_inceptionv1_dispose (GObject * object);
static void gst_inceptionv1_finalize (GObject * object);

static gboolean gst_inceptionv1_preprocess (GstVideoInference * vi,
    GstVideoFrame * inframe, GstVideoFrame * outframe);
static gboolean gst_inceptionv1_postprocess (GstVideoInference * vi,
    const gpointer prediction, gsize predsize, GstMeta * meta_model,
    GstVideoInfo * info_model, gboolean * valid_prediction);
static gboolean gst_inceptionv1_start (GstVideoInference * vi);
static gboolean gst_inceptionv1_stop (GstVideoInference * vi);

enum
{
  PROP_0
};

/* pad templates */

#define CAPS "video/x-raw,format=RGB,width=224,height=224"

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

struct _GstInceptionv1
{
  GstVideoInference parent;
};

struct _GstInceptionv1Class
{
  GstVideoInferenceClass parent;
};

/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstInceptionv1, gst_inceptionv1,
    GST_TYPE_VIDEO_INFERENCE,
    GST_DEBUG_CATEGORY_INIT (gst_inceptionv1_debug_category, "inceptionv1", 0,
        "debug category for inceptionv1 element"));

static void
gst_inceptionv1_class_init (GstInceptionv1Class * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  GstVideoInferenceClass *vi_class = GST_VIDEO_INFERENCE_CLASS (klass);

  gst_element_class_add_static_pad_template (element_class,
      &sink_model_factory);
  gst_element_class_add_static_pad_template (element_class, &src_model_factory);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "inceptionv1", "Filter",
      "Infers incoming image frames using a pretrained GoogLeNet (Inception v1) model",
      "Carlos Rodriguez <carlos.rodriguez@ridgerun.com> \n\t\t\t"
      "   Jose Jimenez <jose.jimenez@ridgerun.com> \n\t\t\t"
      "   Michael Gruner <michael.gruner@ridgerun.com>");

  gobject_class->set_property = gst_inceptionv1_set_property;
  gobject_class->get_property = gst_inceptionv1_get_property;
  gobject_class->dispose = gst_inceptionv1_dispose;
  gobject_class->finalize = gst_inceptionv1_finalize;

  vi_class->start = GST_DEBUG_FUNCPTR (gst_inceptionv1_start);
  vi_class->stop = GST_DEBUG_FUNCPTR (gst_inceptionv1_stop);
  vi_class->preprocess = GST_DEBUG_FUNCPTR (gst_inceptionv1_preprocess);
  vi_class->postprocess = GST_DEBUG_FUNCPTR (gst_inceptionv1_postprocess);
  vi_class->inference_meta_info = gst_classification_meta_get_info ();
}

static void
gst_inceptionv1_init (GstInceptionv1 * inceptionv1)
{
}

void
gst_inceptionv1_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstInceptionv1 *inceptionv1 = GST_INCEPTIONV1 (object);

  GST_DEBUG_OBJECT (inceptionv1, "set_property");

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_inceptionv1_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstInceptionv1 *inceptionv1 = GST_INCEPTIONV1 (object);

  GST_DEBUG_OBJECT (inceptionv1, "get_property");

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_inceptionv1_dispose (GObject * object)
{
  GstInceptionv1 *inceptionv1 = GST_INCEPTIONV1 (object);

  GST_DEBUG_OBJECT (inceptionv1, "dispose");

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (gst_inceptionv1_parent_class)->dispose (object);
}

void
gst_inceptionv1_finalize (GObject * object)
{
  GstInceptionv1 *inceptionv1 = GST_INCEPTIONV1 (object);

  GST_DEBUG_OBJECT (inceptionv1, "finalize");

  /* clean up object here */

  G_OBJECT_CLASS (gst_inceptionv1_parent_class)->finalize (object);
}

static gboolean
gst_inceptionv1_preprocess (GstVideoInference * vi,
    GstVideoFrame * inframe, GstVideoFrame * outframe)
{
  gint i, j, pixel_stride, width, height, channels;
  const gfloat mean = 128.0f;
  const gfloat std = 0.0078125f;

  GST_LOG_OBJECT (vi, "Preprocess");
  channels = GST_VIDEO_FRAME_N_COMPONENTS (inframe);
  pixel_stride = GST_VIDEO_FRAME_COMP_STRIDE (inframe, 0) / channels;
  width = GST_VIDEO_FRAME_WIDTH (inframe);
  height = GST_VIDEO_FRAME_HEIGHT (inframe);

  for (i = 0; i < height; ++i) {
    for (j = 0; j < width; ++j) {
      ((gfloat *) outframe->data[0])[(i * width + j) * channels + 0] =
          (((guchar *) inframe->data[0])[(i * pixel_stride + j) * channels +
              0] - mean) * std;
      ((gfloat *) outframe->data[0])[(i * width + j) * channels + 1] =
          (((guchar *) inframe->data[0])[(i * pixel_stride + j) * channels +
              1] - mean) * std;
      ((gfloat *) outframe->data[0])[(i * width + j) * channels + 2] =
          (((guchar *) inframe->data[0])[(i * pixel_stride + j) * channels +
              2] - mean) * std;
    }
  }

  return TRUE;
}

static gboolean
gst_inceptionv1_postprocess (GstVideoInference * vi, const gpointer prediction,
    gsize predsize, GstMeta * meta_model, GstVideoInfo * info_model,
    gboolean * valid_prediction)
{
  GstClassificationMeta *class_meta = (GstClassificationMeta *) meta_model;
  gint index;
  gdouble max;
  GstDebugLevel level;
  GST_LOG_OBJECT (vi, "Postprocess");

  class_meta->num_labels = predsize / sizeof (gfloat);
  class_meta->label_probs =
      g_malloc (class_meta->num_labels * sizeof (gdouble));
  for (gint i = 0; i < class_meta->num_labels; ++i) {
    class_meta->label_probs[i] = (gdouble) ((gfloat *) prediction)[i];
  }

  /* Only compute the highest probability is label when debug >= 6 */
  level = gst_debug_category_get_threshold (gst_inceptionv1_debug_category);
  if (level >= GST_LEVEL_LOG) {
    index = 0;
    max = -1;
    for (gint i = 0; i < class_meta->num_labels; ++i) {
      gfloat current = ((gfloat *) prediction)[i];
      if (current > max) {
        max = current;
        index = i;
      }
    }
    GST_LOG_OBJECT (vi, "Highest probability is label %i : (%f)", index, max);
  }

  *valid_prediction = TRUE;
  return TRUE;
}

static gboolean
gst_inceptionv1_start (GstVideoInference * vi)
{
  GST_INFO_OBJECT (vi, "Starting Inception v1");

  return TRUE;
}

static gboolean
gst_inceptionv1_stop (GstVideoInference * vi)
{
  GST_INFO_OBJECT (vi, "Stopping Inception v1");

  return TRUE;
}
