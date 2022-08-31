/*
 * GStreamer
 * Copyright (C) 2021 RidgeRun <support@ridgerun.com>
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
 * SECTION:element-gstmobilenetv2ssd
 *
 * The mobilenetv2ssd element allows the user to infer/execute a pretrained 
 * model based on the MobilenetV2 + SSD architecture on incoming image frames.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstmobilenetv2ssd.h"

#include "gst/r2inference/gstinferencedebug.h"
#include "gst/r2inference/gstinferencemeta.h"
#include "gst/r2inference/gstinferencepostprocess.h"
#include "gst/r2inference/gstinferencepreprocess.h"

GST_DEBUG_CATEGORY_STATIC (gst_mobilenetv2ssd_debug_category);
#define GST_CAT_DEFAULT gst_mobilenetv2ssd_debug_category

#define MODEL_CHANNELS 3
#define MEAN 127.5
#define STD 1 / MEAN

/* Class probability threshold */
#define MAX_PROB_THRESH 1
#define MIN_PROB_THRESH 0
#define DEFAULT_PROB_THRESH 0.50
/* Intersection over union threshold */
#define MAX_IOU_THRESH 1
#define MIN_IOU_THRESH 0
#define DEFAULT_IOU_THRESH 0.40

#define TOTAL_CLASSES 90
#define LOCATION_PARAMS 4

/* prototypes */
static void gst_mobilenetv2ssd_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_mobilenetv2ssd_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static gboolean gst_mobilenetv2ssd_preprocess (GstVideoInference * vi,
    GstVideoFrame * inframe, GstVideoFrame * outframe);
static gboolean
gst_mobilenetv2ssd_postprocess (GstVideoInference * vi,
    const gpointer prediction, gsize predsize, GstMeta * meta_model,
    GstVideoInfo * info_model, gboolean * valid_prediction,
    gchar ** labels_list, gint num_labels);
static gint
gst_mobilenetv2ssd_get_boxes_from_prediction (GstMobilenetv2ssd *
    mobilenetv2ssd, const gfloat * prediction, gint num_boxes, gint img_width,
    gint img_height, BBox * boxes, gdouble ** probabilities);

enum
{
  PROP_0,
  PROP_PROB_THRESH,
  PROP_IOU_THRESH
};

/* pad templates */
#define CAPS                             \
  "video/x-raw, "                        \
  "width={ 300, 192 }, "                 \
  "height={ 300, 192 }, "                \
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

struct _GstMobilenetv2ssd
{
  GstVideoInference parent;

  gdouble prob_thresh;
  gdouble iou_thresh;
};

struct _GstMobilenetv2ssdClass
{
  GstVideoInferenceClass parent;
};

/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstMobilenetv2ssd, gst_mobilenetv2ssd,
    GST_TYPE_VIDEO_INFERENCE,
    GST_DEBUG_CATEGORY_INIT (gst_mobilenetv2ssd_debug_category,
        "mobilenetv2ssd", 0, "debug category for mobilenetv2ssd element"));

static void
gst_mobilenetv2ssd_class_init (GstMobilenetv2ssdClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  GstVideoInferenceClass *vi_class = GST_VIDEO_INFERENCE_CLASS (klass);

  gst_element_class_add_static_pad_template (element_class,
      &sink_model_factory);
  gst_element_class_add_static_pad_template (element_class, &src_model_factory);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "mobilenetv2ssd", "Filter",
      "Infers incoming image frames using a pretrained MobilenetV2 + SSD model",
      "Jimena Salas <jimena.salas@ridgerun.com>");

  gobject_class->set_property = gst_mobilenetv2ssd_set_property;
  gobject_class->get_property = gst_mobilenetv2ssd_get_property;

  g_object_class_install_property (gobject_class, PROP_PROB_THRESH,
      g_param_spec_double ("prob-thresh", "Probability threshold",
          "Class probability threshold", MIN_PROB_THRESH, MAX_PROB_THRESH,
          DEFAULT_PROB_THRESH, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_IOU_THRESH,
      g_param_spec_double ("iou-thresh", "Intersection over union threshold",
          "Intersection over union threshold to merge similar boxes",
          MIN_IOU_THRESH, MAX_IOU_THRESH, DEFAULT_IOU_THRESH,
          G_PARAM_READWRITE));

  vi_class->preprocess = GST_DEBUG_FUNCPTR (gst_mobilenetv2ssd_preprocess);
  vi_class->postprocess = GST_DEBUG_FUNCPTR (gst_mobilenetv2ssd_postprocess);
}

