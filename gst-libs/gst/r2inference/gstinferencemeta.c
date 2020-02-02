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

#include "gstinferencemeta.h"

#include <gst/video/video.h>
#include <string.h>

static gboolean gst_inference_meta_init (GstMeta * meta,
    gpointer params, GstBuffer * buffer);
static void gst_inference_meta_free (GstMeta * meta, GstBuffer * buffer);
static gboolean gst_inference_meta_transform (GstBuffer * transbuf,
    GstMeta * meta, GstBuffer * buffer, GQuark type, gpointer data);
static gboolean gst_inference_stream_meta_init (GstMeta * meta,
    gpointer params, GstBuffer * buffer);
static void gst_inference_stream_meta_free (GstMeta * meta, GstBuffer * buffer);
static gboolean gst_inference_stream_meta_transform (GstBuffer * transbuf,
    GstMeta * meta, GstBuffer * buffer, GQuark type, gpointer data);

static gboolean gst_classification_meta_init (GstMeta * meta,
    gpointer params, GstBuffer * buffer);
static void gst_classification_meta_free (GstMeta * meta, GstBuffer * buffer);
static void gst_detection_meta_free (GstMeta * meta, GstBuffer * buffer);
static gboolean gst_detection_meta_init (GstMeta * meta,
    gpointer params, GstBuffer * buffer);
static gboolean gst_detection_meta_transform (GstBuffer * transbuf,
    GstMeta * meta, GstBuffer * buffer, GQuark type, gpointer data);
static gboolean gst_classification_meta_transform (GstBuffer * dest,
    GstMeta * meta, GstBuffer * buffer, GQuark type, gpointer data);

static gboolean gst_detection_meta_copy (GstBuffer * transbuf,
    GstMeta * meta, GstBuffer * buffer);
static gboolean gst_detection_meta_scale (GstBuffer * transbuf,
    GstMeta * meta, GstBuffer * buffer, GstVideoMetaTransform * data);
static gboolean gst_classification_meta_copy (GstBuffer * transbuf,
    GstMeta * meta, GstBuffer * buffer);

GType
gst_inference_meta_api_get_type (void)
{
  static volatile GType type = 0;
  static const gchar *tags[] = { GST_META_TAG_VIDEO_STR, NULL
  };

  if (g_once_init_enter (&type)) {
    GType _type = gst_meta_api_type_register ("GstInferenceMetaAPI", tags);
    g_once_init_leave (&type, _type);
  }
  return type;
}

/* inference metadata */
const GstMetaInfo *
gst_inference_meta_get_info (void)
{
  static const GstMetaInfo *inference_meta_info = NULL;

  if (g_once_init_enter (&inference_meta_info)) {
    const GstMetaInfo *meta = gst_meta_register (GST_INFERENCE_META_API_TYPE,
        "GstInferenceMeta", sizeof (GstInferenceMeta),
        gst_inference_meta_init, gst_inference_meta_free,
        gst_inference_meta_transform);
    g_once_init_leave (&inference_meta_info, meta);
  }
  return inference_meta_info;
}

GType
gst_inference_stream_meta_api_get_type (void)
{
  static volatile GType type = 0;
  static const gchar *tags[] = { GST_META_TAG_VIDEO_STR, NULL
  };

  if (g_once_init_enter (&type)) {
    GType _type =
        gst_meta_api_type_register ("GstInferenceStreamMetaAPI", tags);
    g_once_init_leave (&type, _type);
  }
  return type;
}

/* inference stream metadata */
const GstMetaInfo *
gst_inference_stream_meta_get_info (void)
{
  static const GstMetaInfo *inference_stream_meta_info = NULL;

  if (g_once_init_enter (&inference_stream_meta_info)) {
    const GstMetaInfo *meta =
        gst_meta_register (GST_INFERENCE_STREAM_META_API_TYPE,
        "GstInferenceStreamMeta", sizeof (GstInferenceStreamMeta),
        gst_inference_stream_meta_init, gst_inference_stream_meta_free,
        gst_inference_stream_meta_transform);
    g_once_init_leave (&inference_stream_meta_info, meta);
  }
  return inference_stream_meta_info;
}

GType
gst_embedding_meta_api_get_type (void)
{
  static volatile GType type = 0;
  static const gchar *tags[] = { GST_META_TAG_VIDEO_STR, NULL };

  if (g_once_init_enter (&type)) {
    GType _type = gst_meta_api_type_register ("GstEmbeddingMetaAPI", tags);
    g_once_init_leave (&type, _type);
  }
  return type;
}

/* embedding metadata: As per now the embedding meta is ABI compatible
 * with classification. Reuse the meta methods.
 */
