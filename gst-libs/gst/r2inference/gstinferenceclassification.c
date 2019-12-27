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

#include "gstinferenceclassification.h"

static GType gst_inference_classification_get_type (void);
GST_DEFINE_MINI_OBJECT_TYPE (GstInferenceClassification,
    gst_inference_classification);

static void classification_free (GstInferenceClassification * self);
static void classification_reset (GstInferenceClassification * self);

static gdouble *probabilities_copy (const gdouble * from, gint num_classes);

static void
classification_reset (GstInferenceClassification * self)
{
  g_return_if_fail (self);

  self->class_id = -1;
  self->class_prob = 0.0f;
  self->num_classes = 0;

  if (self->class_label) {
    g_free (self->class_label);
  }
  self->class_label = NULL;

  if (self->probabilities) {
    g_free (self->probabilities);
  }
  self->probabilities = NULL;

  if (self->labels) {
    g_strfreev (self->labels);
  }
  self->labels = NULL;
}

GstInferenceClassification *
gst_inference_classification_new (void)
{
  GstInferenceClassification *self = g_slice_new (GstInferenceClassification);

  gst_mini_object_init (GST_MINI_OBJECT_CAST (self), 0,
      gst_inference_classification_get_type (),
      (GstMiniObjectCopyFunction) gst_inference_classification_copy, NULL,
      (GstMiniObjectFreeFunction) classification_free);

  self->class_label = NULL;
  self->probabilities = NULL;
  self->labels = NULL;

  classification_reset (self);

  return self;
}

static gdouble *
probabilities_copy (const gdouble * from, gint num_classes)
{
  gsize size = 0;
  gdouble *to = NULL;

  g_return_val_if_fail (from, NULL);
  g_return_val_if_fail (num_classes > 0, NULL);

  size = num_classes * sizeof (double);
  to = g_malloc0 (size);

  memcpy (to, from, size);

  return to;
}

GstInferenceClassification *
gst_inference_classification_new_full (gint class_id, gdouble class_prob,
    const gchar * class_label, gint num_classes, const gdouble * probabilities,
    gchar ** labels)
{
  GstInferenceClassification *self = gst_inference_classification_new ();

  self->class_id = class_id;
  self->class_prob = class_prob;
  self->num_classes = num_classes;

  if (class_label) {
    self->class_label = g_strdup (class_label);
  }

  if (probabilities && num_classes > 0) {
    self->probabilities = probabilities_copy (probabilities, num_classes);
  }

  if (labels) {
    self->labels = g_strdupv (labels);
  }

  return self;
}

GstInferenceClassification *
gst_inference_classification_ref (GstInferenceClassification * self)
{
  g_return_val_if_fail (self, NULL);

  return (GstInferenceClassification *)
      gst_mini_object_ref (GST_MINI_OBJECT_CAST (self));
}

void
gst_inference_classification_unref (GstInferenceClassification * self)
{
  g_return_if_fail (self);

  gst_mini_object_unref (GST_MINI_OBJECT_CAST (self));
}

GstInferenceClassification *
gst_inference_classification_copy (const GstInferenceClassification * self)
{
  GstInferenceClassification *other = NULL;

  g_return_val_if_fail (self, NULL);

  other = gst_inference_classification_new ();

  other->class_id = self->class_id;
  other->class_prob = self->class_prob;
  other->num_classes = self->num_classes;

  if (self->class_label) {
    other->class_label = g_strdup (self->class_label);
  }

  if (self->probabilities) {
    other->probabilities =
        probabilities_copy (self->probabilities, self->num_classes);
  }

  if (self->labels) {
    other->labels = g_strdupv (self->labels);
  }

  return other;
}

gchar *
gst_inference_classification_to_string (GstInferenceClassification * self,
    gint level)
{
  gint indent = level * 2;

  g_return_val_if_fail (self, NULL);

  return g_strdup_printf ("{\n"
      "%*s  Class : %d/%d\n"
      "%*s  Label : %s\n"
      "%*s  Probability : %f\n"
      "%*s}",
      indent, "", self->class_id, self->num_classes,
      indent, "", self->class_label, indent, "", self->class_prob, indent, "");
}

static void
classification_free (GstInferenceClassification * self)
{
  classification_reset (self);
}
