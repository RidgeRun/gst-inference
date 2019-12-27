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

#ifndef __GST_INFERENCE_PREDICTION__
#define __GST_INFERENCE_PREDICTION__

#include <gst/r2inference/gstinferenceclassification.h>
#include <gst/video/video.h>

G_BEGIN_DECLS

typedef struct _BoundingBox BoundingBox;
typedef struct _GstInferencePrediction GstInferencePrediction;

/**
 * Size and coordinates of an image region
 */
struct _BoundingBox
{
  guint x;
  guint y;
  guint width;
  guint height;
};

/**
 * Abstraction that represents a prediction
 */
struct _GstInferencePrediction
{
  GstMiniObject base;

  guint64 id;
  gboolean enabled;
  BoundingBox bbox;
  GList * classifications;
  GNode * predictions;
};

GstInferencePrediction * gst_inference_prediction_new (void);
void gst_inference_prediction_reset (GstInferencePrediction * self);
GstInferencePrediction * gst_inference_prediction_copy (const GstInferencePrediction * self);
void gst_inference_prediction_free (GstInferencePrediction * self);
GstInferencePrediction * gst_inference_prediction_ref (GstInferencePrediction * self);
void gst_inference_prediction_unref (GstInferencePrediction * self);
gchar * gst_inference_prediction_to_string (GstInferencePrediction * self);
void gst_inference_prediction_append (GstInferencePrediction * self, GstInferencePrediction * child);
GSList * gst_inference_prediction_get_children (GstInferencePrediction * self);
void gst_inference_prediction_append_classification (GstInferencePrediction * self,
    GstInferenceClassification * c);
GstInferencePrediction * gst_inference_prediction_scale (GstInferencePrediction * self,
    GstVideoInfo * to, GstVideoInfo * from);

G_END_DECLS

#endif // __GST_INFERENCE_PREDICTION__
