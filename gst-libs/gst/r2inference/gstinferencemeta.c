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

#include "gstinferencemeta.h"

#include <gst/video/video.h>

static void gst_inference_classification_meta_free (GstMeta * meta,
    GstBuffer * buffer);
static void gst_inference_detection_meta_free (GstMeta * meta,
    GstBuffer * buffer);
static gboolean gst_inference_meta_init (GstMeta * meta, gpointer params,
    GstBuffer * buffer);

static void
gst_inference_classification_meta_free (GstMeta * meta, GstBuffer * buffer)
{
  GstClassificationMeta *class_meta = (GstClassificationMeta *) meta;

  g_return_if_fail (meta != NULL);
  g_return_if_fail (buffer != NULL);

  if (class_meta->num_labels != 0) {
    g_free (class_meta->label_probs);
  }
}

GType
gst_inference_classification_meta_api_get_type (void)
{
  static volatile GType type = 0;
  static const gchar *tags[] = { GST_META_TAG_VIDEO_STR, NULL };

  if (g_once_init_enter (&type)) {
    GType _type = gst_meta_api_type_register ("GstClassificationMetaAPI", tags);
    g_once_init_leave (&type, _type);
  }
  return type;
}

/* classification metadata */
const GstMetaInfo *
gst_inference_classification_meta_get_info (void)
{
  static const GstMetaInfo *classification_meta_info = NULL;

  if (g_once_init_enter (&classification_meta_info)) {
    const GstMetaInfo *meta =
        gst_meta_register (GST_CLASSIFICATION_META_API_TYPE,
        "GstClassificationMeta", sizeof (GstClassificationMeta),
        gst_inference_meta_init,
        gst_inference_classification_meta_free, NULL);
    g_once_init_leave (&classification_meta_info, meta);
  }
  return classification_meta_info;
}

static void
gst_inference_detection_meta_free (GstMeta * meta, GstBuffer * buffer)
{
  GstDetectionMeta *detect_meta = (GstDetectionMeta *) meta;

  g_return_if_fail (meta != NULL);
  g_return_if_fail (buffer != NULL);

  if (detect_meta->num_boxes != 0) {
    g_free (detect_meta->boxes);
  }
}

GType
gst_inference_detection_meta_api_get_type (void)
{
  static volatile GType type = 0;
  static const gchar *tags[] = { GST_META_TAG_VIDEO_STR, NULL };

  if (g_once_init_enter (&type)) {
    GType _type = gst_meta_api_type_register ("GstDetectionMetaAPI", tags);
    g_once_init_leave (&type, _type);
  }
  return type;
}

/* detection metadata */
const GstMetaInfo *
gst_inference_detection_meta_get_info (void)
{
  static const GstMetaInfo *detection_meta_info = NULL;

  if (g_once_init_enter (&detection_meta_info)) {
    const GstMetaInfo *meta =
        gst_meta_register (GST_DETECTION_META_API_TYPE, "GstDetectionMeta",
        sizeof (GstDetectionMeta), gst_inference_meta_init,
        gst_inference_detection_meta_free,
        NULL);
    g_once_init_leave (&detection_meta_info, meta);
  }
  return detection_meta_info;
}

static gboolean
gst_inference_meta_init (GstMeta * meta, gpointer params, GstBuffer * buffer)
{
  return TRUE;
}
