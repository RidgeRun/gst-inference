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
 * SECTION:element-gsttinyyolov3
 *
 * The tinyyolov3 element allows the user to infer/execute a pretrained model
 * based on the TinyYolo architecture on incoming image frames.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v videotestsrc ! tinyyolov3 ! xvimagesink
 * ]|
 * Process video frames from the camera using a TinyYolo model.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gsttinyyolov3.h"
#include "gst/r2inference/gstinferencemeta.h"
#include <string.h>
#include "gst/r2inference/gstinferencepreprocess.h"
#include "gst/r2inference/gstinferencepostprocess.h"
#include "gst/r2inference/gstinferencedebug.h"

GST_DEBUG_CATEGORY_STATIC (gst_tinyyolov3_debug_category);
#define GST_CAT_DEFAULT gst_tinyyolov3_debug_category

#define MODEL_CHANNELS 3

/* Objectness threshold */
#define MAX_OBJ_THRESH 1
#define MIN_OBJ_THRESH 0
#define DEFAULT_OBJ_THRESH 0.50
/* Class probability threshold */
#define MAX_PROB_THRESH 1
#define MIN_PROB_THRESH 0
#define DEFAULT_PROB_THRESH 0.50
/* Intersection over union threshold */
#define MAX_IOU_THRESH 1
#define MIN_IOU_THRESH 0
#define DEFAULT_IOU_THRESH 0.40

/* prototypes */
static void gst_tinyyolov3_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_tinyyolov3_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);

static gboolean gst_tinyyolov3_preprocess (GstVideoInference * vi,
    GstVideoFrame * inframe, GstVideoFrame * outframe);
static gboolean
gst_tinyyolov3_postprocess (GstVideoInference * vi, const gpointer prediction,
    gsize predsize, GstMeta * meta_model[2], GstVideoInfo * info_model,
    gboolean * valid_prediction, gchar ** labels_list, gint num_labels);
static gboolean
gst_tinyyolov3_postprocess_old (GstVideoInference * vi,
    const gpointer prediction, gsize predsize, GstMeta * meta_model,
    GstVideoInfo * info_model, gboolean * valid_prediction);
static gboolean
gst_tinyyolov3_postprocess_new (GstVideoInference * vi,
    const gpointer prediction, gsize predsize, GstMeta * meta_model,
    GstVideoInfo * info_model, gboolean * valid_prediction,
    gchar ** labels_list, gint num_labels);
static gboolean gst_tinyyolov3_start (GstVideoInference * vi);
static gboolean gst_tinyyolov3_stop (GstVideoInference * vi);

enum
{
  PROP_0,
  PROP_OBJ_THRESH,
  PROP_PROB_THRESH,
  PROP_IOU_THRESH,
};

/* pad templates */
#define CAPS								\
  "video/x-raw, "							\
  "width=416, "								\
  "height=416, "							\
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

struct _GstTinyyolov3
{
  GstVideoInference parent;

  gdouble obj_thresh;
  gdouble prob_thresh;
  gdouble iou_thresh;
};

struct _GstTinyyolov3Class
{
  GstVideoInferenceClass parent;
};

/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstTinyyolov3, gst_tinyyolov3,
    GST_TYPE_VIDEO_INFERENCE,
    GST_DEBUG_CATEGORY_INIT (gst_tinyyolov3_debug_category, "tinyyolov3", 0,
        "debug category for tinyyolov3 element"));

