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

/* GObject methods */
static void gst_video_inference_dispose (GObject * object);

/* GstElement methods */
static GstStateChangeReturn gst_video_inference_change_state (GstElement *element,
    GstStateChange transition);

/* GstVideoInference methods */
static gboolean gst_video_inference_start (GstVideoInference *self);
static gboolean gst_video_inference_stop (GstVideoInference *self);

static void
gst_video_inference_class_init (GstVideoInferenceClass * klass)
{
  GObjectClass *oclass = G_OBJECT_CLASS (klass);
  GstElementClass *eclass = GST_ELEMENT_CLASS (klass);

  oclass->dispose = gst_video_inference_dispose;

  eclass->change_state = GST_DEBUG_FUNCPTR (gst_video_inference_change_state);
  gst_element_class_add_static_pad_template (eclass, &sink_bypass_factory);
  gst_element_class_add_static_pad_template (eclass, &src_bypass_factory);

  klass->start = NULL;
  klass->stop = NULL;
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

static gboolean
gst_video_inference_start (GstVideoInference *self)
{
  GstVideoInferenceClass *klass = GST_VIDEO_INFERENCE_GET_CLASS (self);
  gboolean ret = TRUE;

  if (klass->start != NULL) {
    ret = klass->start (self);
  }

  return ret;
}

static gboolean
gst_video_inference_stop (GstVideoInference *self)
{
  GstVideoInferenceClass *klass = GST_VIDEO_INFERENCE_GET_CLASS (self);
  gboolean ret = TRUE;

  if (klass->stop != NULL) {
    ret = klass->stop (self);
  }

  return ret;
}

static GstStateChangeReturn
gst_video_inference_change_state (GstElement *element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstVideoInference *self = GST_VIDEO_INFERENCE (element);
  GstVideoInferencePrivate *priv = GST_VIDEO_INFERENCE_PRIVATE (self);

 switch (transition) {
 case GST_STATE_CHANGE_READY_TO_PAUSED:
   gst_collect_pads_start (priv->cpads);

   if (FALSE == gst_video_inference_start (self)) {
     GST_ERROR_OBJECT (self, "Subclass failed to start");
     ret = GST_STATE_CHANGE_FAILURE;
     goto out;
   }
   break;
 case GST_STATE_CHANGE_PAUSED_TO_READY:
   gst_collect_pads_stop (priv->cpads);
   break;
 default:
   break;
 }

 ret = GST_ELEMENT_CLASS (gst_video_inference_parent_class)->change_state (element,
    transition);
 if (GST_STATE_CHANGE_FAILURE == ret) {
   GST_ERROR_OBJECT (self, "Parent failed to change state");
   goto out;
 }

 switch (transition) {
 case GST_STATE_CHANGE_PAUSED_TO_READY:
   if (FALSE == gst_video_inference_stop (self)) {
     GST_ERROR_OBJECT (self, "Subclass failed to stop");
     ret = GST_STATE_CHANGE_FAILURE;
     goto out;
   }
   break;
 default:
   break;
 }

 out:
 return ret;
}

static void
gst_video_inference_dispose (GObject * object)
{
  GstVideoInference *self = GST_VIDEO_INFERENCE (object);
  GstVideoInferencePrivate *priv = GST_VIDEO_INFERENCE_PRIVATE (self);

  g_clear_object (&(priv->cpads));
}