static void
gst_mobilenetv2ssd_init (GstMobilenetv2ssd * mobilenetv2ssd)
{
  mobilenetv2ssd->prob_thresh = DEFAULT_PROB_THRESH;
  mobilenetv2ssd->iou_thresh = DEFAULT_IOU_THRESH;
}

static gboolean
gst_mobilenetv2ssd_preprocess (GstVideoInference * vi,
    GstVideoFrame * inframe, GstVideoFrame * outframe)
{
  g_return_val_if_fail (vi, FALSE);
  g_return_val_if_fail (inframe, FALSE);
  g_return_val_if_fail (outframe, FALSE);

  GST_LOG_OBJECT (vi, "Preprocess");

  return gst_normalize (inframe, outframe, MEAN, STD, MODEL_CHANNELS);
}

static gint
gst_mobilenetv2ssd_get_boxes_from_prediction (GstMobilenetv2ssd *
    mobilenetv2ssd, const gfloat * prediction, gint num_boxes, gint img_width,
    gint img_height, BBox * boxes, gdouble ** probabilities)
{
  gint cur_box = 0;
  gdouble left = 0, top = 0, right = 0, bottom = 0;
  gint i_box = 0, i_prob = 0, i_location = 0, i_label = 0;
  gdouble prob = 0;
  gdouble prob_thresh = 0;
  gdouble iou_thresh = 0;

  g_return_val_if_fail (mobilenetv2ssd, cur_box);
  g_return_val_if_fail (prediction, cur_box);
  g_return_val_if_fail (boxes, cur_box);
  g_return_val_if_fail (probabilities, cur_box);

  GST_OBJECT_LOCK (mobilenetv2ssd);
  prob_thresh = mobilenetv2ssd->prob_thresh;
  iou_thresh = mobilenetv2ssd->iou_thresh;
  GST_OBJECT_UNLOCK (mobilenetv2ssd);

  for (i_box = 0; i_box < num_boxes; i_box++) {
    /* Here prediction has the 4 concatenated tensors in the order
       [locations, labels, probabilities, num_boxes], so we compute the indices 
       to access each one of them */
    i_location = i_box * LOCATION_PARAMS;
    i_label = i_box + (num_boxes * LOCATION_PARAMS);
    i_prob = i_label + num_boxes;
    prob = prediction[i_prob];

    if (prob > prob_thresh) {
      BBox result = { 0 };

      top = prediction[i_location] * img_height;
      left = prediction[i_location + 1] * img_width;
      bottom = prediction[i_location + 2] * img_height;
      right = prediction[i_location + 3] * img_width;

      result.x = left;
      result.y = top;
      result.width = right - left;
      result.height = bottom - top;
      result.label = prediction[i_label];
      result.prob = prob;
      probabilities[cur_box][result.label] = result.prob;
      boxes[cur_box] = result;
      cur_box++;
    }
  }

  gst_remove_duplicated_boxes (iou_thresh, boxes, &cur_box);

  return cur_box;
}

