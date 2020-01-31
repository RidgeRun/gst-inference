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
 * SECTION:element-gstinceptionv1
 *
 * The inceptionv1 element allows the user to infer/execute a pretrained model
 * based on the GoogLeNet (Inception v1 or Inception v2) architectures on 
 * incoming image frames.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v videotestsrc ! inceptionv1 ! xvimagesink
 * ]|
 * Process video frames from the camera using a GoogLeNet (Inception v1 or 
 * Inception v2) model.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstinceptionv1.h"
#include "gst/r2inference/gstinferencemeta.h"
#include <string.h>
#include "gst/r2inference/gstinferencepreprocess.h"
#include "gst/r2inference/gstinferencepostprocess.h"
#include "gst/r2inference/gstinferencedebug.h"

GST_DEBUG_CATEGORY_STATIC (gst_inceptionv1_debug_category);
#define GST_CAT_DEFAULT gst_inceptionv1_debug_category

#define MEAN 128.0
#define STD 1/128.0
#define MODEL_CHANNELS 3

static gboolean gst_inceptionv1_preprocess (GstVideoInference * vi,
    GstVideoFrame * inframe, GstVideoFrame * outframe);
static gboolean gst_inceptionv1_postprocess (GstVideoInference * vi,
    const gpointer prediction, gsize predsize, GstMeta * meta_model[2],
    GstVideoInfo * info_model, gboolean * valid_prediction,
    gchar ** labels_list, gint num_labels);
static gboolean gst_inceptionv1_postprocess_old (GstVideoInference * vi,
    const gpointer prediction, gsize predsize, GstMeta * meta_model,
    GstVideoInfo * info_model, gboolean * valid_prediction);
static gboolean gst_inceptionv1_postprocess_new (GstVideoInference * vi,
    const gpointer prediction, gsize predsize, GstMeta * meta_model,
    GstVideoInfo * info_model, gboolean * valid_prediction,
    gchar ** labels_list, gint num_labels);
static gboolean gst_inceptionv1_start (GstVideoInference * vi);
static gboolean gst_inceptionv1_stop (GstVideoInference * vi);

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
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  GstVideoInferenceClass *vi_class = GST_VIDEO_INFERENCE_CLASS (klass);

  gst_element_class_add_static_pad_template (element_class,
      &sink_model_factory);
  gst_element_class_add_static_pad_template (element_class, &src_model_factory);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "inceptionv1", "Filter",
      "Infers incoming image frames using a pretrained GoogLeNet (Inception v1 or Inception v2) model",
      "Carlos Rodriguez <carlos.rodriguez@ridgerun.com> \n\t\t\t"
      "   Jose Jimenez <jose.jimenez@ridgerun.com> \n\t\t\t"
      "   Michael Gruner <michael.gruner@ridgerun.com> \n\t\t\t"
      "   Mauricio Montero <mauricio.montero@ridgerun.com>");

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

static gboolean
gst_inceptionv1_preprocess (GstVideoInference * vi,
    GstVideoFrame * inframe, GstVideoFrame * outframe)
{
  GST_LOG_OBJECT (vi, "Preprocess");
  return gst_normalize (inframe, outframe, MEAN, STD, MODEL_CHANNELS);
}

static gboolean
gst_inceptionv1_postprocess_old (GstVideoInference * vi,
    const gpointer prediction, gsize predsize, GstMeta * meta_model,
    GstVideoInfo * info_model, gboolean * valid_prediction)
{
  GstClassificationMeta *class_meta = (GstClassificationMeta *) meta_model;
  GstDebugLevel gst_debug_level = GST_LEVEL_LOG;

  GST_LOG_OBJECT (vi, "Postprocess");

  gst_fill_classification_meta (class_meta, prediction, predsize);

  gst_inference_print_highest_probability (vi, gst_inceptionv1_debug_category,
      class_meta, prediction, gst_debug_level);

  *valid_prediction = TRUE;
  return TRUE;
}

static gboolean
gst_inceptionv1_postprocess_new (GstVideoInference * vi,
    const gpointer prediction, gsize predsize, GstMeta * meta_model,
    GstVideoInfo * info_model, gboolean * valid_prediction,
    gchar ** labels_list, gint num_labels)
{
  GstInferenceMeta *imeta = NULL;
  GstInferenceClassification *c = NULL;
  GstInferencePrediction *root = NULL;

  g_return_val_if_fail (vi != NULL, FALSE);
  g_return_val_if_fail (meta_model != NULL, FALSE);
  g_return_val_if_fail (info_model != NULL, FALSE);

  GST_LOG_OBJECT (vi, "Postprocess Meta");

  imeta = (GstInferenceMeta *) meta_model;

  root = imeta->prediction;
  if (!root) {
    GST_ERROR_OBJECT (vi, "Prediction is not part of the Inference Meta");
    return FALSE;
  }

  c = gst_create_class_from_prediction (vi, prediction, predsize, labels_list,
      num_labels);
  gst_inference_prediction_append_classification (root, c);
  gst_inference_print_predictions (vi, gst_inceptionv1_debug_category, imeta);

  *valid_prediction = TRUE;
  return TRUE;
}

static gboolean
gst_inceptionv1_postprocess (GstVideoInference * vi, const gpointer prediction,
    gsize predsize, GstMeta * meta_model[2], GstVideoInfo * info_model,
    gboolean * valid_prediction, gchar ** labels_list, gint num_labels)
{
  gboolean ret = TRUE;

  ret &=
      gst_inceptionv1_postprocess_old (vi, prediction, predsize, meta_model[0],
      info_model, valid_prediction);
  ret &=
      gst_inceptionv1_postprocess_new (vi, prediction, predsize, meta_model[1],
      info_model, valid_prediction, labels_list, num_labels);

  return ret;
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
