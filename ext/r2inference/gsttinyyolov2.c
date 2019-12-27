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
 * SECTION:element-gsttinyyolov2
 *
 * The tinyyolov2 element allows the user to infer/execute a pretrained model
 * based on the TinyYolo architecture on incoming image frames.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v videotestsrc ! tinyyolov2 ! xvimagesink
 * ]|
 * Process video frames from the camera using a TinyYolo model.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gsttinyyolov2.h"
#include "gst/r2inference/gstinferencemeta.h"
#include <string.h>
#include <math.h>
#include "gst/r2inference/gstinferencepreprocess.h"
#include "gst/r2inference/gstinferencepostprocess.h"
#include "gst/r2inference/gstinferencedebug.h"

GST_DEBUG_CATEGORY_STATIC (gst_tinyyolov2_debug_category);
#define GST_CAT_DEFAULT gst_tinyyolov2_debug_category

#define MEAN 0
#define STD 1/255.0
#define MODEL_CHANNELS 3

/* Objectness threshold */
#define MAX_OBJ_THRESH 1
#define MIN_OBJ_THRESH 0
#define DEFAULT_OBJ_THRESH 0.08
/* Class probability threshold */
#define MAX_PROB_THRESH 1
#define MIN_PROB_THRESH 0
#define DEFAULT_PROB_THRESH 0.08
/* Intersection over union threshold */
#define MAX_IOU_THRESH 1
#define MIN_IOU_THRESH 0
#define DEFAULT_IOU_THRESH 0.30

const gfloat box_anchors[] =
    { 1.08, 1.19, 3.42, 4.41, 6.63, 11.38, 9.42, 5.11, 16.62, 10.52 };

/* prototypes */
static void gst_tinyyolov2_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_tinyyolov2_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_tinyyolov2_dispose (GObject * object);
static void gst_tinyyolov2_finalize (GObject * object);

static gboolean gst_tinyyolov2_preprocess (GstVideoInference * vi,
    GstVideoFrame * inframe, GstVideoFrame * outframe);
static gboolean
gst_tinyyolov2_postprocess (GstVideoInference * vi, const gpointer prediction,
    gsize predsize, GstMeta * meta_model[2], GstVideoInfo * info_model,
    gboolean * valid_prediction);
static gboolean
gst_tinyyolov2_postprocess_old (GstVideoInference * vi,
    const gpointer prediction, gsize predsize, GstMeta * meta_model,
    GstVideoInfo * info_model, gboolean * valid_prediction);
static gboolean
gst_tinyyolov2_postprocess_new (GstVideoInference * vi,
    const gpointer prediction, gsize predsize, GstMeta * meta_model,
    GstVideoInfo * info_model, gboolean * valid_prediction);
static gboolean gst_tinyyolov2_start (GstVideoInference * vi);
static gboolean gst_tinyyolov2_stop (GstVideoInference * vi);

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
  "width=416, "							\
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

struct _GstTinyyolov2
{
  GstVideoInference parent;

  gdouble obj_thresh;
  gdouble prob_thresh;
  gdouble iou_thresh;
};

struct _GstTinyyolov2Class
{
  GstVideoInferenceClass parent;
};

/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstTinyyolov2, gst_tinyyolov2,
    GST_TYPE_VIDEO_INFERENCE,
    GST_DEBUG_CATEGORY_INIT (gst_tinyyolov2_debug_category, "tinyyolov2", 0,
        "debug category for tinyyolov2 element"));