static gboolean
gst_mobilenetv2ssd_postprocess (GstVideoInference * vi,
    const gpointer prediction, gsize predsize, GstMeta * meta_model,
    GstVideoInfo * info_model, gboolean * valid_prediction,
    gchar ** labels_list, gint num_labels)
{
  GstMobilenetv2ssd *mobilenetv2ssd = NULL;
  GstInferenceMeta *imeta = NULL;
  BBox *boxes = NULL;
  gdouble **probabilities = NULL;
  gint total_boxes = 0;
  gint valid_boxes = 0;
  gint i = 0;
  gboolean ret = TRUE;
  const gfloat *pred = NULL;

  g_return_val_if_fail (vi, FALSE);
  g_return_val_if_fail (prediction, FALSE);
  g_return_val_if_fail (meta_model, FALSE);
  g_return_val_if_fail (info_model, FALSE);
  g_return_val_if_fail (valid_prediction, FALSE);

  GST_LOG_OBJECT (vi, "Postprocess");

  pred = (const gfloat *) prediction;
  mobilenetv2ssd = GST_MOBILENETV2SSD (vi);
  /* The ssd mobilenetv2 model has 4 output tensors:
     0: [N * 4] tensor with the location of the N bounding boxes (top-left and
     right-bottom corners). This tensor is flattened so the output we get here
     is of shape [N * 4]
     1: [N] tensor with the number of labels for the N bounding boxes
     2: [N] tensor with the probabilities of those N labels
     3: [1] tensor with the number of detected boxes 
     They are all concatenated here in a 1D array in row-major order.
   */
  total_boxes = pred[predsize / sizeof (gfloat) - 1];

  GST_LOG_OBJECT (mobilenetv2ssd, "Number of total predictions: %d",
      total_boxes);

  if (0 == total_boxes) {
    goto out;
  }

  imeta = (GstInferenceMeta *) meta_model;

  boxes = g_malloc (total_boxes * sizeof (BBox));
  probabilities = g_malloc (total_boxes * sizeof (gdouble *));
  /* We create an array of probabilities of the total classes size to have
     compatibility with the other classification models, but this model only
     outputs 1 label and 1 probability per bounding box
   */
  for (i = 0; i < total_boxes; i++) {
    probabilities[i] = g_malloc0 (TOTAL_CLASSES * sizeof (gdouble));
  }

  valid_boxes =
      gst_mobilenetv2ssd_get_boxes_from_prediction (mobilenetv2ssd, pred,
      total_boxes, info_model->width, info_model->height, boxes, probabilities);

  GST_LOG_OBJECT (mobilenetv2ssd, "Number of valid predictions: %d",
      valid_boxes);

  if (0 == valid_boxes) {
    goto free;
  }

  if (NULL == imeta->prediction) {
    imeta->prediction = gst_inference_prediction_new ();
    imeta->prediction->bbox.width = info_model->width;
    imeta->prediction->bbox.height = info_model->height;
  }

  for (i = 0; i < valid_boxes; i++) {
    GstInferencePrediction *pred =
        gst_create_prediction_from_box (vi, &boxes[i], labels_list, num_labels,
        probabilities[i]);
    gst_inference_prediction_append (imeta->prediction, pred);
  }

  gst_inference_print_predictions (vi, gst_mobilenetv2ssd_debug_category,
      imeta);

free:
  /* Free boxes after creation */
  g_free (boxes);
  for (i = 0; i < total_boxes; i++) {
    g_free (probabilities[i]);
  }
  g_free (probabilities);

out:
  *valid_prediction = (valid_boxes > 0) ? TRUE : FALSE;

  return ret;
}

static void
gst_mobilenetv2ssd_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstMobilenetv2ssd *mobilenetv2ssd = GST_MOBILENETV2SSD (object);

  GST_DEBUG_OBJECT (mobilenetv2ssd, "set_property");

  GST_OBJECT_LOCK (mobilenetv2ssd);
  switch (property_id) {
    case PROP_PROB_THRESH:
      mobilenetv2ssd->prob_thresh = g_value_get_double (value);
      GST_DEBUG_OBJECT (mobilenetv2ssd,
          "Changed probability threshold to %lf", mobilenetv2ssd->prob_thresh);
      break;
    case PROP_IOU_THRESH:
      mobilenetv2ssd->iou_thresh = g_value_get_double (value);
      GST_DEBUG_OBJECT (mobilenetv2ssd,
          "Changed intersection over union threshold to %lf",
          mobilenetv2ssd->prob_thresh);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK (mobilenetv2ssd);
}

static void
gst_mobilenetv2ssd_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstMobilenetv2ssd *mobilenetv2ssd = GST_MOBILENETV2SSD (object);

  GST_DEBUG_OBJECT (mobilenetv2ssd, "get_property");

  GST_OBJECT_LOCK (mobilenetv2ssd);
  switch (property_id) {
    case PROP_PROB_THRESH:
      g_value_set_double (value, mobilenetv2ssd->prob_thresh);
      break;
    case PROP_IOU_THRESH:
      g_value_set_double (value, mobilenetv2ssd->iou_thresh);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK (mobilenetv2ssd);
}
