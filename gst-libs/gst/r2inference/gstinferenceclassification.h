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

#ifndef __GST_INFERENCE_CLASSIFICATION__
#define __GST_INFERENCE_CLASSIFICATION__

#include <gst/gst.h>

G_BEGIN_DECLS

/**
 * Classification to be assigned to an object
 */
typedef struct _GstInferenceClassification GstInferenceClassification;
struct _GstInferenceClassification
{
  GstMiniObject base;

  gint class_id;
  gdouble class_prob;
  gchar *class_label;
  gint num_classes;
  gdouble *probabilities;
  gchar **labels;
};

GstInferenceClassification * gst_inference_classification_new (void);
GstInferenceClassification * gst_inference_classification_new_full (gint class_id, gdouble class_prob,
    const gchar * class_label, gint num_classes, const gdouble * probabilities,
    gchar ** labels);
void gst_inference_classification_reset (GstInferenceClassification * self);
GstInferenceClassification * gst_inference_classification_copy (const GstInferenceClassification * self);
void gst_inference_classification_free (GstInferenceClassification * self);
GstInferenceClassification * gst_inference_classification_ref (GstInferenceClassification * self);
void gst_inference_classification_unref (GstInferenceClassification * self);
gchar * gst_inference_classification_to_string (GstInferenceClassification * self, gint level);

G_END_DECLS

#endif // __GST_INFERENCE_CLASSIFICATION__
