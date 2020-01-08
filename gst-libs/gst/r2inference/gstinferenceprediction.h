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
 * BoundingBox:
 * @x: horizontal coordinate of the upper left position of the
 * bounding box in pixels
 * @y: vertical coordinate of the upper left position of the bounding
 * box in pixels
 * @width: width of the bounding box in pixels
 * @height: height of the bounding box in pixels
 *
 * Size and coordinates of a prediction region
 */
struct _BoundingBox
{
  guint x;
  guint y;
  guint width;
  guint height;
};

/**
 * GstInferencePrediction:
 * @prediction_id: unique id for this specific prediction
 * @enabled: flag indicating wether or not this prediction should be
 * used for further inference
 * @bbox: the BoundingBox for this specific prediction
 * @classifications: a linked list of GstInfereferenceClassification
 * associated to this prediction
 * @predictions: a n-ary tree of child predictions within this
 * specific prediction. It is recommended to to access the tree
 * directly, but to use this module's API to interact with the
 * children.
 *
 * Abstraction that represents a prediction
 */
struct _GstInferencePrediction
{
  /*<private>*/
  GstMiniObject base;

  /*<public>*/
  guint64 prediction_id;
  gboolean enabled;
  BoundingBox bbox;
  GList * classifications;
  GNode * predictions;
};

/**
 * gst_inference_prediction_new:
 * 
 * Creates a new GstInferencePrediction.
 *
 * Returns: A newly allocated and initialized GstInferencePrediction.
 */
GstInferencePrediction * gst_inference_prediction_new (void);

/**
 * gst_inference_prediction_reset:
 * @self: the prediction to reset 
 *
 * Clears a prediction, effectively removing al children and resetting
 * all members.
 */
void gst_inference_prediction_reset (GstInferencePrediction * self);

/**
 * gst_inference_prediction_copy:
 * @self: the prediction to copy
 *
 * Copies a prediction into a newly allocated one. This is a deep
 * copy, meaning that all children and classifications are copied as
 * well. No references are shared.
 *
 * Returns: a newly allocated copy of the original prediction
 */
GstInferencePrediction * gst_inference_prediction_copy (const GstInferencePrediction * self);

/**
 * gst_inference_prediction_ref:
 * @self: the prediction to ref
 *
 * Increase the reference counter of the prediction.
 *
 * Returns: the same prediction, for convenience purposes.
 */
GstInferencePrediction * gst_inference_prediction_ref (GstInferencePrediction * self);

/**
 * gst_inference_prediction_unref:
 * @self: the prediction to unref
 *
 * Decreases the reference counter of the prediction. When the
 * reference counter hits zero, the prediction is freed.
 */
void gst_inference_prediction_unref (GstInferencePrediction * self);

/**
 * gst_inference_prediction_to_string:
 * @self: the prediction to serialize
 *
 * Serializes the prediction along with it's classifications and
 * children into a JSON-like string. Free this string after usage
 * using g_free()
 *
 * Returns: a string representing the prediction.
 */
gchar * gst_inference_prediction_to_string (GstInferencePrediction * self);

/**
 * gst_inference_prediction_append:
 * @self: the parent prediction
 * @child: the prediction to append as a child
 *
 * Append a new prediction as part of the parent prediction
 * children. The parent takes ownership, use
 * gst_inference_prediction_ref() if you wish to keep a reference.
 */
void gst_inference_prediction_append (GstInferencePrediction * self, GstInferencePrediction * child);

/**
 * gst_inference_prediction_get_children:
 * @self: the parent prediction
 *
 * Gets a list of the immediate children of the current prediction. In
 * other words, the children of these childrens are not returned. The
 * references of these children are still owned by the parent.
 * 
 * Returns: A linked list of the child predictions.
 */
GSList * gst_inference_prediction_get_children (GstInferencePrediction * self);

/**
 * gst_inference_prediction_append_classification:
 * @self: the parent prediction
 * @c: the classification to append to the prediction
 *
 * A new GstInferenceClassification to associate with this
 * prediction. The prediction takes ownership of the classification
 */
void gst_inference_prediction_append_classification (GstInferencePrediction * self,
    GstInferenceClassification * c);

/**
 * gst_inference_prediction_scale:
 * @self: the original prediction
 * @to: the resulting image size
 * @from: the original image size
 *
 * Modifies the BoundingBox associated with this prediction (and all
 * its children) to scale to the new image size. This is typically
 * used by the GstMeta subsystem automatically and not for public
 * usage.
 *
 * Returns: a newly allocated and scaled prediction.
 */
GstInferencePrediction * gst_inference_prediction_scale (GstInferencePrediction * self,
    GstVideoInfo * to, GstVideoInfo * from);

G_END_DECLS

#endif // __GST_INFERENCE_PREDICTION__
