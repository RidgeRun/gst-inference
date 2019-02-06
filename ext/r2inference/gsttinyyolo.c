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
#include "gst/r2inference/gstinferencemeta.h"
#include <string.h>

GST_DEBUG_CATEGORY_STATIC (gst_tinyyolo_debug_category);
#define GST_CAT_DEFAULT gst_tinyyolo_debug_category

/* prototypes */

#define CHANNELS 3
#define GRID_H 7
#define GRID_W 7
/* Number of classes */
#define CLASSES 20
/* Number of boxes per cell */
#define BOXES 2
/* Box dim */
#define BOX_DIM 4
/* Probability threshold */
#define PROB_THRESH 0.08
/* Intersection over union threshold */
#define IOU_THRESH 0.30

static void gst_tinyyolo_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_tinyyolo_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_tinyyolo_dispose (GObject * object);
static void gst_tinyyolo_finalize (GObject * object);

static gboolean gst_tinyyolo_preprocess (GstVideoInference * vi,
    GstVideoFrame * inframe, GstVideoFrame * outframe);
static gboolean gst_tinyyolo_postprocess (GstVideoInference * vi,
    GstMeta * meta, GstVideoFrame * outframe, const gpointer prediction,
    gsize predsize, gboolean * valid_prediction);
static gboolean gst_tinyyolo_start (GstVideoInference * vi);
static gboolean gst_tinyyolo_stop (GstVideoInference * vi);

static void print_top_predictions (GstVideoInference * vi, gpointer prediction,
    gint input_image_width, gint input_image_height, BBox * resulting_boxes,
    gint * elements);
static void get_boxes_from_prediction (gpointer prediction,
    gint input_image_width, gint input_image_height, BBox * boxes,
    gint * elements);

static void box_to_pixels (BBox * normalized_box, gint row, gint col,
    gint image_width, gint image_height);

static void remove_duplicated_boxes (BBox * boxes, gint * num_boxes);

static void delete_box (BBox * boxes, gint * num_boxes, gint index);

static gdouble intersection_over_union (BBox box_1, BBox box_2);

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
      "   Jose Jimenez <jose.jimenez@ridgerun.com> \n\t\t\t"
      "   Michael Gruner <michael.gruner@ridgerun.com> \n\t\t\t"
      "   Carlos Aguero <carlos.aguero@ridgerun.com> \n\t\t\t"
      "   Miguel Taylor <miguel.taylor@ridgerun.com> \n\t\t\t"
      "   Greivin Fallas <greivin.fallas@ridgerun.com>");

  gobject_class->set_property = gst_tinyyolo_set_property;
  gobject_class->get_property = gst_tinyyolo_get_property;
  gobject_class->dispose = gst_tinyyolo_dispose;
  gobject_class->finalize = gst_tinyyolo_finalize;

  vi_class->start = GST_DEBUG_FUNCPTR (gst_tinyyolo_start);
  vi_class->stop = GST_DEBUG_FUNCPTR (gst_tinyyolo_stop);
  vi_class->preprocess = GST_DEBUG_FUNCPTR (gst_tinyyolo_preprocess);
  vi_class->postprocess = GST_DEBUG_FUNCPTR (gst_tinyyolo_postprocess);
  vi_class->inference_meta_info = gst_inference_detection_meta_get_info ();
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

void
box_to_pixels (BBox * normalized_box, gint row, gint col, gint image_width,
    gint image_height)
{

  /* adjust the box center according to its cell and grid dim */
  normalized_box->x += col;
  normalized_box->y += row;
  normalized_box->x /= GRID_H;
  normalized_box->y /= GRID_W;

  /* adjust the lengths and widths */
  normalized_box->width *= normalized_box->width;
  normalized_box->height *= normalized_box->height;

  /* scale the boxes to the image size in pixels */
  normalized_box->x *= image_width;
  normalized_box->y *= image_height;
  normalized_box->width *= image_width;
  normalized_box->height *= image_height;
}

