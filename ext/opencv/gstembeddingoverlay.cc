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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstembeddingoverlay.h"

#include "gst/r2inference/gstinferencemeta.h"

#define DEFAULT_EMBEDDINGS NULL
#define DEFAULT_NUM_EMBEDDINGS 0
#define MIN_LIKENESS_THRESH 0.0
#define MAX_LIKENESS_THRESH G_MAXDOUBLE
#define DEFAULT_LIKENESS_THRESH 1.0

/* *INDENT-OFF* */
static const cv::Scalar forest_green = cv::Scalar (11, 102, 35);
static const cv::Scalar chilli_red = cv::Scalar (194, 24, 7);
static const cv::Scalar white = cv::Scalar (255, 255, 255, 255);
/* *INDENT-ON* */

GST_DEBUG_CATEGORY_STATIC (gst_embedding_overlay_debug_category);
#define GST_CAT_DEFAULT gst_embedding_overlay_debug_category

/* prototypes */
static void gst_embedding_overlay_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_embedding_overlay_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_embedding_overlay_finalize (GObject * object);
static GstFlowReturn gst_embedding_overlay_process_meta (GstInferenceBaseOverlay
    * inference_overlay, cv::Mat & cv_mat, GstVideoFrame * frame,
    GstMeta * meta, gdouble font_scale, gint thickness, gchar ** labels_list,
    gint num_labels, LineStyleBoundingBox style);
static gboolean gst_embedding_overlay_set_embeddings (GstEmbeddingOverlay *
    embedding_overlay, const GValue * value);

enum
{
  PROP_0,
  PROP_EMBEDDINGS,
  PROP_LIKENESS_THRESH
};

struct _GstEmbeddingOverlay
{
  GstInferenceBaseOverlay parent;
  gchar *embeddings;
  gchar **embeddings_list;
  gint num_embeddings;
  gint embedding_size;
  gdouble likeness_thresh;
};

struct _GstClassificationOverlayClass
{
  GstInferenceBaseOverlay parent;
};

/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstEmbeddingOverlay, gst_embedding_overlay,
    GST_TYPE_INFERENCE_BASE_OVERLAY,
    GST_DEBUG_CATEGORY_INIT (gst_embedding_overlay_debug_category,
        "embeddingoverlay", 0, "debug category for embedding_overlay element"));

static void
gst_embedding_overlay_class_init (GstEmbeddingOverlayClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstInferenceBaseOverlayClass *io_class =
      GST_INFERENCE_BASE_OVERLAY_CLASS (klass);

  gobject_class->set_property = gst_embedding_overlay_set_property;
  gobject_class->get_property = gst_embedding_overlay_get_property;
  gobject_class->finalize = gst_embedding_overlay_finalize;

  g_object_class_install_property (gobject_class, PROP_EMBEDDINGS,
      g_param_spec_string ("embeddings", "embeddings",
          "String containing the embeddings. Consecutive values are \n\t\t\t"
          "separated with a space and embedding with a semicolon.",
          DEFAULT_EMBEDDINGS, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_LIKENESS_THRESH,
      g_param_spec_double ("likeness-threshold", "likeness-thresh",
          "Likeness threshold to compare distance between embeddings.\n\t\t\t"
          "For lower values the algorithm is stricter. 1.0 is recommended.",
          MIN_LIKENESS_THRESH, MAX_LIKENESS_THRESH, DEFAULT_LIKENESS_THRESH,
          G_PARAM_READWRITE));

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "embeddingoverlay", "Filter",
      "Overlays classification metadata on input buffer",
      "Carlos Rodriguez <carlos.rodriguez@ridgerun.com> \n\t\t\t"
      "   Jose Jimenez <jose.jimenez@ridgerun.com> \n\t\t\t"
      "   Michael Gruner <michael.gruner@ridgerun.com> \n\t\t\t"
      "   Carlos Aguero <carlos.aguero@ridgerun.com> \n\t\t\t"
      "   Miguel Taylor <miguel.taylor@ridgerun.com> \n\t\t\t"
      "   Greivin Fallas <greivin.fallas@ridgerun.com>");

  io_class->process_meta =
      GST_DEBUG_FUNCPTR (gst_embedding_overlay_process_meta);
  io_class->meta_type = GST_CLASSIFICATION_META_API_TYPE;
}

