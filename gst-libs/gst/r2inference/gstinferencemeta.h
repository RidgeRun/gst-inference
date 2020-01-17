/*
 * GStreamer
 * Copyright (C) 2018-2020 RidgeRun <support@ridgerun.com>
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

#ifndef GST_INFERENCE_META_H
#define GST_INFERENCE_META_H

#include <gst/gst.h>

#include <gst/r2inference/gstinferenceprediction.h>

G_BEGIN_DECLS
#define GST_EMBEDDING_META_API_TYPE (gst_embedding_meta_api_get_type())
#define GST_EMBEDDING_META_INFO  (gst_embedding_meta_get_info())
#define GST_CLASSIFICATION_META_API_TYPE (gst_classification_meta_api_get_type())
#define GST_CLASSIFICATION_META_INFO  (gst_classification_meta_get_info())
#define GST_DETECTION_META_API_TYPE (gst_detection_meta_api_get_type())
#define GST_DETECTION_META_INFO  (gst_detection_meta_get_info())
#define GST_INFERENCE_META_API_TYPE (gst_inference_meta_api_get_type())
#define GST_INFERENCE_META_INFO  (gst_inference_meta_get_info())

/**
 * Basic bounding box structure for detection
 */
typedef struct _BBox BBox;
struct _BBox
{
  gint label;
  gdouble prob;
  gdouble x;
  gdouble y;
  gdouble width;
  gdouble height;
};

/**
 * Implements the placeholder for inference information.
 */
typedef struct _GstInferenceMeta GstInferenceMeta;
struct _GstInferenceMeta
{
  GstMeta meta;

  GstInferencePrediction *prediction;
};

/**
 * Implements the placeholder for embedding information.
 */
typedef struct _GstEmbeddingMeta GstEmbeddingMeta;
struct _GstEmbeddingMeta
{
  GstMeta meta;
  gint num_dimensions;
  gdouble *embedding;
};

/**
 * Implements the placeholder for classification information.
 */
typedef struct _GstClassificationMeta GstClassificationMeta;
struct _GstClassificationMeta
{
  GstMeta meta;
  gint num_labels;
  gdouble *label_probs;
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

GType gst_inference_meta_api_get_type (void);
const GstMetaInfo *gst_inference_meta_get_info (void);

GType gst_embedding_meta_api_get_type (void);
const GstMetaInfo *gst_embedding_meta_get_info (void);

GType gst_classification_meta_api_get_type (void);
const GstMetaInfo *gst_classification_meta_get_info (void);

GType gst_detection_meta_api_get_type (void);
const GstMetaInfo *gst_detection_meta_get_info (void);

G_END_DECLS
#endif // GST_INFERENCE_META_H
