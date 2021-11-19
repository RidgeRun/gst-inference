/*
 * GStreamer
 * Copyright (C) 2018-2021 RidgeRun <support@ridgerun.com>
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
 * SECTION:element-rosetta
 *
 * The rosetta element allows the user to infer/execute a pretrained model
 * based on the ResNet architecture on incoming image frames.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * 
 * Process video frames from the camera using a TinyYolo model.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "stdio.h"

#include "gstrosetta.h"
#include "gst/r2inference/gstinferencemeta.h"
#include <string.h>
#include "gst/r2inference/gstinferencepreprocess.h"
#include "gst/r2inference/gstinferencepostprocess.h"
#include "gst/r2inference/gstinferencedebug.h"

GST_DEBUG_CATEGORY_STATIC (gst_rosetta_debug_category);
#define GST_CAT_DEFAULT gst_rosetta_debug_category

#define MODEL_CHANNELS 1
#define MEAN 127.5
#define OFFSET -1


#define MODEL_OUTPUT_ROWS 26
#define MODEL_OUTPUT_COLS 37

/* prototypes */
static void gst_rosetta_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec);

static void gst_rosetta_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_rosetta_preprocess (GstVideoInference * vi,
    GstVideoFrame * inframe, GstVideoFrame * outframe);

static gboolean
gst_rosetta_postprocess (GstVideoInference * vi,
    const gpointer prediction, gsize predsize, GstMeta * meta_model,
    GstVideoInfo * info_model, gboolean * valid_prediction,
    gchar ** labels_list, gint num_labels);

int get_max_indexes (float row[MODEL_OUTPUT_COLS]);

char *concatenate_chars (int maxIndixes[MODEL_OUTPUT_ROWS]);
static gboolean gst_rosetta_start (GstVideoInference * vi);
static gboolean gst_rosetta_stop (GstVideoInference * vi);

#define CAPS								\
  "video/x-raw, "							\
  "width=100, "								\
  "height=32, "							\
  "format={GRAY8}"

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

struct _GstRosetta
{
  GstVideoInference parent;
};

struct _GstRosettaClass
{
  GstVideoInferenceClass parent;
};

/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstRosetta, gst_rosetta,
    GST_TYPE_VIDEO_INFERENCE,
    GST_DEBUG_CATEGORY_INIT (gst_rosetta_debug_category,
        "rosetta", 0, "debug category for rosetta element"));

static void
gst_rosetta_class_init (GstRosettaClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  GstVideoInferenceClass *vi_class = GST_VIDEO_INFERENCE_CLASS (klass);
  gst_element_class_add_static_pad_template (element_class,
      &sink_model_factory);
  gst_element_class_add_static_pad_template (element_class, &src_model_factory);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "Rosetta", "Filter",
      "Infers characters from an incoming image",
      "Edgar Chaves <edgar.chaves@ridgerun.com>");

  gobject_class->set_property = gst_rosetta_set_property;
  gobject_class->get_property = gst_rosetta_get_property;

  vi_class->preprocess = GST_DEBUG_FUNCPTR (gst_rosetta_preprocess);
  vi_class->postprocess = GST_DEBUG_FUNCPTR (gst_rosetta_postprocess);
  vi_class->start = GST_DEBUG_FUNCPTR (gst_rosetta_start);
  vi_class->stop = GST_DEBUG_FUNCPTR (gst_rosetta_stop);
}


static void
gst_rosetta_init (GstRosetta * rosetta)
{
  return;
}

static void
gst_rosetta_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstRosetta *rosetta = GST_ROSETTA (object);
  GST_DEBUG_OBJECT (rosetta, "set_property");
}

static void
gst_rosetta_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstRosetta *rosetta = GST_ROSETTA (object);
  GST_DEBUG_OBJECT (rosetta, "get_property");
}

