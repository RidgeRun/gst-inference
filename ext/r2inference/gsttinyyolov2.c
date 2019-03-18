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

GST_DEBUG_CATEGORY_STATIC (gst_tinyyolov2_debug_category);
#define GST_CAT_DEFAULT gst_tinyyolov2_debug_category

/* prototypes */

#define CHANNELS 3
#define GRID_H 13
#define GRID_W 13
/* Grid cell size in pixels */
#define GRID_SIZE 32
/* Number of classes */
#define CLASSES 20
/* Number of boxes per cell */
#define BOXES 5
/* 
 * Box dim 
 * [0]: x (center)
 * [0]: y (center)
 * [2]: height
 * [3]: width
 * [4]: Objectness score
 */
#define BOX_DIM 5
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
    gsize predsize, GstMeta * meta_model, GstVideoInfo * info_model,
    gboolean * valid_prediction);
static gboolean gst_tinyyolov2_start (GstVideoInference * vi);
static gboolean gst_tinyyolov2_stop (GstVideoInference * vi);

static void print_top_predictions (GstVideoInference * vi, gpointer prediction,
    gint input_image_width, gint input_image_height, BBox ** resulting_boxes,
    gint * elements);
static void gst_tinyyolov2_get_boxes_from_prediction (GstTinyyolov2 *
    tinyyolov2, gpointer prediction, BBox * boxes, gint * elements);

static void box_to_pixels (BBox * normalized_box, gint row, gint col, gint box);

static void gst_tinyyolov2_remove_duplicated_boxes (GstTinyyolov2 * tinyyolov2,
    BBox * boxes, gint * num_boxes);

static void delete_box (BBox * boxes, gint * num_boxes, gint index);

static gdouble intersection_over_union (BBox box_1, BBox box_2);
static gdouble sigmoid (gdouble x);

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

void
box_to_pixels (BBox * normalized_box, gint row, gint col, gint box)
{
  /* adjust the box center according to its cell and grid dim */
  normalized_box->x = (col + sigmoid (normalized_box->x)) * GRID_SIZE;
  normalized_box->y = (row + sigmoid (normalized_box->y)) * GRID_SIZE;

  /* adjust the lengths and widths */
  normalized_box->width =
      pow (M_E, normalized_box->width) * box_anchors[2 * box] * GRID_SIZE;
  normalized_box->height =
      pow (M_E, normalized_box->height) * box_anchors[2 * box + 1] * GRID_SIZE;
}

void
gst_tinyyolov2_get_boxes_from_prediction (GstTinyyolov2 * tinyyolov2,
    gpointer prediction, BBox * boxes, gint * elements)
{

  gint i, j, c, b;
  gint index;
  gdouble obj_prob;
  gdouble cur_class_prob, max_class_prob;
  gint max_class_prob_index;
  gint counter = 0;

  /* Iterate rows */
  for (i = 0; i < GRID_H; i++) {
    /* Iterate colums */
    for (j = 0; j < GRID_W; j++) {
      /* Iterate boxes */
      for (b = 0; b < BOXES; b++) {
        index = ((i * GRID_W + j) * BOXES + b) * (BOX_DIM + CLASSES);
        obj_prob = ((gfloat *) prediction)[index + 4];
        /* If the Objectness score is over the threshold add it to the boxes list */
        if (obj_prob > tinyyolov2->obj_thresh) {
          max_class_prob = 0;
          max_class_prob_index = 0;
          for (c = 0; c < CLASSES; c++) {
            cur_class_prob = ((gfloat *) prediction)[index + BOX_DIM + c];
            if (cur_class_prob > max_class_prob) {
              max_class_prob = cur_class_prob;
              max_class_prob_index = c;
            }
          }
          if (max_class_prob > tinyyolov2->prob_thresh) {
            BBox result;
            result.label = max_class_prob_index;
            result.prob = max_class_prob;
            result.x = ((gfloat *) prediction)[index];
            result.y = ((gfloat *) prediction)[index + 1];
            result.width = ((gfloat *) prediction)[index + 2];
            result.height = ((gfloat *) prediction)[index + 3];
            box_to_pixels (&result, i, j, b);
            result.x = result.x - result.width * 0.5;
            result.y = result.y - result.height * 0.5;
            boxes[counter] = result;
            counter = counter + 1;
          }
        }
      }
    }
    *elements = counter;
  }
}

