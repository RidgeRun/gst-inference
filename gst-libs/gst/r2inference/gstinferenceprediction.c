/* Copyright (C) 2019 RidgeRun, LLC (http://www.ridgerun.com)
 * All Rights Reserved.
 *
 * The contents of this software are proprietary and confidential to RidgeRun,
 * LLC.  No part of this program may be photocopied, reproduced or translated
 * into another programming language without prior written consent of
 * RidgeRun, LLC.  The user is free to modify the source code after obtaining
 * a software license from RidgeRun.  All source code changes must be provided
 * back to RidgeRun without any encumbrance.
*/

#include "gstinferenceprediction.h"

static GType gst_inference_prediction_get_type (void);
GST_DEFINE_MINI_OBJECT_TYPE (GstInferencePrediction, gst_inference_prediction);

static GstInferencePrediction *prediction_copy (const GstInferencePrediction *
    self);
static void prediction_free (GstInferencePrediction * obj);
static void prediction_reset (GstInferencePrediction * self);
static gchar *prediction_to_string (GstInferencePrediction * self, gint level);
static gchar *prediction_children_to_string (GstInferencePrediction * self,
    gint level);
static gchar *prediction_classes_to_string (GstInferencePrediction * self,
    gint level);

static GstInferenceClassification
    * classification_copy (GstInferenceClassification * from, gpointer data);

static void bounding_box_reset (BoundingBox * bbox);
static gchar *bounding_box_to_string (BoundingBox * bbox, gint level);

static void node_get_children (GNode * node, gpointer data);
static gpointer node_copy (gconstpointer node, gpointer data);
static gboolean node_assign (GNode * node, gpointer data);

GstInferencePrediction *
gst_inference_prediction_new (void)
{
  GstInferencePrediction *self = g_slice_new (GstInferencePrediction);

  gst_mini_object_init (GST_MINI_OBJECT_CAST (self), 0,
      gst_inference_prediction_get_type (),
      (GstMiniObjectCopyFunction) gst_inference_prediction_copy, NULL,
      (GstMiniObjectFreeFunction) prediction_free);

  self->predictions = NULL;
  self->classifications = NULL;

  prediction_reset (self);

  return self;
}

GstInferencePrediction *
gst_inference_prediction_ref (GstInferencePrediction * self)
{
  g_return_val_if_fail (self, NULL);

  return (GstInferencePrediction *)
      gst_mini_object_ref (GST_MINI_OBJECT_CAST (self));
}

void
gst_inference_prediction_unref (GstInferencePrediction * self)
{
  g_return_if_fail (self);

  gst_mini_object_unref (GST_MINI_OBJECT_CAST (self));
}

void
gst_inference_prediction_append (GstInferencePrediction * self,
    GstInferencePrediction * child)
{
  g_return_if_fail (self);
  g_return_if_fail (child);

  g_node_append (self->predictions, child->predictions);
}

static GstInferenceClassification *
classification_copy (GstInferenceClassification * from, gpointer data)
{
  return gst_inference_classification_copy (from);
}

static GstInferencePrediction *
prediction_copy (const GstInferencePrediction * self)
{
  GstInferencePrediction *other = NULL;

  g_return_val_if_fail (self, NULL);

  other = gst_inference_prediction_new ();

  other->id = self->id;
  other->enabled = self->enabled;
  other->bbox = self->bbox;

  other->classifications =
      g_list_copy_deep (self->classifications, (GCopyFunc) classification_copy,
      NULL);

  return other;
}

static gpointer
node_copy (gconstpointer node, gpointer data)
{
  GstInferencePrediction *self = (GstInferencePrediction *) node;

  return prediction_copy (self);
}

static gboolean
node_assign (GNode * node, gpointer data)
{
  GstInferencePrediction *pred = (GstInferencePrediction *) node->data;

  pred->predictions = node;

  return FALSE;
}

GstInferencePrediction *
gst_inference_prediction_copy (const GstInferencePrediction * self)
{
  GNode *other = NULL;

  g_return_val_if_fail (self, NULL);

  /* Copy the binary tree */
  other = g_node_copy_deep (self->predictions, node_copy, NULL);

  /* Now finish assigning the nodes to the predictions */
  g_node_traverse (other, G_IN_ORDER, G_TRAVERSE_ALL, -1, node_assign, NULL);

  return (GstInferencePrediction *) other->data;
}

static gchar *
bounding_box_to_string (BoundingBox * bbox, gint level)
{
  gint indent = level * 2;

  g_return_val_if_fail (bbox, NULL);

  return g_strdup_printf ("{\n"
      "%*s  x : %u\n"
      "%*s  y : %u\n"
      "%*s  width : %u\n"
      "%*s  height : %u\n"
      "%*s}",
      indent, "", bbox->x,
      indent, "", bbox->y,
      indent, "", bbox->width, indent, "", bbox->height, indent, "");
}