static void
gst_embedding_overlay_init (GstEmbeddingOverlay * embedding_overlay)
{
  embedding_overlay->embeddings = DEFAULT_EMBEDDINGS;
  embedding_overlay->embeddings_list = DEFAULT_EMBEDDINGS;
  embedding_overlay->num_embeddings = DEFAULT_NUM_EMBEDDINGS;
  embedding_overlay->likeness_thresh = DEFAULT_LIKENESS_THRESH;
}

void
gst_embedding_overlay_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstEmbeddingOverlay *embedding_overlay = GST_EMBEDDING_OVERLAY (object);

  GST_DEBUG_OBJECT (embedding_overlay, "set_property");

  switch (property_id) {
    case PROP_EMBEDDINGS:
      if (gst_embedding_overlay_set_embeddings (embedding_overlay, value)) {
        GST_DEBUG_OBJECT (embedding_overlay, "Changed inference labels %s",
            embedding_overlay->embeddings);
      } else {
        GST_ERROR_OBJECT (embedding_overlay, "Failed setting embeddings");
      }
      break;
    case PROP_LIKENESS_THRESH:
      GST_OBJECT_LOCK (embedding_overlay);
      embedding_overlay->likeness_thresh = g_value_get_double (value);
      GST_DEBUG_OBJECT (embedding_overlay,
          "Changed likeness threshold to %lf",
          embedding_overlay->likeness_thresh);
      GST_OBJECT_UNLOCK (embedding_overlay);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_embedding_overlay_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstEmbeddingOverlay *embedding_overlay = GST_EMBEDDING_OVERLAY (object);

  GST_DEBUG_OBJECT (embedding_overlay, "get_property");

  switch (property_id) {
    case PROP_EMBEDDINGS:
      GST_OBJECT_LOCK (embedding_overlay);
      g_value_set_string (value, embedding_overlay->embeddings);
      GST_OBJECT_UNLOCK (embedding_overlay);
      break;
    case PROP_LIKENESS_THRESH:
      GST_OBJECT_LOCK (embedding_overlay);
      g_value_set_double (value, embedding_overlay->likeness_thresh);
      GST_OBJECT_UNLOCK (embedding_overlay);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_embedding_overlay_finalize (GObject * object)
{
  GstEmbeddingOverlay *embedding_overlay = GST_EMBEDDING_OVERLAY (object);

  GST_DEBUG_OBJECT (embedding_overlay, "finalize");

  /* clean up as possible.  may be called multiple times */
  if (embedding_overlay->embeddings != NULL) {
    g_free (embedding_overlay->embeddings);
  }
  if (embedding_overlay->embeddings_list != NULL) {
    g_strfreev (embedding_overlay->embeddings_list);
  }

  G_OBJECT_CLASS (gst_embedding_overlay_parent_class)->finalize (object);
}

static GstFlowReturn
gst_embedding_overlay_process_meta (GstInferenceBaseOverlay * inference_overlay,
    cv::Mat & cv_mat, GstVideoFrame * frame, GstMeta * meta, gdouble font_scale,
    gint thickness, gchar ** labels_list, gint num_labels,
    LineStyleBoundingBox style)
{
  GstEmbeddingOverlay *embedding_overlay =
      GST_EMBEDDING_OVERLAY (inference_overlay);
  GstClassificationMeta *class_meta;
  gint i, j, width, height;
  gdouble current, diff;
  cv::String str;
  cv::Size size;
  cv::Scalar tmp_color;
  cv::Scalar color = cv::Scalar (0, 0, 0, 0);

  gchar **embeddings_list;
  gint num_embeddings;
  gint embedding_size;
  gdouble likeness_thresh;

  GST_OBJECT_LOCK (embedding_overlay);
  embeddings_list = g_strdupv (embedding_overlay->embeddings_list);
  num_embeddings = embedding_overlay->num_embeddings;
  embedding_size = embedding_overlay->embedding_size;
  likeness_thresh = embedding_overlay->likeness_thresh;
  GST_OBJECT_UNLOCK (embedding_overlay);

  if (num_embeddings == 0) {
    GST_WARNING_OBJECT (embedding_overlay,
        "Please set at least one valid embedding using the 'embeddings'"
        "property");
    goto end;
  }

  class_meta = (GstClassificationMeta *) meta;

  if (class_meta->num_labels != embedding_size) {
    GST_WARNING_OBJECT (embedding_overlay,
        "Provided embeddings and inference output have different sizes");
    goto end;
  }

  str = cv::format ("Fail");
  tmp_color[0] = chilli_red[0];
  tmp_color[1] = chilli_red[1];
  tmp_color[2] = chilli_red[2];
  for (i = 0; i < num_embeddings; ++i) {
    diff = 0.0;
    for (j = 0; j < embedding_size; ++j) {
      current = class_meta->label_probs[j];
      current -= atof (embeddings_list[i * embedding_size + j]);
      current = current * current;
      diff = diff + current;
    }
    if (diff < likeness_thresh) {
      if (num_labels > i) {
        str = cv::format ("Pass: %s", labels_list[i]);
      } else {
        str = cv::format ("Pass");
      }
      tmp_color[0] = forest_green[0];
      tmp_color[1] = forest_green[1];
      tmp_color[2] = forest_green[2];
      break;
    }
  }

  /* Convert color according to colorspace */
  switch (GST_VIDEO_FRAME_FORMAT (frame)) {
    case GST_VIDEO_FORMAT_BGR:
    case GST_VIDEO_FORMAT_BGRx:
    case GST_VIDEO_FORMAT_BGRA:
      color[0] = tmp_color[2];
      color[1] = tmp_color[1];
      color[2] = tmp_color[0];
      break;
    case GST_VIDEO_FORMAT_xRGB:
    case GST_VIDEO_FORMAT_ARGB:
      color[1] = tmp_color[0];
      color[2] = tmp_color[1];
      color[3] = tmp_color[2];
      break;
    case GST_VIDEO_FORMAT_xBGR:
    case GST_VIDEO_FORMAT_ABGR:
      color[1] = tmp_color[2];
      color[2] = tmp_color[1];
      color[3] = tmp_color[0];
      break;
    default:
      color = tmp_color;
      break;
  }

  width = cv_mat.cols;
  height = cv_mat.rows;

  /* Put string on screen
   * 10*font_scale+16 aproximates text's rendered size on screen as a
   * lineal function to avoid using cv::getTextSize.
   * 5*thickness adds the border offset
   */
  cv::putText (cv_mat, str, cv::Point (5 * thickness,
          5 * thickness + 10 * font_scale + 16), cv::FONT_HERSHEY_PLAIN,
      font_scale, white, thickness + (thickness * 0.5));
  cv::putText (cv_mat, str, cv::Point (5 * thickness,
          5 * thickness + 10 * font_scale + 16), cv::FONT_HERSHEY_PLAIN,
      font_scale, color, thickness);
  cv::rectangle (cv_mat, cv::Point (0, 0), cv::Point (width, height), color,
      10 * thickness);

end:
  g_strfreev (embeddings_list);
  return GST_FLOW_OK;
}

gboolean
gst_embedding_overlay_set_embeddings (GstEmbeddingOverlay * embedding_overlay,
    const GValue * value)
{
  g_return_val_if_fail (embedding_overlay, FALSE);
  g_return_val_if_fail (value, FALSE);

  GST_OBJECT_LOCK (embedding_overlay);
  if (embedding_overlay->embeddings != NULL) {
    g_free (embedding_overlay->embeddings);
  }
  if (embedding_overlay->embeddings_list != NULL) {
    g_strfreev (embedding_overlay->embeddings_list);
  }
  embedding_overlay->embeddings = g_value_dup_string (value);
  embedding_overlay->embeddings_list =
      g_strsplit (g_value_get_string (value), ";", 0);
  embedding_overlay->num_embeddings =
      g_strv_length (embedding_overlay->embeddings_list);
  g_strfreev (embedding_overlay->embeddings_list);
  embedding_overlay->embeddings_list =
      g_strsplit_set (g_value_get_string (value), "; ", 0);
  embedding_overlay->embedding_size =
      g_strv_length (embedding_overlay->embeddings_list) /
      embedding_overlay->num_embeddings;
  GST_OBJECT_UNLOCK (embedding_overlay);

  return TRUE;
}