const GstMetaInfo *
gst_embedding_meta_get_info (void)
{
  static const GstMetaInfo *embedding_meta_info = NULL;

  if (g_once_init_enter (&embedding_meta_info)) {
    const GstMetaInfo *meta = gst_meta_register (GST_EMBEDDING_META_API_TYPE,
        "GstEmbeddingMeta", sizeof (GstEmbeddingMeta),
        gst_classification_meta_init, gst_classification_meta_free,
        gst_classification_meta_transform);
    g_once_init_leave (&embedding_meta_info, meta);
  }
  return embedding_meta_info;
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
        gst_classification_meta_init, gst_classification_meta_free,
        gst_classification_meta_transform);
    g_once_init_leave (&classification_meta_info, meta);
  }
  return classification_meta_info;
}

GType
gst_detection_meta_api_get_type (void)
{
  static volatile GType type = 0;
  static const gchar *tags[] = { GST_META_TAG_VIDEO_STR, NULL
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
        sizeof (GstDetectionMeta), gst_detection_meta_init,
        gst_detection_meta_free,
        gst_detection_meta_transform);
    g_once_init_leave (&detection_meta_info, meta);
  }
  return detection_meta_info;
}

static gboolean
gst_classification_meta_init (GstMeta * meta, gpointer params,
    GstBuffer * buffer)
{
  GstClassificationMeta *cmeta = (GstClassificationMeta *) meta;

  cmeta->label_probs = NULL;
  cmeta->num_labels = 0;

  return TRUE;
}

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

static gboolean
gst_inference_meta_transform_existing_meta (GstBuffer * dest, GstMeta * meta,
    GstBuffer * buffer, GQuark type, gpointer data)
{
  GstInferenceMeta *dmeta, *smeta;
  GstInferencePrediction *pred = NULL;
  gboolean ret = TRUE;
  gboolean needs_scale = FALSE;

  g_return_val_if_fail (dest, FALSE);
  g_return_val_if_fail (meta, FALSE);
  g_return_val_if_fail (buffer, FALSE);

  smeta = (GstInferenceMeta *) meta;
  dmeta =
      (GstInferenceMeta *) gst_buffer_get_meta (dest,
      gst_inference_meta_api_get_type ());

  g_return_val_if_fail (dmeta, FALSE);

  pred =
      gst_inference_prediction_find (dmeta->prediction,
      smeta->prediction->prediction_id);

  if (!pred) {
    GST_ERROR
        ("Predictions between metas do not match. Something really wrong happened");
    g_return_val_if_reached (FALSE);
  }

  needs_scale = gst_inference_prediction_merge (smeta->prediction, pred);

  if (GST_META_TRANSFORM_IS_COPY (type)) {
    GST_LOG ("Copy detection metadata");

    /* The merge already handled the copy */
    goto out;
  }


  if (GST_VIDEO_META_TRANSFORM_IS_SCALE (type)) {
    if (needs_scale) {
      GstVideoMetaTransform *trans = (GstVideoMetaTransform *) data;
      gst_inference_prediction_scale_ip (pred, trans->out_info, trans->in_info);

    }
    goto out;
  }

  /* Invalid transformation */
  gst_buffer_remove_meta (dest, (GstMeta *) dmeta);
  ret = FALSE;

out:
  gst_inference_prediction_unref (pred);
  return ret;
}

static gboolean
gst_inference_meta_transform_new_meta (GstBuffer * dest, GstMeta * meta,
    GstBuffer * buffer, GQuark type, gpointer data)
{
  GstInferenceMeta *dmeta, *smeta;

  g_return_val_if_fail (dest, FALSE);
  g_return_val_if_fail (meta, FALSE);
  g_return_val_if_fail (buffer, FALSE);

  smeta = (GstInferenceMeta *) meta;
  dmeta =
      (GstInferenceMeta *) gst_buffer_add_meta (dest, GST_INFERENCE_META_INFO,
      NULL);
  if (!dmeta) {
    GST_ERROR ("Unable to add meta to buffer");
    return FALSE;
  }

  gst_inference_prediction_unref (dmeta->prediction);

  if (GST_META_TRANSFORM_IS_COPY (type)) {
    GST_LOG ("Copy detection metadata");

    dmeta->prediction = gst_inference_prediction_copy (smeta->prediction);
    return TRUE;
  }

  if (GST_VIDEO_META_TRANSFORM_IS_SCALE (type)) {
    GstVideoMetaTransform *trans = (GstVideoMetaTransform *) data;

    dmeta->prediction =
        gst_inference_prediction_scale (smeta->prediction, trans->out_info,
        trans->in_info);
    return TRUE;
  }

  /* Invalid transformation */
  gst_buffer_remove_meta (dest, (GstMeta *) dmeta);
  return FALSE;
}

