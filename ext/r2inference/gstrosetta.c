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
 * based on the ResNet architecture on incoming image frames and extract
 * the characters from it.
 *
 * <refsect2>
 * <title>Source</title>
 * This element is based on the TensorFlow Lite Hub Rosetta Google
 * Colaboratory script:
 * https://tfhub.dev/tulasiram58827/lite-model/rosetta/dr/1
 * </refsect2>
 */

#include "gstrosetta.h"

#include "gst/r2inference/gstinferencedebug.h"
#include "gst/r2inference/gstinferencemeta.h"
#include "gst/r2inference/gstinferencepostprocess.h"
#include "gst/r2inference/gstinferencepreprocess.h"

GST_DEBUG_CATEGORY_STATIC (gst_rosetta_debug_category);
#define GST_CAT_DEFAULT gst_rosetta_debug_category

#define BLANK 0
#define DEFAULT_MODEL_CHANNELS 1
#define DEFAULT_DATA_MEAN 127.5
#define DEFAULT_DATA_OFFSET -1
#define MODEL_OUTPUT_ROWS 26
#define MODEL_OUTPUT_COLS 37

/* prototypes */
static gboolean gst_rosetta_preprocess (GstVideoInference * vi,
    GstVideoFrame * inframe, GstVideoFrame * outframe);

static gboolean
gst_rosetta_postprocess (GstVideoInference * vi,
    const gpointer prediction, gsize predsize, GstMeta * meta_model,
    GstVideoInfo * info_model, gboolean * valid_prediction,
    gchar ** labels_list, gint num_labels);

gint get_max_indices (gfloat row[MODEL_OUTPUT_COLS]);

gchar *concatenate_chars (gint max_indices[MODEL_OUTPUT_ROWS]);
static gboolean gst_rosetta_start (GstVideoInference * vi);
static gboolean gst_rosetta_stop (GstVideoInference * vi);

#define CAPS							\
  "video/x-raw, "						\
  "width=100, "							\
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
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  GstVideoInferenceClass *vi_class = GST_VIDEO_INFERENCE_CLASS (klass);
  gst_element_class_add_static_pad_template (element_class,
      &sink_model_factory);
  gst_element_class_add_static_pad_template (element_class, &src_model_factory);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "Rosetta", "Filter",
      "Infers characters from an incoming image",
      "Edgar Chaves <edgar.chaves@ridgerun.com>\n\t\t\t"
      "   Luis Leon <luis.leon@ridgerun.com>");

  vi_class->preprocess = GST_DEBUG_FUNCPTR (gst_rosetta_preprocess);
  vi_class->postprocess = GST_DEBUG_FUNCPTR (gst_rosetta_postprocess);
  vi_class->start = GST_DEBUG_FUNCPTR (gst_rosetta_start);
  vi_class->stop = GST_DEBUG_FUNCPTR (gst_rosetta_stop);
}


static void
gst_rosetta_init (GstRosetta * rosetta)
{
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

  GST_LOG_OBJECT (rosetta, "Input frame dimensions w = %i , h = %i", width,
      height);
  return gst_normalize_gray_image (inframe, outframe, DEFAULT_DATA_MEAN,
      DEFAULT_DATA_OFFSET, DEFAULT_MODEL_CHANNELS);
}

gint
get_max_indices (gfloat row[MODEL_OUTPUT_COLS])
{
  gfloat largest_probability = row[0];
  gint temp_max_index = 0;
  for (int i = 0; i < MODEL_OUTPUT_COLS; ++i) {
    if (largest_probability < row[i]) {
      largest_probability = row[i];
      temp_max_index = i;
    }
  }
  return temp_max_index;
}

/**
 * NOTE: After using this function, please free the returned
 *       gchar with g_free(), due to this function is transfering
 *       the ownership of the allocated memory.
 **/
gchar *
concatenate_chars (int max_indices[MODEL_OUTPUT_ROWS])
{
  gint i = 0;
  gint counter = 0;
  gchar *final_phrase = NULL;
  const gchar chars[MODEL_OUTPUT_COLS] =
      { '_', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c',
    'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q',
    'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'
  };
  /* Instead of using g_malloc() & memset g_strnfill(), will create
   * the memory allocation and fill the string with empty spaces. 
   */
  final_phrase = g_strnfill (MODEL_OUTPUT_ROWS + 1, ' ');

  for (i = 0; i < MODEL_OUTPUT_ROWS; ++i) {

    /* Checking if the actual max index value is different from '_' character
     * and also, checking if i is greater than 0, and finally, checking
     * if the actual max index is equal from the previous one.
     */
    if (BLANK != max_indices[i] && !(0 < i
            && (max_indices[i - 1] == max_indices[i]))) {
      final_phrase[counter] = chars[max_indices[i]];
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

  gint max_indices[MODEL_OUTPUT_ROWS];
  gfloat row[MODEL_OUTPUT_COLS];
  gint index = 0;
  const gfloat *pred = NULL;
  gchar *output = NULL;
  GstInferenceMeta *imeta = NULL;
  GstInferencePrediction *root = NULL;

  g_return_val_if_fail (vi, FALSE);
  g_return_val_if_fail (prediction, FALSE);
  g_return_val_if_fail (meta_model, FALSE);
  g_return_val_if_fail (info_model, FALSE);

  GST_LOG_OBJECT (rosetta, "Rosetta Postprocess");

  imeta = (GstInferenceMeta *) meta_model;
  rosetta = GST_ROSETTA (vi);
  root = imeta->prediction;
  if (!root) {
    GST_ERROR_OBJECT (vi, "Prediction is not part of the Inference Meta");
    return FALSE;
  }
  pred = (const gfloat *) prediction;
  GST_LOG_OBJECT (vi, "Predicting...");

  for (int j = 0; j < MODEL_OUTPUT_ROWS; ++j) {
    for (int i = 0; i < MODEL_OUTPUT_COLS; ++i) {
      row[i] = pred[index];
      ++index;
    }
    max_indices[j] = get_max_indices (row);
  }
  GST_LOG_OBJECT (vi, "Rosetta prediction is done");

  output = concatenate_chars (max_indices);

  GST_LOG_OBJECT (vi, "The phrase is %s", output);

  g_free (output);
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
