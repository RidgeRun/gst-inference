/*
 * GStreamer
 * Copyright (C) 2018 RidgeRun
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

#include "gstvideoinference.h"

#include <gst/base/gstcollectpads.h>

static GstStaticPadTemplate sink_bypass_factory = GST_STATIC_PAD_TEMPLATE ("sink_bypass",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS ("ANY")
  );

static GstStaticPadTemplate src_bypass_factory = GST_STATIC_PAD_TEMPLATE ("src_bypass",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS ("ANY")
  );

GST_DEBUG_CATEGORY_STATIC (gst_video_inference_debug_category);
#define GST_CAT_DEFAULT gst_video_inference_debug_category

typedef struct _GstVideoInferencePrivate GstVideoInferencePrivate;
struct _GstVideoInferencePrivate
{
  GstCollectPads * cpads;
  GstPad * sink_bypass;
  GstPad * src_bypass;
  GstPad * sink_model;
  GstPad * src_model;
};

G_DEFINE_TYPE_WITH_CODE (GstVideoInference, gst_video_inference, GST_TYPE_ELEMENT,
    GST_DEBUG_CATEGORY_INIT (gst_video_inference_debug_category, "videoinference", 0,
        "debug category for videoinference base class");
     G_ADD_PRIVATE (GstVideoInference));

#define GST_VIDEO_INFERENCE_PRIVATE(self) \
  (GstVideoInferencePrivate *)(gst_video_inference_get_instance_private (self))


static void gst_video_inference_dispose (GObject * object);

static void
gst_video_inference_class_init (GstVideoInferenceClass * klass)
{
  GObjectClass *oclass = G_OBJECT_CLASS (klass);
  GstElementClass *eclass = GST_ELEMENT_CLASS (klass);

  oclass->dispose = gst_video_inference_dispose;

  gst_element_class_add_static_pad_template (eclass, &sink_bypass_factory);
  gst_element_class_add_static_pad_template (eclass, &src_bypass_factory);
}

static void
gst_video_inference_init (GstVideoInference * self)
{
  GstVideoInferencePrivate *priv = GST_VIDEO_INFERENCE_PRIVATE (self);

  priv->cpads = gst_collect_pads_new ();
  priv->sink_bypass = NULL;
  priv->src_bypass = NULL;
  priv->sink_model = NULL;
  priv->src_model = NULL;
}

static void
gst_video_inference_dispose (GObject * object)
{
  GstVideoInference *self = GST_VIDEO_INFERENCE (object);
  GstVideoInferencePrivate *priv = GST_VIDEO_INFERENCE_PRIVATE (self);

  g_clear_object (&(priv->cpads));
}
