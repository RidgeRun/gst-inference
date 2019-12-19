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

static void bounding_box_reset (BoundingBox * bbox);
static GstMiniObject *_gst_inference_prediction_copy (const GstMiniObject *
    obj);

static void classification_reset (GList * classification);

GstInferencePrediction *
gst_inference_prediction_new (void)
{
  GstInferencePrediction *self = g_slice_new (GstInferencePrediction);

  gst_mini_object_init (GST_MINI_OBJECT_CAST (self), 0,
      gst_inference_prediction_get_type (), _gst_inference_prediction_copy,
      NULL, NULL);

  gst_inference_prediction_reset (self);

  return self;
}

void
gst_inference_prediction_reset (GstInferencePrediction * self)
{
  g_return_if_fail (self);

  self->id = 0;
  self->enabled = FALSE;

  bounding_box_reset (&self->bbox);
  classification_reset (self->classifications);
}

static void
bounding_box_reset (BoundingBox * bbox)
{
  bbox->x = 0;
  bbox->y = 0;
  bbox->width = 0;
  bbox->height = 0;
}

static void
classification_reset (GList * classifications)
{
  GList * iter = NULL;

  for (iter = classifications; iter != NULL; iter = g_list_next (iter)) {
    Classification *c = (Classification *)iter->data;
    c->class_id = 0;
    c->class_prob = 0.0f;
    c->class_label = NULL;
    c->num_classes = 0;
    c->classes_probs = NULL;
  }
}

GstInferencePrediction *
gst_inference_prediction_copy (const GstInferencePrediction * self)
{
  GstInferencePrediction *other = NULL;

  g_return_val_if_fail (self, NULL);

  other = gst_inference_prediction_new ();

  other->id = self->id;
  other->enabled = self->enabled;
  other->bbox = self->bbox;

  return other;
}

static GstMiniObject *
_gst_inference_prediction_copy (const GstMiniObject * obj)
{
  return
      GST_MINI_OBJECT_CAST (gst_inference_prediction_copy (
          (GstInferencePrediction *) obj));
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
