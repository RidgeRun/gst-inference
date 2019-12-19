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
#include "gstinferencedebug.h"

static gboolean gst_inference_print_prediction (GNode * node, gpointer data);

void
gst_inference_print_embedding (GstVideoInference * vi,
    GstDebugCategory * category, GstClassificationMeta * class_meta,
    const gpointer prediction, GstDebugLevel gstlevel)
{
  GstDebugLevel level;

  g_return_if_fail (vi != NULL);
  g_return_if_fail (category != NULL);
  g_return_if_fail (class_meta != NULL);
  g_return_if_fail (prediction != NULL);

  /* Only display vector if debug level >= 6 */
  level = gst_debug_category_get_threshold (category);
  if (level >= gstlevel) {
    for (gint i = 0; i < class_meta->num_labels; ++i) {
      gfloat current = ((gfloat *) prediction)[i];
      GST_CAT_LEVEL_LOG (category, gstlevel, vi,
          "Output vector element %i : (%f)", i, current);
    }
  }
}

void
gst_inference_print_highest_probability (GstVideoInference * vi,
    GstDebugCategory * category, GstClassificationMeta * class_meta,
    const gpointer prediction, GstDebugLevel gstlevel)
{
  gint index;
  gdouble max;
  GstDebugLevel level;

  g_return_if_fail (vi != NULL);
  g_return_if_fail (category != NULL);
  g_return_if_fail (class_meta != NULL);
  g_return_if_fail (prediction != NULL);

  /* Only compute the highest probability is label when debug >= 6 */
  level = gst_debug_category_get_threshold (category);
  if (level >= gstlevel) {
    index = 0;
    max = -1;
    for (gint i = 0; i < class_meta->num_labels; ++i) {
      gfloat current = ((gfloat *) prediction)[i];
      if (current > max) {
        max = current;
        index = i;
      }
    }
    GST_CAT_LEVEL_LOG (category, gstlevel, vi,
        "Highest probability is label %i : (%f)", index, max);
  }
}

void
gst_inference_print_boxes (GstVideoInference * vi, GstDebugCategory * category,
    GstDetectionMeta * detect_meta)
{
  gint index;

  g_return_if_fail (vi != NULL);
  g_return_if_fail (category != NULL);
  g_return_if_fail (detect_meta != NULL);

  for (index = 0; index < detect_meta->num_boxes; index++) {
    GST_CAT_LOG_OBJECT (category, vi,
        "Box: [class:%d, x:%f, y:%f, width:%f, height:%f, prob:%f]",
        detect_meta->boxes[index].label, detect_meta->boxes[index].x,
        detect_meta->boxes[index].y, detect_meta->boxes[index].width,
        detect_meta->boxes[index].height, detect_meta->boxes[index].prob);
  }
}

static gboolean
gst_inference_print_prediction (GNode * node, gpointer data)
{
  Prediction *predict = (Prediction *) node->data;
  GstDebugCategory *category = (GstDebugCategory *) data;
  BBox *box = NULL;

  g_return_val_if_fail (category != NULL, FALSE);
  g_return_val_if_fail (predict != NULL, FALSE);

  box = predict->box;

  if (box != NULL) {
    GST_CAT_LOG (category,
        "Prediction ID:%d - Level:%d - Box: [class:%d, x:%f, y:%f, width:%f, height:%f, prob:%f]",
        predict->id, g_node_depth (node), box->label, box->x, box->y,
        box->width, box->height, box->prob);
  } else {
    GST_CAT_LOG (category, "Prediction ID:%d - Level:%d - No Box",
        predict->id, g_node_depth (node));
  }

  return FALSE;
}

void
gst_inference_print_predictions (GstVideoInference * vi,
    GstDebugCategory * category, GstInferenceMeta * inference_meta)
{
  Prediction *root = NULL;

  g_return_if_fail (vi != NULL);
  g_return_if_fail (inference_meta != NULL);

  root = inference_meta->prediction;

  g_node_traverse (root->node, G_LEVEL_ORDER, G_TRAVERSE_ALL, -1,
      gst_inference_print_prediction, (gpointer) category);
}