static gboolean
gst_inference_meta_transform (GstBuffer * dest, GstMeta * meta,
    GstBuffer * buffer, GQuark type, gpointer data)
{
  GstMeta *dmeta;

  g_return_val_if_fail (dest, FALSE);
  g_return_val_if_fail (meta, FALSE);
  g_return_val_if_fail (buffer, FALSE);

  GST_LOG ("Transforming inference metadata");

  dmeta = gst_buffer_get_meta (dest, gst_inference_meta_api_get_type ());
  if (!dmeta) {
    return gst_inference_meta_transform_new_meta (dest, meta, buffer, type,
        data);
  } else {
    return gst_inference_meta_transform_existing_meta (dest, meta, buffer, type,
        data);
  }
}

static gboolean
gst_detection_meta_init (GstMeta * meta, gpointer params, GstBuffer * buffer)
{
  GstDetectionMeta *dmeta = (GstDetectionMeta *) meta;

  dmeta->boxes = NULL;
  dmeta->num_boxes = 0;

  return TRUE;
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

static gboolean
gst_detection_meta_copy (GstBuffer * dest, GstMeta * meta, GstBuffer * buffer)
{
  GstDetectionMeta *dmeta, *smeta;
  gsize raw_size;

  g_return_val_if_fail (dest, FALSE);
  g_return_val_if_fail (meta, FALSE);
  g_return_val_if_fail (buffer, FALSE);

  smeta = (GstDetectionMeta *) meta;
  dmeta =
      (GstDetectionMeta *) gst_buffer_add_meta (dest, GST_DETECTION_META_INFO,
      NULL);
  if (!dmeta) {
    GST_ERROR ("Unable to add meta to buffer");
    return FALSE;
  }

  dmeta->num_boxes = smeta->num_boxes;
  raw_size = dmeta->num_boxes * sizeof (BBox);
  dmeta->boxes = (BBox *) g_malloc (raw_size);
  memcpy (dmeta->boxes, smeta->boxes, raw_size);

  return TRUE;
}

static gboolean
gst_detection_meta_scale (GstBuffer * dest,
    GstMeta * meta, GstBuffer * buffer, GstVideoMetaTransform * trans)
{
  GstDetectionMeta *dmeta, *smeta;
  gsize raw_size;
  gint ow, oh, nw, nh;
  gdouble hfactor, vfactor;

  g_return_val_if_fail (dest, FALSE);
  g_return_val_if_fail (meta, FALSE);
  g_return_val_if_fail (buffer, FALSE);
  g_return_val_if_fail (trans, FALSE);

  smeta = (GstDetectionMeta *) meta;
  dmeta =
      (GstDetectionMeta *) gst_buffer_add_meta (dest, GST_DETECTION_META_INFO,
      NULL);

  if (!dmeta) {
    GST_ERROR ("Unable to add meta to buffer");
    return FALSE;
  }

  ow = GST_VIDEO_INFO_WIDTH (trans->in_info);
  nw = GST_VIDEO_INFO_WIDTH (trans->out_info);
  oh = GST_VIDEO_INFO_HEIGHT (trans->in_info);
  nh = GST_VIDEO_INFO_HEIGHT (trans->out_info);

  g_return_val_if_fail (ow, FALSE);
  g_return_val_if_fail (oh, FALSE);

  dmeta->num_boxes = smeta->num_boxes;
  raw_size = dmeta->num_boxes * sizeof (BBox);
  dmeta->boxes = (BBox *) g_malloc (raw_size);

  hfactor = nw * 1.0 / ow;
  vfactor = nh * 1.0 / oh;

  GST_DEBUG ("Scaling detection metadata %dx%d -> %dx%d", ow, oh, nw, nh);
  for (gint i = 0; i < dmeta->num_boxes; ++i) {
    dmeta->boxes[i].x = smeta->boxes[i].x * hfactor;
    dmeta->boxes[i].y = smeta->boxes[i].y * vfactor;

    dmeta->boxes[i].width = smeta->boxes[i].width * hfactor;
    dmeta->boxes[i].height = smeta->boxes[i].height * vfactor;

    dmeta->boxes[i].label = smeta->boxes[i].label;
    dmeta->boxes[i].prob = smeta->boxes[i].prob;

    GST_LOG ("scaled bbox %d: %fx%f@%fx%f -> %fx%f@%fx%f",
        smeta->boxes[i].label, smeta->boxes[i].x, smeta->boxes[i].y,
        smeta->boxes[i].width, smeta->boxes[i].height, dmeta->boxes[i].x,
        dmeta->boxes[i].y, dmeta->boxes[i].width, dmeta->boxes[i].height);
  }
  return TRUE;
}

static gboolean
gst_detection_meta_transform (GstBuffer * dest, GstMeta * meta,
    GstBuffer * buffer, GQuark type, gpointer data)
{
  GST_LOG ("Transforming detection metadata");

  if (GST_META_TRANSFORM_IS_COPY (type)) {
    GST_LOG ("Copy detection metadata");
    return gst_detection_meta_copy (dest, meta, buffer);
  }

  if (GST_VIDEO_META_TRANSFORM_IS_SCALE (type)) {
    GstVideoMetaTransform *trans = (GstVideoMetaTransform *) data;
    return gst_detection_meta_scale (dest, meta, buffer, trans);
  }

  /* No transform supported */
  return FALSE;
}

static gboolean
gst_classification_meta_copy (GstBuffer * dest,
    GstMeta * meta, GstBuffer * buffer)
{
  GstClassificationMeta *dmeta, *smeta;
  gsize raw_size;

  smeta = (GstClassificationMeta *) meta;
  dmeta =
      (GstClassificationMeta *) gst_buffer_add_meta (dest,
      GST_CLASSIFICATION_META_INFO, NULL);

  if (!dmeta) {
    GST_ERROR ("Unable to add meta to buffer");
    return FALSE;
  }

  GST_LOG ("Copy classification metadata");
  dmeta->num_labels = smeta->num_labels;
  raw_size = dmeta->num_labels * sizeof (gdouble);
  dmeta->label_probs = (gdouble *) g_malloc (raw_size);
  memcpy (dmeta->label_probs, smeta->label_probs, raw_size);

  return TRUE;
}

static gboolean
gst_classification_meta_transform (GstBuffer * dest, GstMeta * meta,
    GstBuffer * buffer, GQuark type, gpointer data)
{
  GST_LOG ("Transforming detection metadata");

  /* TODO: Eventually check for the specific transformation here.
     Fail if its an unsupported transformation */
  return gst_classification_meta_copy (dest, meta, buffer);
}

/* inference metadata functions */
static gboolean
gst_inference_meta_init (GstMeta * meta, gpointer params, GstBuffer * buffer)
{
  GstInferenceMeta *imeta = (GstInferenceMeta *) meta;
  GstInferencePrediction *root;

  /* Create root Prediction */
  root = gst_inference_prediction_new ();

  imeta->prediction = root;

  return TRUE;
}

static void
gst_inference_meta_free (GstMeta * meta, GstBuffer * buffer)
{
  GstInferenceMeta *imeta = NULL;

  g_return_if_fail (meta != NULL);
  g_return_if_fail (buffer != NULL);

  imeta = (GstInferenceMeta *) meta;
  gst_inference_prediction_unref (imeta->prediction);
}

/* inference stream metadata functions */
static gboolean
gst_inference_stream_meta_init (GstMeta * meta, gpointer params,
    GstBuffer * buffer)
{
  GstInferenceStreamMeta *imeta = (GstInferenceStreamMeta *) meta;

  imeta->stream_id = NULL;

  return TRUE;
}

static void
gst_inference_stream_meta_free (GstMeta * meta, GstBuffer * buffer)
{
  GstInferenceStreamMeta *imeta = NULL;

  g_return_if_fail (meta != NULL);
  g_return_if_fail (buffer != NULL);

  imeta = (GstInferenceStreamMeta *) meta;
  if (imeta->stream_id)
    g_free (imeta->stream_id);
}

static gboolean
gst_inference_stream_meta_transform (GstBuffer * dest, GstMeta * meta,
    GstBuffer * buffer, GQuark type, gpointer data)
{
  GstInferenceStreamMeta *dmeta = NULL;
  GstInferenceStreamMeta *smeta = NULL;

  g_return_val_if_fail (dest, FALSE);
  g_return_val_if_fail (meta, FALSE);
  g_return_val_if_fail (buffer, FALSE);

  GST_LOG ("Transforming inference stream metadata");

  smeta = (GstInferenceStreamMeta *) meta;
  dmeta =
      (GstInferenceStreamMeta *) gst_buffer_get_meta (dest,
      gst_inference_stream_meta_api_get_type ());

  if (!dmeta) {
    dmeta =
        (GstInferenceStreamMeta *) gst_buffer_add_meta (dest,
        GST_INFERENCE_STREAM_META_INFO, NULL);
  } else {
    if (dmeta->stream_id) {
      g_free (dmeta->stream_id);
    }
  }

  dmeta->stream_id = g_strdup (smeta->stream_id);

  return TRUE;
}
