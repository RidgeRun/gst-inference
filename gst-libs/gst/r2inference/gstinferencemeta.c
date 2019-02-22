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

static void gst_classification_meta_free (GstMeta * meta, GstBuffer * buffer);
static void gst_detection_meta_free (GstMeta * meta, GstBuffer * buffer);
static gboolean gst_inference_meta_init (GstMeta * meta, gpointer params,
    GstBuffer * buffer);
static gboolean gst_detection_meta_transform (GstBuffer * transbuf,
    GstMeta * meta, GstBuffer * buffer, GQuark type, gpointer data);
static gboolean gst_classification_meta_transform (GstBuffer * dest,
    GstMeta * meta, GstBuffer * buffer, GQuark type, gpointer data);

static void
gst_classification_meta_free (GstMeta * meta, GstBuffer * buffer)
{
  GstClassificationMeta *class_meta = (GstClassificationMeta *) meta;

  g_return_if_fail (meta != NULL);
  g_return_if_fail (buffer != NULL);

  if (class_meta->num_labels != 0) {
    g_free (class_meta->label_probs);
  }
}

GType
gst_classification_meta_api_get_type (void)
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
gst_classification_meta_get_info (void)
{
  static const GstMetaInfo *classification_meta_info = NULL;

  if (g_once_init_enter (&classification_meta_info)) {
    const GstMetaInfo *meta =
        gst_meta_register (GST_CLASSIFICATION_META_API_TYPE,
        "GstClassificationMeta", sizeof (GstClassificationMeta),
        gst_inference_meta_init, gst_classification_meta_free,
        gst_classification_meta_transform);
    g_once_init_leave (&classification_meta_info, meta);
  }
  return classification_meta_info;
}

static void
gst_detection_meta_free (GstMeta * meta, GstBuffer * buffer)
{
  GstDetectionMeta *detect_meta = (GstDetectionMeta *) meta;

  g_return_if_fail (meta != NULL);
  g_return_if_fail (buffer != NULL);

  if (detect_meta->num_boxes != 0) {
    g_free (detect_meta->boxes);
  }
}

GType
gst_detection_meta_api_get_type (void)
{
  static volatile GType type = 0;
  static const gchar *tags[] =
      { GST_META_TAG_VIDEO_STR, GST_META_TAG_VIDEO_ORIENTATION_STR,
    GST_META_TAG_VIDEO_SIZE_STR, NULL
  };

  if (g_once_init_enter (&type)) {
    GType _type = gst_meta_api_type_register ("GstDetectionMetaAPI", tags);
    g_once_init_leave (&type, _type);
  }
  return type;
}

/* detection metadata */
const GstMetaInfo *
gst_detection_meta_get_info (void)
{
  static const GstMetaInfo *detection_meta_info = NULL;

  if (g_once_init_enter (&detection_meta_info)) {
    const GstMetaInfo *meta =
        gst_meta_register (GST_DETECTION_META_API_TYPE, "GstDetectionMeta",
        sizeof (GstDetectionMeta), gst_inference_meta_init,
        gst_detection_meta_free,
        gst_detection_meta_transform);
    g_once_init_leave (&detection_meta_info, meta);
  }
  return detection_meta_info;
}

static gboolean
gst_inference_meta_init (GstMeta * meta, gpointer params, GstBuffer * buffer)
{
  return TRUE;
}

static gboolean
gst_detection_meta_transform (GstBuffer * dest, GstMeta * meta,
    GstBuffer * buffer, GQuark type, gpointer data)
{
  GstDetectionMeta *dmeta, *smeta;
  gsize raw_size;

  GST_LOG ("Transforming detection metadata");

  if (GST_META_TRANSFORM_IS_COPY (type)) {
    smeta = (GstDetectionMeta *) meta;
    dmeta =
        (GstDetectionMeta *) gst_buffer_add_meta (dest, GST_DETECTION_META_INFO,
        NULL);
    if (!dmeta) {
      return FALSE;
    }

    GST_LOG ("Copy detection metadata");
    dmeta->num_boxes = smeta->num_boxes;
    raw_size = dmeta->num_boxes * sizeof (BBox);
    dmeta->boxes = (BBox *) g_malloc (raw_size);
    memcpy (dmeta->boxes, smeta->boxes, raw_size);
  } else if (GST_VIDEO_META_TRANSFORM_IS_SCALE (type)) {
    return FALSE;
  } else {
    /* No transform supported */
    return FALSE;
  }

  return TRUE;
}

static gboolean
gst_classification_meta_transform (GstBuffer * dest, GstMeta * meta,
    GstBuffer * buffer, GQuark type, gpointer data)
{
  GstClassificationMeta *dmeta, *smeta;
  gsize raw_size;

  GST_LOG ("Transforming detection metadata");

  if (GST_META_TRANSFORM_IS_COPY (type)) {
    smeta = (GstClassificationMeta *) meta;
    dmeta =
        (GstClassificationMeta *) gst_buffer_add_meta (dest,
        GST_CLASSIFICATION_META_INFO, NULL);
    if (!dmeta) {
      return FALSE;
    }

    GST_LOG ("Copy classification metadata");
    dmeta->num_labels = smeta->num_labels;
    raw_size = dmeta->num_labels * sizeof (gdouble);
    dmeta->label_probs = (gdouble *) g_malloc (raw_size);
    memcpy (dmeta->label_probs, smeta->label_probs, raw_size);
  } else {
    /* No transform supported */
    return FALSE;
  }

  return TRUE;
}