void
print_top_predictions (GstVideoInference * vi, gpointer prediction,
    gint input_image_width, gint input_image_height, BBox ** resulting_boxes,
    gint * elements)
{
  GstTinyyolov2 *tinyyolov2 = GST_TINYYOLOV2 (vi);
  BBox boxes[GRID_H * GRID_W * BOXES];
  gint index = 0;
  *elements = 0;

  gst_tinyyolov2_get_boxes_from_prediction (tinyyolov2, prediction, boxes,
      elements);
  gst_tinyyolov2_remove_duplicated_boxes (tinyyolov2, boxes, elements);

  *resulting_boxes = g_malloc (*elements * sizeof (BBox));
  memcpy (*resulting_boxes, boxes, *elements * sizeof (BBox));
  for (index = 0; index < *elements; index++) {
    GST_LOG_OBJECT (vi,
        "Box: [class:%d, x:%f, y:%f, width:%f, height:%f, prob:%f]",
        boxes[index].label, boxes[index].x, boxes[index].y, boxes[index].width,
        boxes[index].height, boxes[index].prob);
  }
}

gdouble
intersection_over_union (BBox box_1, BBox box_2)
{

  /*
   * Evaluate the intersection-over-union for two boxes
   * The intersection-over-union metric determines how close
   * two boxes are to being the same box.
   */
  gdouble intersection_dim_1;
  gdouble intersection_dim_2;
  gdouble intersection_area;
  gdouble union_area;

  /* First diminsion of the intersecting box */
  intersection_dim_1 =
      MIN (box_1.x + box_1.width, box_2.x + box_2.width) - MAX (box_1.x,
      box_2.x);

  /* Second dimension of the intersecting box */
  intersection_dim_2 =
      MIN (box_1.y + box_1.height, box_2.y + box_2.height) - MAX (box_1.y,
      box_2.y);

  if ((intersection_dim_1 < 0) || (intersection_dim_2 < 0)) {
    intersection_area = 0;
  } else {
    intersection_area = intersection_dim_1 * intersection_dim_2;
  }
  union_area = box_1.width * box_1.height + box_2.width * box_2.height -
      intersection_area;
  return intersection_area / union_area;
}

/* sigmoid approximation as a lineal function */
static gdouble
sigmoid (gdouble x)
{
  return 1.0 / (1.0 + pow (M_E, -1.0 * x));
}

void
delete_box (BBox * boxes, gint * num_boxes, gint index)
{

  gint i, last_index;
  if (*num_boxes > 0 && index < *num_boxes && index > -1) {
    last_index = *num_boxes - 1;
    for (i = index; i < last_index; i++) {
      boxes[i] = boxes[i + 1];
    }
    *num_boxes -= 1;
  }
}

void
gst_tinyyolov2_remove_duplicated_boxes (GstTinyyolov2 * tinyyolov2,
    BBox * boxes, gint * num_boxes)
{

  /* Remove duplicated boxes. A box is considered a duplicate if its
   * intersection over union metric is above a threshold
   */
  gdouble iou;
  gint it1, it2;

  for (it1 = 0; it1 < *num_boxes - 1; it1++) {
    for (it2 = it1 + 1; it2 < *num_boxes; it2++) {
      if (boxes[it1].label == boxes[it2].label) {
        iou = intersection_over_union (boxes[it1], boxes[it2]);
        if (iou > tinyyolov2->iou_thresh) {
          if (boxes[it1].prob > boxes[it2].prob) {
            delete_box (boxes, num_boxes, it2);
            it2--;
          } else {
            delete_box (boxes, num_boxes, it1);
            it1--;
            break;
          }
        }
      }
    }
  }
}

static gboolean
gst_tinyyolov2_preprocess (GstVideoInference * vi,
    GstVideoFrame * inframe, GstVideoFrame * outframe)
{
  gint size;
  gint first_index, last_index, offset;

  GST_LOG_OBJECT (vi, "Preprocess");

  size = CHANNELS * inframe->info.width * inframe->info.height;

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
    default:
      GST_ERROR_OBJECT (vi, "Invalid format");
      return FALSE;
      break;
  }

  for (gint i = 0; i < size; i += CHANNELS) {
    ((gfloat *) outframe->data[0])[i + first_index] =
        (((guchar *) inframe->data[0])[i + 2 + offset]) / 255.0;
    ((gfloat *) outframe->data[0])[i + 1] =
        (((guchar *) inframe->data[0])[i + 1 + offset]) / 255.0;
    ((gfloat *) outframe->data[0])[i + last_index] =
        (((guchar *) inframe->data[0])[i + 0 + offset]) / 255.0;
  }

  return TRUE;
}

static gboolean
gst_tinyyolov2_postprocess (GstVideoInference * vi, const gpointer prediction,
    gsize predsize, GstMeta * meta_model, GstVideoInfo * info_model,
    gboolean * valid_prediction)
{
  GstDetectionMeta *detect_meta = (GstDetectionMeta *) meta_model;
  GST_LOG_OBJECT (vi, "Postprocess");
  detect_meta->num_boxes = 0;

  print_top_predictions (vi, prediction, info_model->width,
      info_model->height, &detect_meta->boxes, &detect_meta->num_boxes);
  *valid_prediction = (detect_meta->num_boxes > 0) ? TRUE : FALSE;

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