static void
gst_tinyyolov2_class_init (GstTinyyolov2Class * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  GstVideoInferenceClass *vi_class = GST_VIDEO_INFERENCE_CLASS (klass);

  gst_element_class_add_static_pad_template (element_class,
      &sink_model_factory);
  gst_element_class_add_static_pad_template (element_class, &src_model_factory);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "tinyyolov2", "Filter",
      "Infers incoming image frames using a pretrained TinyYolo model",
      "Carlos Rodriguez <carlos.rodriguez@ridgerun.com> \n\t\t\t"
      "   Jose Jimenez <jose.jimenez@ridgerun.com> \n\t\t\t"
      "   Michael Gruner <michael.gruner@ridgerun.com> \n\t\t\t"
      "   Carlos Aguero <carlos.aguero@ridgerun.com> \n\t\t\t"
      "   Miguel Taylor <miguel.taylor@ridgerun.com> \n\t\t\t"
      "   Greivin Fallas <greivin.fallas@ridgerun.com>");

  gobject_class->set_property = gst_tinyyolov2_set_property;
  gobject_class->get_property = gst_tinyyolov2_get_property;
  gobject_class->dispose = gst_tinyyolov2_dispose;
  gobject_class->finalize = gst_tinyyolov2_finalize;

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

  vi_class->start = GST_DEBUG_FUNCPTR (gst_tinyyolov2_start);
  vi_class->stop = GST_DEBUG_FUNCPTR (gst_tinyyolov2_stop);
  vi_class->preprocess = GST_DEBUG_FUNCPTR (gst_tinyyolov2_preprocess);
  vi_class->postprocess = GST_DEBUG_FUNCPTR (gst_tinyyolov2_postprocess);
  vi_class->inference_meta_info = gst_detection_meta_get_info ();
}

static void
gst_tinyyolov2_init (GstTinyyolov2 * tinyyolov2)
{
  tinyyolov2->obj_thresh = DEFAULT_OBJ_THRESH;
  tinyyolov2->prob_thresh = DEFAULT_PROB_THRESH;
  tinyyolov2->iou_thresh = DEFAULT_IOU_THRESH;
}