static gboolean
gst_rosetta_preprocess (GstVideoInference * vi,
    GstVideoFrame * inframe, GstVideoFrame * outframe)
{
  GstRosetta *rosetta = NULL;
  gint width = 0, height = 0;
  g_return_val_if_fail (vi, FALSE);
  g_return_val_if_fail (inframe, FALSE);
  g_return_val_if_fail (outframe, FALSE);

  rosetta = GST_ROSETTA (vi);

  GST_LOG_OBJECT (rosetta, "Rosetta Preprocess");

  width = GST_VIDEO_FRAME_WIDTH (inframe);
  height = GST_VIDEO_FRAME_HEIGHT (inframe);

  GST_LOG_OBJECT (vi, "Input frame dimensions w = %i , h = %i\n", width,
      height);


  return gst_normalize_gray_image (inframe, outframe, MEAN, OFFSET,
      MODEL_CHANNELS);
}

int
get_max_indexes (float row[MODEL_OUTPUT_COLS])
{
  float largest = row[0];
  int temp = 0;
  for (int i = 0; i < MODEL_OUTPUT_COLS; ++i) {
    if (largest < row[i]) {
      largest = row[i];
      temp = i;
    }
  }
  return temp;
}

char *
concatenate_chars (int maxIndixes[MODEL_OUTPUT_ROWS])
{
  int i = 0, counter = 0;
  char chars[37] =
      { '_', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c',
    'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q',
    'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'
  };
  char *final_phrase =
      (char *) malloc (sizeof (char) * (MODEL_OUTPUT_ROWS) + 1);
  memset (final_phrase, ' ', MODEL_OUTPUT_ROWS);

  for (i = 0; i < MODEL_OUTPUT_ROWS; ++i) {
    if (maxIndixes[i] != 0 && !(i > 0 && (maxIndixes[i - 1] == maxIndixes[i]))) {
      final_phrase[counter] = chars[maxIndixes[i]];
      ++counter;
    }
  }

  final_phrase[MODEL_OUTPUT_ROWS] = '\0';

  return final_phrase;
}

static gboolean
gst_rosetta_postprocess (GstVideoInference * vi,
    const gpointer prediction, gsize predsize, GstMeta * meta_model,
    GstVideoInfo * info_model, gboolean * valid_prediction,
    gchar ** labels_list, gint num_labels)
{
  GstRosetta *rosetta = NULL;

  int max_indexes[26];
  float row[MODEL_OUTPUT_COLS];
  int index = 0;
  const gfloat *pred = NULL;
  char *output = NULL;
  GstInferenceMeta *imeta = NULL;
  GstInferencePrediction *root = NULL;

  g_return_val_if_fail (vi, FALSE);
  g_return_val_if_fail (prediction, FALSE);
  g_return_val_if_fail (meta_model, FALSE);
  g_return_val_if_fail (info_model, FALSE);

  GST_LOG_OBJECT (rosetta, "Postprocess");

  GST_INFO_OBJECT (rosetta, "Rosetta Postprocess");

  imeta = (GstInferenceMeta *) meta_model;
  rosetta = GST_ROSETTA (vi);
  root = imeta->prediction;
  if (!root) {
    GST_ERROR_OBJECT (vi, "Prediction is not part of the Inference Meta");
    return FALSE;
  }
  //gst_inference_print_predictions (vi, gst_rosetta_debug_category, imeta);
  pred = (const gfloat *) prediction;
  GST_INFO_OBJECT (vi, "Predicting...");

  for (int j = 0; j < MODEL_OUTPUT_ROWS; ++j) {
    for (int i = 0; i < MODEL_OUTPUT_COLS; ++i) {
      row[i] = pred[index];
      GST_INFO_OBJECT (vi, "Output tensor %i = %f", i, row[i]);
      ++index;
    }


    max_indexes[j] = get_max_indexes (row);

  }
  GST_LOG_OBJECT (vi, "The end");

  output = concatenate_chars (max_indexes);

  printf ("The phrase is: %s\n", output);
  GST_LOG_OBJECT (vi, "The prhase is %s", output);

  free (output);
  return TRUE;

}

static gboolean
gst_rosetta_start (GstVideoInference * vi)
{
  GST_INFO_OBJECT (vi, "Starting Rosetta");

  return TRUE;
}

static gboolean
gst_rosetta_stop (GstVideoInference * vi)
{
  GST_INFO_OBJECT (vi, "Stopping Rosetta");

  return TRUE;
}