static void
gst_tinyyolov3_class_init (GstTinyyolov3Class * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  GstVideoInferenceClass *vi_class = GST_VIDEO_INFERENCE_CLASS (klass);

  gst_element_class_add_static_pad_template (element_class,
      &sink_model_factory);
  gst_element_class_add_static_pad_template (element_class, &src_model_factory);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "tinyyolov3", "Filter",
      "Infers incoming image frames using a pretrained TinyYolo model",
      "Carlos Rodriguez <carlos.rodriguez@ridgerun.com> \n\t\t\t"
      "   Jose Jimenez <jose.jimenez@ridgerun.com> \n\t\t\t"
      "   Michael Gruner <michael.gruner@ridgerun.com> \n\t\t\t"
      "   Carlos Aguero <carlos.aguero@ridgerun.com> \n\t\t\t"
      "   Miguel Taylor <miguel.taylor@ridgerun.com> \n\t\t\t"
      "   Greivin Fallas <greivin.fallas@ridgerun.com>");

  gobject_class->set_property = gst_tinyyolov3_set_property;
  gobject_class->get_property = gst_tinyyolov3_get_property;

  g_object_class_install_property (gobject_class, PROP_OBJ_THRESH,
      g_param_spec_double ("object-threshold", "obj-thresh",
          "Objectness threshold", MIN_OBJ_THRESH, MAX_OBJ_THRESH,
          DEFAULT_OBJ_THRESH, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_PROB_THRESH,
      g_param_spec_double ("probability-threshold", "prob-thresh",
          "Class probability threshold", MIN_PROB_THRESH, MAX_PROB_THRESH,
          DEFAULT_PROB_THRESH, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_IOU_THRESH,
      g_param_spec_double ("iou-threshold", "iou-thresh",
          "Intersection over union threshold to merge similar boxes",
          MIN_IOU_THRESH, MAX_IOU_THRESH, DEFAULT_IOU_THRESH,
          G_PARAM_READWRITE));

  vi_class->start = GST_DEBUG_FUNCPTR (gst_tinyyolov3_start);
  vi_class->stop = GST_DEBUG_FUNCPTR (gst_tinyyolov3_stop);
  vi_class->preprocess = GST_DEBUG_FUNCPTR (gst_tinyyolov3_preprocess);
  vi_class->postprocess = GST_DEBUG_FUNCPTR (gst_tinyyolov3_postprocess);
  vi_class->inference_meta_info = gst_detection_meta_get_info ();
}

static void
gst_tinyyolov3_init (GstTinyyolov3 * tinyyolov3)
{
  tinyyolov3->obj_thresh = DEFAULT_OBJ_THRESH;
  tinyyolov3->prob_thresh = DEFAULT_PROB_THRESH;
  tinyyolov3->iou_thresh = DEFAULT_IOU_THRESH;
}

static void
gst_tinyyolov3_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstTinyyolov3 *tinyyolov3 = GST_TINYYOLOV3 (object);

  GST_DEBUG_OBJECT (tinyyolov3, "set_property");

  switch (property_id) {
    case PROP_OBJ_THRESH:
      tinyyolov3->obj_thresh = g_value_get_double (value);
      GST_DEBUG_OBJECT (tinyyolov3,
          "Changed objectness threshold to %lf", tinyyolov3->obj_thresh);
      break;
    case PROP_PROB_THRESH:
      tinyyolov3->prob_thresh = g_value_get_double (value);
      GST_DEBUG_OBJECT (tinyyolov3,
          "Changed probability threshold to %lf", tinyyolov3->prob_thresh);
      break;
    case PROP_IOU_THRESH:
      tinyyolov3->iou_thresh = g_value_get_double (value);
      GST_DEBUG_OBJECT (tinyyolov3,
          "Changed intersection over union threshold to %lf",
          tinyyolov3->iou_thresh);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
gst_tinyyolov3_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstTinyyolov3 *tinyyolov3 = GST_TINYYOLOV3 (object);

  GST_DEBUG_OBJECT (tinyyolov3, "get_property");

  switch (property_id) {
    case PROP_OBJ_THRESH:
      g_value_set_double (value, tinyyolov3->obj_thresh);
      break;
    case PROP_PROB_THRESH:
      g_value_set_double (value, tinyyolov3->prob_thresh);
      break;
    case PROP_IOU_THRESH:
      g_value_set_double (value, tinyyolov3->iou_thresh);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static gboolean
gst_tinyyolov3_preprocess (GstVideoInference * vi,
    GstVideoFrame * inframe, GstVideoFrame * outframe)
{
  GST_LOG_OBJECT (vi, "Preprocess");
  return gst_pixel_to_float (inframe, outframe, MODEL_CHANNELS);
}

static gboolean
gst_tinyyolov3_postprocess (GstVideoInference * vi, const gpointer prediction,
    gsize predsize, GstMeta * meta_model[2], GstVideoInfo * info_model,
    gboolean * valid_prediction, gchar ** labels_list, gint num_labels)
{
  gboolean ret = TRUE;

  g_return_val_if_fail (vi, FALSE);
  g_return_val_if_fail (prediction, FALSE);
  g_return_val_if_fail (meta_model, FALSE);
  g_return_val_if_fail (info_model, FALSE);
  g_return_val_if_fail (valid_prediction, FALSE);

  ret &=
      gst_tinyyolov3_postprocess_old (vi, prediction, predsize, meta_model[0],
      info_model, valid_prediction);
  ret &=
      gst_tinyyolov3_postprocess_new (vi, prediction, predsize, meta_model[1],
      info_model, valid_prediction, labels_list, num_labels);

  return TRUE;
}

static gboolean
gst_tinyyolov3_postprocess_old (GstVideoInference * vi,
    const gpointer prediction, gsize predsize, GstMeta * meta_model,
    GstVideoInfo * info_model, gboolean * valid_prediction)
{
  GstTinyyolov3 *tinyyolov3;
  gdouble *probabilities = NULL;

  GstDetectionMeta *detect_meta = (GstDetectionMeta *) meta_model;

  g_return_val_if_fail (detect_meta, FALSE);

  GST_LOG_OBJECT (vi, "Postprocess");
  detect_meta->num_boxes = 0;
  tinyyolov3 = GST_TINYYOLOV3 (vi);

  gst_create_boxes (vi, prediction, valid_prediction,
      &detect_meta->boxes, &detect_meta->num_boxes, tinyyolov3->obj_thresh,
      tinyyolov3->prob_thresh, tinyyolov3->iou_thresh, &probabilities);

  gst_inference_print_boxes (vi, gst_tinyyolov3_debug_category, detect_meta);

  *valid_prediction = (detect_meta->num_boxes > 0) ? TRUE : FALSE;

  return TRUE;
}

static gboolean
gst_tinyyolov3_postprocess_new (GstVideoInference * vi,
    const gpointer prediction, gsize predsize, GstMeta * meta_model,
    GstVideoInfo * info_model, gboolean * valid_prediction,
    gchar ** labels_list, gint num_labels)
{
  GstTinyyolov3 *tinyyolov3 = NULL;
  GstInferenceMeta *imeta = NULL;
  BBox *boxes = NULL;
  gint num_boxes, i;
  gdouble *probabilities = NULL;

  g_return_val_if_fail (vi != NULL, FALSE);
  g_return_val_if_fail (meta_model != NULL, FALSE);
  g_return_val_if_fail (info_model != NULL, FALSE);

  imeta = (GstInferenceMeta *) meta_model;
  tinyyolov3 = GST_TINYYOLOV3 (vi);

  GST_LOG_OBJECT (tinyyolov3, "Postprocess Meta");

  /* Create boxes from prediction data */
  gst_create_boxes (vi, prediction, valid_prediction,
      &boxes, &num_boxes, tinyyolov3->obj_thresh,
      tinyyolov3->prob_thresh, tinyyolov3->iou_thresh, &probabilities);

  GST_LOG_OBJECT (tinyyolov3, "Number of predictions: %d", num_boxes);

  if (NULL == imeta->prediction) {
    imeta->prediction = gst_inference_prediction_new ();
    imeta->prediction->bbox.width = info_model->width;
    imeta->prediction->bbox.height = info_model->height;
  }

  for (i = 0; i < num_boxes; i++) {
    GstInferencePrediction *pred =
        gst_create_prediction_from_box (vi, &boxes[i], labels_list, num_labels,
        probabilities);
    gst_inference_prediction_append (imeta->prediction, pred);
  }

  /* Free boxes after creation */
  g_free (boxes);

  /* Log predictions */
  gst_inference_print_predictions (vi, gst_tinyyolov3_debug_category, imeta);

  *valid_prediction = (num_boxes > 0) ? TRUE : FALSE;

  return TRUE;
}

static gboolean
gst_tinyyolov3_start (GstVideoInference * vi)
{
  GST_INFO_OBJECT (vi, "Starting TinyYolo");

  return TRUE;
}

static gboolean
gst_tinyyolov3_stop (GstVideoInference * vi)
{
  GST_INFO_OBJECT (vi, "Stopping TinyYolo");

  return TRUE;
}
