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

GType
gst_inference_meta_api_get_type (void)
{
  static GType type = 0;
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

  /* Transfer Stream ID */
  g_free (dmeta->stream_id);
  dmeta->stream_id = g_strdup (smeta->stream_id);

  if (GST_META_TRANSFORM_IS_COPY (type)) {
    GST_LOG ("Copy inference metadata");

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

  /* Transfer Stream ID */
  g_free (dmeta->stream_id);
  dmeta->stream_id = g_strdup (smeta->stream_id);

  if (GST_META_TRANSFORM_IS_COPY (type)) {
    GST_LOG ("Copy inference metadata");

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
gst_inference_meta_init (GstMeta * meta, gpointer params, GstBuffer * buffer)
{
  GstInferenceMeta *imeta = (GstInferenceMeta *) meta;
  GstInferencePrediction *root;

  /* Create root Prediction */
  root = gst_inference_prediction_new ();

  imeta->prediction = root;
  imeta->stream_id = NULL;

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
  g_free (imeta->stream_id);
}
