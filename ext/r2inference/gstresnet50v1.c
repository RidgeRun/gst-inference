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
 * SECTION:element-gstresnet50v1
 *
 * The resnet50v1 element allows the user to infer/execute a pretrained model
 * based on the ResNet (ResNet v1 or ResNet v2) architectures on 
 * incoming image frames.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v videotestsrc ! resnet50v1 ! xvimagesink
 * ]|
 * Process video frames from the camera using a GoogLeNet (Resnet50 v1 or 
 * Resnet50 v2) model.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstresnet50v1.h"
#include "gst/r2inference/gstinferencemeta.h"
#include <string.h>

GST_DEBUG_CATEGORY_STATIC (gst_resnet50v1_debug_category);
#define GST_CAT_DEFAULT gst_resnet50v1_debug_category

#define MODEL_CHANNELS 3

/* prototypes */
static gboolean gst_resnet50v1_preprocess (GstVideoInference * vi,
    GstVideoFrame * inframe, GstVideoFrame * outframe);
static gboolean gst_resnet50v1_postprocess (GstVideoInference * vi,
    const gpointer prediction, gsize predsize, GstMeta * meta_model,
    GstVideoInfo * info_model, gboolean * valid_prediction);
static gboolean gst_resnet50v1_start (GstVideoInference * vi);
static gboolean gst_resnet50v1_stop (GstVideoInference * vi);

enum
{
  PROP_0
};

/* pad templates */
#define CAPS								\
  "video/x-raw, "							\
  "width=224, "							\
  "height=224, "							\
  "format={RGB, RGBx, RGBA, BGR, BGRx, BGRA, xRGB, ARGB, xBGR, ABGR}"

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

struct _GstResnet50v1
{
  GstVideoInference parent;
};

struct _GstResnet50v1Class
{
  GstVideoInferenceClass parent;
};

/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstResnet50v1, gst_resnet50v1,
    GST_TYPE_VIDEO_INFERENCE,
    GST_DEBUG_CATEGORY_INIT (gst_resnet50v1_debug_category, "resnet50v1", 0,
        "debug category for resnet50v1 element"));

static void
gst_resnet50v1_class_init (GstResnet50v1Class * klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  GstVideoInferenceClass *vi_class = GST_VIDEO_INFERENCE_CLASS (klass);

  gst_element_class_add_static_pad_template (element_class,
      &sink_model_factory);
  gst_element_class_add_static_pad_template (element_class, &src_model_factory);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "resnet50v1", "Filter",
      "Infers incoming image frames using a pretrained ResNet (Resnet50 v1 or Resnet50 v2) model",
      "Carlos Rodriguez <carlos.rodriguez@ridgerun.com> \n\t\t\t"
      "   Jose Jimenez <jose.jimenez@ridgerun.com> \n\t\t\t"
      "   Michael Gruner <michael.gruner@ridgerun.com> \n\t\t\t"
      "   Greivin Fallas <greivin.fallas@ridgerun.com>");

  vi_class->start = GST_DEBUG_FUNCPTR (gst_resnet50v1_start);
  vi_class->stop = GST_DEBUG_FUNCPTR (gst_resnet50v1_stop);
  vi_class->preprocess = GST_DEBUG_FUNCPTR (gst_resnet50v1_preprocess);
  vi_class->postprocess = GST_DEBUG_FUNCPTR (gst_resnet50v1_postprocess);
  vi_class->inference_meta_info = gst_classification_meta_get_info ();
}

static void
gst_resnet50v1_init (GstResnet50v1 * resnet50v1)
{
}

static gboolean
gst_resnet50v1_preprocess (GstVideoInference * vi,
    GstVideoFrame * inframe, GstVideoFrame * outframe)
{
  gint i, j, pixel_stride, width, height, channels;
  gint first_index, last_index, offset;
  const gdouble mean_red = 123.68;
  const gdouble mean_green = 116.78;
  const gdouble mean_blue = 103.94;

  GST_LOG_OBJECT (vi, "Preprocess");

  channels = inframe->info.finfo->n_components;
  switch (GST_VIDEO_FRAME_FORMAT (inframe)) {
    case GST_VIDEO_FORMAT_RGB:
    case GST_VIDEO_FORMAT_RGBx:
    case GST_VIDEO_FORMAT_RGBA:
      first_index = 0;
      last_index = 2;
      offset = 0;
      break;
    case GST_VIDEO_FORMAT_BGR:
    case GST_VIDEO_FORMAT_BGRx:
    case GST_VIDEO_FORMAT_BGRA:
      first_index = 2;
      last_index = 0;
      offset = 0;
      break;
    case GST_VIDEO_FORMAT_xRGB:
    case GST_VIDEO_FORMAT_ARGB:
      first_index = 0;
      last_index = 2;
      offset = 1;
      break;
    case GST_VIDEO_FORMAT_xBGR:
    case GST_VIDEO_FORMAT_ABGR:
      first_index = 2;
      last_index = 0;
      offset = 1;
      break;
    default:
      GST_ERROR_OBJECT (vi, "Invalid format");
      return FALSE;
      break;
  }
  pixel_stride = GST_VIDEO_FRAME_COMP_STRIDE (inframe, 0) / channels;
  width = GST_VIDEO_FRAME_WIDTH (inframe);
  height = GST_VIDEO_FRAME_HEIGHT (inframe);

  for (i = 0; i < height; ++i) {
    for (j = 0; j < width; ++j) {
      ((gfloat *) outframe->data[0])[(i * width + j) * MODEL_CHANNELS +
          first_index] =
          (((guchar *) inframe->data[0])[(i * pixel_stride + j) * channels + 0 +
              offset] - mean_red);
      ((gfloat *) outframe->data[0])[(i * width + j) * MODEL_CHANNELS + 1] =
          (((guchar *) inframe->data[0])[(i * pixel_stride + j) * channels + 1 +
              offset] - mean_green);
      ((gfloat *) outframe->data[0])[(i * width + j) * MODEL_CHANNELS +
          last_index] =
          (((guchar *) inframe->data[0])[(i * pixel_stride + j) * channels + 2 +
              offset] - mean_blue);
    }
  }

  return TRUE;
}

static gboolean
gst_resnet50v1_postprocess (GstVideoInference * vi, const gpointer prediction,
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
  level = gst_debug_category_get_threshold (gst_resnet50v1_debug_category);
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
gst_resnet50v1_start (GstVideoInference * vi)
{
  GST_INFO_OBJECT (vi, "Starting Resnet50 v1");

  return TRUE;
}

static gboolean
gst_resnet50v1_stop (GstVideoInference * vi)
{
  GST_INFO_OBJECT (vi, "Stopping Resnet50 v1");

  return TRUE;
}
