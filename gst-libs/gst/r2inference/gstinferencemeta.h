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

#ifndef GST_INFERENCE_META_H
#define GST_INFERENCE_META_H

#include <gst/gst.h>
#include <gst/video/video.h>

G_BEGIN_DECLS
#define GST_CLASSIFICATION_META_API_TYPE (gst_classification_meta_api_get_type())
#define GST_CLASSIFICATION_META_INFO  (gst_classification_meta_get_info())
#define GST_DETECTION_META_API_TYPE (gst_detection_meta_api_get_type())
#define GST_DETECTION_META_INFO  (gst_detection_meta_get_info())

/**
 * Basic bounding box structure for detection
 */
typedef struct _BBox BBox;
struct _BBox
{
  gint x;
  gint y;
  gint w;
  gint h;
  gint label;
  gdouble prob;
};

/**
 * Implements the placeholder for classification information.
 */
typedef struct _GstClassificationMeta GstClassificationMeta;
struct _GstClassificationMeta
{
  GstMeta meta;
  gint num_labels;
  gdouble *probs;
};

/**
 * Implements the placeholder for detection information.
 */
typedef struct _GstDetectionMeta GstDetectionMeta;
struct _GstDetectionMeta
{
  GstMeta meta;
  gint num_boxes;
  BBox *boxes;
};

GType gst_classification_meta_api_get_type (void);
const GstMetaInfo *gst_inference_classification_meta_get_info (void);

GType gst_detection_meta_api_get_type (void);
const GstMetaInfo *gst_inference_detection_meta_get_info (void);

G_END_DECLS
#endif // GST_INFERENCE_META_H