void
gst_tinyyolov2_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstTinyyolov2 *tinyyolov2 = GST_TINYYOLOV2 (object);

  GST_DEBUG_OBJECT (tinyyolov2, "set_property");

  switch (property_id) {
    case PROP_OBJ_THRESH:
      tinyyolov2->obj_thresh = g_value_get_double (value);
      GST_DEBUG_OBJECT (tinyyolov2,
          "Changed objectness threshold to %lf", tinyyolov2->obj_thresh);
      break;
    case PROP_PROB_THRESH:
      tinyyolov2->prob_thresh = g_value_get_double (value);
      GST_DEBUG_OBJECT (tinyyolov2,
          "Changed probability threshold to %lf", tinyyolov2->prob_thresh);
      break;
    case PROP_IOU_THRESH:
      tinyyolov2->iou_thresh = g_value_get_double (value);
      GST_DEBUG_OBJECT (tinyyolov2,
          "Changed intersection over union threshold to %lf",
          tinyyolov2->iou_thresh);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_tinyyolov2_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstTinyyolov2 *tinyyolov2 = GST_TINYYOLOV2 (object);

  GST_DEBUG_OBJECT (tinyyolov2, "get_property");

  switch (property_id) {
    case PROP_OBJ_THRESH:
      g_value_set_double (value, tinyyolov2->obj_thresh);
      break;
    case PROP_PROB_THRESH:
      g_value_set_double (value, tinyyolov2->prob_thresh);
      break;
    case PROP_IOU_THRESH:
      g_value_set_double (value, tinyyolov2->iou_thresh);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_tinyyolov2_dispose (GObject * object)
{
  GstTinyyolov2 *tinyyolov2 = GST_TINYYOLOV2 (object);

  GST_DEBUG_OBJECT (tinyyolov2, "dispose");

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (gst_tinyyolov2_parent_class)->dispose (object);
}

void
gst_tinyyolov2_finalize (GObject * object)
{
  GstTinyyolov2 *tinyyolov2 = GST_TINYYOLOV2 (object);

  GST_DEBUG_OBJECT (tinyyolov2, "finalize");

  /* clean up object here */

  G_OBJECT_CLASS (gst_tinyyolov2_parent_class)->finalize (object);
}

static gboolean
gst_tinyyolov2_preprocess (GstVideoInference * vi,
    GstVideoFrame * inframe, GstVideoFrame * outframe)
{
  GST_LOG_OBJECT (vi, "Preprocess");
  return gst_normalize (inframe, outframe, MEAN, STD, MODEL_CHANNELS);
}

static gboolean
gst_tinyyolov2_postprocess (GstVideoInference * vi, const gpointer prediction,
    gsize predsize, GstMeta * meta_model[2], GstVideoInfo * info_model,
    gboolean * valid_prediction)
{
  gboolean ret = TRUE;

  ret &=
      gst_tinyyolov2_postprocess_old (vi, prediction, predsize, meta_model[0],
      info_model, valid_prediction);
  ret &=
      gst_tinyyolov2_postprocess_new (vi, prediction, predsize, meta_model[1],
      info_model, valid_prediction);

  return TRUE;
}

static gboolean
gst_tinyyolov2_postprocess_old (GstVideoInference * vi,
    const gpointer prediction, gsize predsize, GstMeta * meta_model,
    GstVideoInfo * info_model, gboolean * valid_prediction)
{
  GstTinyyolov2 *tinyyolov2;
  GstDetectionMeta *detect_meta = (GstDetectionMeta *) meta_model;
  GST_LOG_OBJECT (vi, "Postprocess");
  detect_meta->num_boxes = 0;
  tinyyolov2 = GST_TINYYOLOV2 (vi);

  gst_create_boxes (vi, prediction, valid_prediction,
      &detect_meta->boxes, &detect_meta->num_boxes, tinyyolov2->obj_thresh,
      tinyyolov2->prob_thresh, tinyyolov2->iou_thresh);

  gst_inference_print_boxes (vi, gst_tinyyolov2_debug_category, detect_meta);

  *valid_prediction = (detect_meta->num_boxes > 0) ? TRUE : FALSE;

  return TRUE;
}

static gboolean
gst_tinyyolov2_postprocess_new (GstVideoInference * vi,
    const gpointer prediction, gsize predsize, GstMeta * meta_model,
    GstVideoInfo * info_model, gboolean * valid_prediction)
{
  GstTinyyolov2 *tinyyolov2 = NULL;
  GstInferenceMeta *imeta = NULL;
  BBox *boxes = NULL;
  gint num_boxes, i;

  g_return_val_if_fail (vi != NULL, FALSE);
  g_return_val_if_fail (meta_model != NULL, FALSE);
  g_return_val_if_fail (info_model != NULL, FALSE);

  imeta = (GstInferenceMeta *) meta_model;
  tinyyolov2 = GST_TINYYOLOV2 (vi);

  GST_LOG_OBJECT (tinyyolov2, "Postprocess Meta");

  /* Create boxes from prediction data */
  gst_create_boxes (vi, prediction, valid_prediction,
      &boxes, &num_boxes, tinyyolov2->obj_thresh,
      tinyyolov2->prob_thresh, tinyyolov2->iou_thresh);

  GST_LOG_OBJECT (tinyyolov2, "Number of predictions: %d", num_boxes);

  if (NULL == imeta->prediction) {
    imeta->prediction = gst_inference_prediction_new ();
    imeta->prediction->bbox.width = info_model->width;
    imeta->prediction->bbox.height = info_model->height;
  }

  for (i = 0; i < num_boxes; i++) {
    GstInferencePrediction *pred =
        gst_create_prediction_from_box (vi, &boxes[i]);
    gst_inference_prediction_append (imeta->prediction, pred);
  }

  /* Free boxes after creation */
  g_free (boxes);

  /* Log predictions */
  gst_inference_print_predictions (vi, gst_tinyyolov2_debug_category, imeta);

  *valid_prediction = (num_boxes > 0) ? TRUE : FALSE;

  return TRUE;
}

static gboolean
gst_tinyyolov2_start (GstVideoInference * vi)
{
  GST_INFO_OBJECT (vi, "Starting TinyYolo");

  return TRUE;
}

static gboolean
gst_tinyyolov2_stop (GstVideoInference * vi)
{
  GST_INFO_OBJECT (vi, "Stopping TinyYolo");

  return TRUE;
}