void
get_boxes_from_prediction (gpointer prediction, gint input_image_width,
    gint input_image_height, BBox * boxes, gint * elements)
{

  gint i;
  gint j;
  gint c;
  gint b;
  gint box_probs_start = GRID_H * GRID_W * CLASSES;
  gint all_boxes_start = GRID_H * GRID_W * CLASSES + GRID_H * GRID_W * BOXES;
  gint index;
  gdouble class_prob;
  gdouble box_prob;
  gdouble prob;
  gint counter = 0;

  /* Iterate rows */
  for (i = 0; i < GRID_H; i++) {
    /* Iterate colums */
    for (j = 0; j < GRID_W; j++) {
      /* Iterate classes */
      for (c = 0; c < CLASSES; c++) {
        index = (i * GRID_W + j) * CLASSES + c;
        class_prob = ((gfloat *) prediction)[index];
        for (b = 0; b < BOXES; b++) {
          index = (i * GRID_W + j) * BOXES + b;
          box_prob = ((gfloat *) prediction)[box_probs_start + index];
          prob = class_prob * box_prob;
          /* If the probability is over the threshold add it to the boxes list */
          if (prob > PROB_THRESH) {
            BBox result;
            index = ((i * GRID_W + j) * BOXES + b) * BOX_DIM;
            result.label = c;
            result.x = ((gfloat *) prediction)[all_boxes_start + index];
            result.y = ((gfloat *) prediction)[all_boxes_start + index + 1];
            result.width = ((gfloat *) prediction)[all_boxes_start + index + 2];
            result.height =
                ((gfloat *) prediction)[all_boxes_start + index + 3];
            result.prob = prob;
            box_to_pixels (&result, i, j, input_image_width,
                input_image_height);
            result.x = result.x - result.width / 2.0;
            result.y = result.y - result.height / 2.0;
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
    gint input_image_width, gint input_image_height, BBox * resulting_boxes,
    gint * elements)
{

  BBox boxes[GRID_H * GRID_W * BOXES];
  gint index = 0;
  *elements = 0;

  get_boxes_from_prediction (prediction, input_image_width, input_image_height,
      boxes, elements);

  remove_duplicated_boxes (boxes, elements);

  resulting_boxes = malloc (*elements * sizeof (BBox));
  for (index = 0; index < *elements; index = index + 1) {
    resulting_boxes[index] = boxes[index];
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
  intersection_dim_1 = MIN (box_1.x + 0.5 * box_1.width,
      box_2.x + 0.5 * box_2.width) -
      MAX (box_1.x - 0.5 * box_1.width, box_2.x - 0.5 * box_2.width);

  /* Second dimension of the intersecting box */
  intersection_dim_2 = MIN (box_1.y + 0.5 * box_1.height,
      box_2.y + 0.5 * box_2.height) -
      MAX (box_1.y - 0.5 * box_1.height, box_2.y - 0.5 * box_2.height);

  if ((intersection_dim_1 < 0) || (intersection_dim_2 < 0)) {
    intersection_area = 0;
  } else {
    intersection_area = intersection_dim_1 * intersection_dim_2;
  }
  union_area = box_1.width * box_1.height + box_2.width * box_2.height -
      intersection_area;
  return intersection_area / union_area;
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
remove_duplicated_boxes (BBox * boxes, gint * num_boxes)
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
        if (iou > IOU_THRESH) {
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
gst_tinyyolo_preprocess (GstVideoInference * vi,
    GstVideoFrame * inframe, GstVideoFrame * outframe)
{

  gint size;
  GST_LOG_OBJECT (vi, "Preprocess");

  size = CHANNELS * inframe->info.width * inframe->info.height;

  for (gint i = 0; i < size; i += CHANNELS) {
    ((gfloat *) outframe->data[0])[i + 2] =
        (((guchar *) inframe->data[0])[i + 2]) / 255.0;
    ((gfloat *) outframe->data[0])[i + 1] =
        (((guchar *) inframe->data[0])[i + 1]) / 255.0;
    ((gfloat *) outframe->data[0])[i + 0] =
        (((guchar *) inframe->data[0])[i + 0]) / 255.0;
  }

  return TRUE;
}

static gboolean
gst_tinyyolo_postprocess (GstVideoInference * vi, GstMeta * meta,
    GstVideoFrame * outframe, const gpointer prediction, gsize predsize,
    gboolean * valid_prediction)
{
  GstDetectionMeta *detect_meta = (GstDetectionMeta *) meta;
  GST_LOG_OBJECT (vi, "Postprocess");
  detect_meta->num_boxes = 0;

  print_top_predictions (vi, prediction, outframe->info.width,
      outframe->info.height, detect_meta->boxes, &detect_meta->num_boxes);
  *valid_prediction = (detect_meta->num_boxes > 0) ? TRUE : FALSE;

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
