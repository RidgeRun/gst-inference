/*
 * GStreamer
 * Copyright (C) 2018-2021 RidgeRun <support@ridgerun.com>
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

/**
 * SECTION:element-gstmobilenetv2ssd
 *
 * The mobilenetv2ssd element allows the user to infer/execute a pretrained 
 * model based on the MobilenetV2 + SSD architecture on incoming image frames.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v videotestsrc ! mobilenetv2ssd ! xvimagesink
 * ]|
 * Process video frames from the camera using a MobilenetV2 + SSD model.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstmobilenetv2ssd.h"

#include "gst/r2inference/gstinferencemeta.h"
#include "gst/r2inference/gstinferencepostprocess.h"
#include "gst/r2inference/gstinferencepreprocess.h"

GST_DEBUG_CATEGORY_STATIC (gst_mobilenetv2ssd_debug_category);
#define GST_CAT_DEFAULT gst_mobilenetv2ssd_debug_category

#define MODEL_CHANNELS 3
#define MEAN 127.5
#define STD 1 / MEAN

/* Class probability threshold */
#define MAX_PROB_THRESH 1
#define MIN_PROB_THRESH 0
#define DEFAULT_PROB_THRESH 0.50

#define TOTAL_CLASSES 90

/* prototypes */
static void gst_mobilenetv2ssd_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_mobilenetv2ssd_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static gboolean gst_mobilenetv2ssd_preprocess (GstVideoInference * vi,
    GstVideoFrame * inframe, GstVideoFrame * outframe);
static gboolean
gst_mobilenetv2ssd_postprocess (GstVideoInference * vi,
    const gpointer prediction, gsize predsize, GstMeta * meta_model[2],
    GstVideoInfo * info_model, gboolean * valid_prediction,
    gchar ** labels_list, gint num_labels);

enum
{
  PROP_0,
  PROP_PROB_THRESH,
};

/* pad templates */
#define CAPS								\
  "video/x-raw, "							\
  "width=300, "								\
  "height=300, "							\
  "format={RGB, RGBx, RGBA, BGR, BGRx, BGRA, xRGB, ARGB, xBGR, ABGR}"

static GstStaticPadTemplate sink_model_factory =
GST_STATIC_PAD_TEMPLATE ("sink_model",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS (CAPS)
    );

static GstStaticPadTemplate src_model_factory =
GST_STATIC_PAD_TEMPLATE ("src_model",
    GST_PAD_SRC,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS (CAPS)
    );

struct _GstMobilenetv2ssd
{
  GstVideoInference parent;

  gdouble prob_thresh;
};

struct _GstMobilenetv2ssdClass
{
  GstVideoInferenceClass parent;
};

/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstMobilenetv2ssd, gst_mobilenetv2ssd,
    GST_TYPE_VIDEO_INFERENCE,
    GST_DEBUG_CATEGORY_INIT (gst_mobilenetv2ssd_debug_category,
        "mobilenetv2ssd", 0, "debug category for mobilenetv2ssd element"));

static void
gst_mobilenetv2ssd_class_init (GstMobilenetv2ssdClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  GstVideoInferenceClass *vi_class = GST_VIDEO_INFERENCE_CLASS (klass);

  gst_element_class_add_static_pad_template (element_class,
      &sink_model_factory);
  gst_element_class_add_static_pad_template (element_class, &src_model_factory);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "mobilenetv2ssd", "Filter",
      "Infers incoming image frames using a pretrained MobilenetV2 + SSD model",
      "Jimena Salas <jimena.salas@ridgerun.com>");

  gobject_class->set_property = gst_mobilenetv2ssd_set_property;
  gobject_class->get_property = gst_mobilenetv2ssd_get_property;

  g_object_class_install_property (gobject_class, PROP_PROB_THRESH,
      g_param_spec_double ("prob-thresh", "Probability threshold",
          "Class probability threshold", MIN_PROB_THRESH, MAX_PROB_THRESH,
          DEFAULT_PROB_THRESH, G_PARAM_READWRITE));

  vi_class->preprocess = GST_DEBUG_FUNCPTR (gst_mobilenetv2ssd_preprocess);
  vi_class->postprocess = GST_DEBUG_FUNCPTR (gst_mobilenetv2ssd_postprocess);
  vi_class->inference_meta_info = gst_detection_meta_get_info ();
}

static void
gst_mobilenetv2ssd_init (GstMobilenetv2ssd * mobilenetv2ssd)
{
  mobilenetv2ssd->prob_thresh = DEFAULT_PROB_THRESH;
}

static gboolean
gst_mobilenetv2ssd_preprocess (GstVideoInference * vi,
    GstVideoFrame * inframe, GstVideoFrame * outframe)
{
  g_return_val_if_fail (vi, FALSE);
  g_return_val_if_fail (inframe, FALSE);
  g_return_val_if_fail (outframe, FALSE);

  GST_LOG_OBJECT (vi, "Preprocess");

  return gst_normalize (inframe, outframe, MEAN, STD, MODEL_CHANNELS);
}

static gboolean
gst_mobilenetv2ssd_postprocess (GstVideoInference * vi,
    const gpointer prediction, gsize predsize, GstMeta * meta_model[2],
    GstVideoInfo * info_model, gboolean * valid_prediction,
    gchar ** labels_list, gint num_labels)
{
  gboolean ret = TRUE;

  g_return_val_if_fail (vi, FALSE);
  g_return_val_if_fail (prediction, FALSE);
  g_return_val_if_fail (meta_model, FALSE);
  g_return_val_if_fail (info_model, FALSE);
  g_return_val_if_fail (valid_prediction, FALSE);

  GST_LOG_OBJECT (vi, "Postprocess");

  return ret;
}

static void
gst_mobilenetv2ssd_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstMobilenetv2ssd *mobilenetv2ssd = GST_MOBILENETV2SSD (object);

  GST_DEBUG_OBJECT (mobilenetv2ssd, "set_property");

  switch (property_id) {
    case PROP_PROB_THRESH:
      mobilenetv2ssd->prob_thresh = g_value_get_double (value);
      GST_DEBUG_OBJECT (mobilenetv2ssd,
          "Changed probability threshold to %lf", mobilenetv2ssd->prob_thresh);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
gst_mobilenetv2ssd_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstMobilenetv2ssd *mobilenetv2ssd = GST_MOBILENETV2SSD (object);

  GST_DEBUG_OBJECT (mobilenetv2ssd, "get_property");

  switch (property_id) {
    case PROP_PROB_THRESH:
      g_value_set_double (value, mobilenetv2ssd->prob_thresh);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}