static gchar *
prediction_children_to_string (GstInferencePrediction * self, gint level)
{
  GSList *subpreds = NULL;
  GSList *iter = NULL;
  GString *string = NULL;

  g_return_val_if_fail (self, NULL);

  /* Build the child predictions using a GString */
  string = g_string_new (NULL);

  subpreds = gst_inference_prediction_get_children (self);

  for (iter = subpreds; iter != NULL; iter = g_slist_next (iter)) {
    GstInferencePrediction *pred = (GstInferencePrediction *) iter->data;
    gchar *child = prediction_to_string (pred, level + 1);

    g_string_append_printf (string, "%s, ", child);
    g_free (child);
  }

  return g_string_free (string, FALSE);
}

static gchar *
prediction_classes_to_string (GstInferencePrediction * self, gint level)
{
  GList *iter = NULL;
  GString *string = NULL;

  g_return_val_if_fail (self, NULL);

  /* Build the classes for this predictions using a GString */
  string = g_string_new (NULL);

  for (iter = self->classifications; iter != NULL; iter = g_list_next (iter)) {
    GstInferenceClassification *c = (GstInferenceClassification *) iter->data;
    gchar *sclass = gst_inference_classification_to_string (c, level + 1);

    g_string_append_printf (string, "%s, ", sclass);
    g_free (sclass);
  }

  return g_string_free (string, FALSE);
}

static gchar *
prediction_to_string (GstInferencePrediction * self, gint level)
{
  gint indent = level * 2;
  gchar *bbox = NULL;
  gchar *children = NULL;
  gchar *classes = NULL;
  gchar *prediction = NULL;

  g_return_val_if_fail (self, NULL);

  bbox = bounding_box_to_string (&self->bbox, level + 1);
  classes = prediction_classes_to_string (self, level + 1);
  children = prediction_children_to_string (self, level + 1);

  prediction = g_strdup_printf ("{\n"
      "%*s  id : %llu,\n"
      "%*s  enabled : %s,\n"
      "%*s  bbox : %s,\n"
      "%*s  classes : [\n"
      "%*s    %s\n"
      "%*s  ],\n"
      "%*s  predictions : [\n"
      "%*s    %s\n"
      "%*s  ]\n"
      "%*s}",
      indent, "", self->id,
      indent, "", self->enabled ? "True" : "False",
      indent, "", bbox,
      indent, "", indent, "", classes, indent, "",
      indent, "", indent, "", children, indent, "", indent, "");

  g_free (bbox);
  g_free (children);
  g_free (classes);

  return prediction;
}

gchar *
gst_inference_prediction_to_string (GstInferencePrediction * self)
{
  return prediction_to_string (self, 0);
}

static void
node_get_children (GNode * node, gpointer data)
{
  GSList **children = (GSList **) data;
  GstInferencePrediction *prediction;

  g_return_if_fail (node);
  g_return_if_fail (children);

  prediction = (GstInferencePrediction *) node->data;

  *children = g_slist_append (*children, prediction);
}

GSList *
gst_inference_prediction_get_children (GstInferencePrediction * self)
{
  GSList *children = NULL;

  g_return_val_if_fail (self, NULL);

  if (self->predictions) {
    g_node_children_foreach (self->predictions, G_TRAVERSE_ALL,
        node_get_children, &children);
  }

  return children;
}

static void
bounding_box_reset (BoundingBox * bbox)
{
  g_return_if_fail (bbox);

  bbox->x = 0;
  bbox->y = 0;
  bbox->width = 0;
  bbox->height = 0;
}

static void
prediction_reset (GstInferencePrediction * self)
{
  g_return_if_fail (self);

  self->id = 0;
  self->enabled = FALSE;

  bounding_box_reset (&self->bbox);

  /* Free al children */
  prediction_free (self);

  self->predictions = g_node_new (self);
}

static void
prediction_free (GstInferencePrediction * self)
{
  GSList *children = gst_inference_prediction_get_children (self);
  GSList *iter = NULL;

  for (iter = children; iter != NULL; iter = g_slist_next (iter)) {
    GstInferencePrediction *child = (GstInferencePrediction *) iter->data;

    gst_inference_prediction_unref (child);
  }

  g_list_free_full (self->classifications,
      (GDestroyNotify) gst_inference_classification_unref);
  self->classifications = NULL;

  if (self->predictions) {
    g_node_destroy (self->predictions);
    self->predictions = NULL;
  }
}

void
gst_inference_prediction_append_classification (GstInferencePrediction * self,
    GstInferenceClassification * c)
{
  g_return_if_fail (self);
  g_return_if_fail (c);

  self->classifications = g_list_append (self->classifications, c);
}
