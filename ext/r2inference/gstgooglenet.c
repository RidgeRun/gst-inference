/* 
 * Copyright (C) 2017 RidgeRun, LLC (http://www.ridgerun.com)
 * All Rights Reserved.
 *
 * The contents of this software are proprietary and confidential to RidgeRun,
 * LLC.  No part of this program may be photocopied, reproduced or translated
 * into another programming language without prior written consent of 
 * RidgeRun, LLC.  The user is free to modify the source code after obtaining
 * a software license from RidgeRun.  All source code changes must be provided
 * back to RidgeRun without any encumbrance.
 */

/**
 * SECTION:element-gstgooglenet
 *
 * The googlenet element allows the user to infer/execute a pretrained model
 * based on the GoogLeNet architecture on incoming image frames.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v videotestsrc ! googlenet ! xvimagesink
 * ]|
 * Process video frames from the camera using a GoogLeNet model.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include "gstgooglenet.h"

GST_DEBUG_CATEGORY_STATIC (gst_googlenet_debug_category);
#define GST_CAT_DEFAULT gst_googlenet_debug_category

/* prototypes */


static void gst_googlenet_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_googlenet_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_googlenet_dispose (GObject * object);
static void gst_googlenet_finalize (GObject * object);

#if 0
static GstCaps *gst_googlenet_transform_caps (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps, GstCaps * filter);
static GstCaps *gst_googlenet_fixate_caps (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps, GstCaps * othercaps);
static gboolean gst_googlenet_accept_caps (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps);
static gboolean gst_googlenet_set_caps (GstBaseTransform * trans,
    GstCaps * incaps, GstCaps * outcaps);
static gboolean gst_googlenet_query (GstBaseTransform * trans,
    GstPadDirection direction, GstQuery * query);
static gboolean gst_googlenet_decide_allocation (GstBaseTransform * trans,
    GstQuery * query);
static gboolean gst_googlenet_filter_meta (GstBaseTransform * trans,
    GstQuery * query, GType api, const GstStructure * params);
static gboolean gst_googlenet_propose_allocation (GstBaseTransform * trans,
    GstQuery * decide_query, GstQuery * query);
static gboolean gst_googlenet_transform_size (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps, gsize size, GstCaps * othercaps,
    gsize * othersize);
static gboolean gst_googlenet_get_unit_size (GstBaseTransform * trans,
    GstCaps * caps, gsize * size);
static gboolean gst_googlenet_start (GstBaseTransform * trans);
static gboolean gst_googlenet_stop (GstBaseTransform * trans);
static gboolean gst_googlenet_sink_event (GstBaseTransform * trans,
    GstEvent * event);
static gboolean gst_googlenet_src_event (GstBaseTransform * trans,
    GstEvent * event);
static GstFlowReturn gst_googlenet_prepare_output_buffer (GstBaseTransform *
    trans, GstBuffer * input, GstBuffer ** outbuf);
static gboolean gst_googlenet_copy_metadata (GstBaseTransform * trans,
    GstBuffer * input, GstBuffer * outbuf);
static gboolean gst_googlenet_transform_meta (GstBaseTransform * trans,
    GstBuffer * outbuf, GstMeta * meta, GstBuffer * inbuf);
static void gst_googlenet_before_transform (GstBaseTransform * trans,
    GstBuffer * buffer);
static GstFlowReturn gst_googlenet_transform (GstBaseTransform * trans,
    GstBuffer * inbuf, GstBuffer * outbuf);
#endif
static GstFlowReturn gst_googlenet_transform_ip (GstBaseTransform * trans,
    GstBuffer * buf);

enum
{
  PROP_0
};

/* pad templates */

static GstStaticPadTemplate gst_googlenet_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
    );

static GstStaticPadTemplate gst_googlenet_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
    );


/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstGooglenet, gst_googlenet, GST_TYPE_BASE_TRANSFORM,
    GST_DEBUG_CATEGORY_INIT (gst_googlenet_debug_category, "googlenet", 0,
        "debug category for googlenet element"));

static void
gst_googlenet_class_init (GstGooglenetClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseTransformClass *base_transform_class =
      GST_BASE_TRANSFORM_CLASS (klass);

  /* Setting up pads and setting metadata should be moved to
     base_class_init if you intend to subclass this class. */
  gst_element_class_add_pad_template (GST_ELEMENT_CLASS (klass),
      gst_static_pad_template_get (&gst_googlenet_src_template));
  gst_element_class_add_pad_template (GST_ELEMENT_CLASS (klass),
      gst_static_pad_template_get (&gst_googlenet_sink_template));

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "googlenet", "Filter", "Infers incoming image frames using a pretrained GoogLeNet model",
      "Carlos Rodriguez <carlos.rodriguez@ridgerun.com> \n\t\t\t"
      "   Jose Jimenez <jose.jimenez@ridgerun.com> \n\t\t\t"
      "   Michael Gruner <michael.gruner@ridgerun.com>");

  gobject_class->set_property = gst_googlenet_set_property;
  gobject_class->get_property = gst_googlenet_get_property;
  gobject_class->dispose = gst_googlenet_dispose;
  gobject_class->finalize = gst_googlenet_finalize;
#if 0
  base_transform_class->transform_caps =
      GST_DEBUG_FUNCPTR (gst_googlenet_transform_caps);
  base_transform_class->fixate_caps =
      GST_DEBUG_FUNCPTR (gst_googlenet_fixate_caps);
  base_transform_class->accept_caps =
      GST_DEBUG_FUNCPTR (gst_googlenet_accept_caps);
  base_transform_class->set_caps = GST_DEBUG_FUNCPTR (gst_googlenet_set_caps);
  base_transform_class->query = GST_DEBUG_FUNCPTR (gst_googlenet_query);
  base_transform_class->decide_allocation =
      GST_DEBUG_FUNCPTR (gst_googlenet_decide_allocation);
  base_transform_class->filter_meta =
      GST_DEBUG_FUNCPTR (gst_googlenet_filter_meta);
  base_transform_class->propose_allocation =
      GST_DEBUG_FUNCPTR (gst_googlenet_propose_allocation);
  base_transform_class->transform_size =
      GST_DEBUG_FUNCPTR (gst_googlenet_transform_size);
  base_transform_class->get_unit_size =
      GST_DEBUG_FUNCPTR (gst_googlenet_get_unit_size);
  base_transform_class->start = GST_DEBUG_FUNCPTR (gst_googlenet_start);
  base_transform_class->stop = GST_DEBUG_FUNCPTR (gst_googlenet_stop);
  base_transform_class->sink_event =
      GST_DEBUG_FUNCPTR (gst_googlenet_sink_event);
  base_transform_class->src_event =
      GST_DEBUG_FUNCPTR (gst_googlenet_src_event);
  base_transform_class->prepare_output_buffer =
      GST_DEBUG_FUNCPTR (gst_googlenet_prepare_output_buffer);
  base_transform_class->copy_metadata =
      GST_DEBUG_FUNCPTR (gst_googlenet_copy_metadata);
  base_transform_class->transform_meta =
      GST_DEBUG_FUNCPTR (gst_googlenet_transform_meta);
  base_transform_class->before_transform =
      GST_DEBUG_FUNCPTR (gst_googlenet_before_transform);
  base_transform_class->transform =
      GST_DEBUG_FUNCPTR (gst_googlenet_transform);
#endif
  base_transform_class->transform_ip =
      GST_DEBUG_FUNCPTR (gst_googlenet_transform_ip);

}

static void
gst_googlenet_init (GstGooglenet * googlenet)
{
}

void
gst_googlenet_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstGooglenet *googlenet = GST_GOOGLENET (object);

  GST_DEBUG_OBJECT (googlenet, "set_property");

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_googlenet_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstGooglenet *googlenet = GST_GOOGLENET (object);

  GST_DEBUG_OBJECT (googlenet, "get_property");

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_googlenet_dispose (GObject * object)
{
  GstGooglenet *googlenet = GST_GOOGLENET (object);

  GST_DEBUG_OBJECT (googlenet, "dispose");

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (gst_googlenet_parent_class)->dispose (object);
}

void
gst_googlenet_finalize (GObject * object)
{
  GstGooglenet *googlenet = GST_GOOGLENET (object);

  GST_DEBUG_OBJECT (googlenet, "finalize");

  /* clean up object here */

  G_OBJECT_CLASS (gst_googlenet_parent_class)->finalize (object);
}

#if 0
static GstCaps *
gst_googlenet_transform_caps (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps, GstCaps * filter)
{
  GstGooglenet *googlenet = GST_GOOGLENET (trans);
  GstCaps *othercaps;

  GST_DEBUG_OBJECT (googlenet, "transform_caps");

  othercaps = gst_caps_copy (caps);

  /* Copy other caps and modify as appropriate */
  /* This works for the simplest cases, where the transform modifies one
   * or more fields in the caps structure.  It does not work correctly
   * if passthrough caps are preferred. */
  if (direction == GST_PAD_SRC) {
    /* transform caps going upstream */
  } else {
    /* transform caps going downstream */
  }

  if (filter) {
    GstCaps *intersect;

    intersect = gst_caps_intersect (othercaps, filter);
    gst_caps_unref (othercaps);

    return intersect;
  } else {
    return othercaps;
  }
}

static GstCaps *
gst_googlenet_fixate_caps (GstBaseTransform * trans, GstPadDirection direction,
    GstCaps * caps, GstCaps * othercaps)
{
  GstGooglenet *googlenet = GST_GOOGLENET (trans);

  GST_DEBUG_OBJECT (googlenet, "fixate_caps");

  return NULL;
}

static gboolean
gst_googlenet_accept_caps (GstBaseTransform * trans, GstPadDirection direction,
    GstCaps * caps)
{
  GstGooglenet *googlenet = GST_GOOGLENET (trans);

  GST_DEBUG_OBJECT (googlenet, "accept_caps");

  return TRUE;
}

static gboolean
gst_googlenet_set_caps (GstBaseTransform * trans, GstCaps * incaps,
    GstCaps * outcaps)
{
  GstGooglenet *googlenet = GST_GOOGLENET (trans);

  GST_DEBUG_OBJECT (googlenet, "set_caps");

  return TRUE;
}

static gboolean
gst_googlenet_query (GstBaseTransform * trans, GstPadDirection direction,
    GstQuery * query)
{
  GstGooglenet *googlenet = GST_GOOGLENET (trans);

  GST_DEBUG_OBJECT (googlenet, "query");

  return TRUE;
}

/* decide allocation query for output buffers */
static gboolean
gst_googlenet_decide_allocation (GstBaseTransform * trans, GstQuery * query)
{
  GstGooglenet *googlenet = GST_GOOGLENET (trans);

  GST_DEBUG_OBJECT (googlenet, "decide_allocation");

  return TRUE;
}

static gboolean
gst_googlenet_filter_meta (GstBaseTransform * trans, GstQuery * query,
    GType api, const GstStructure * params)
{
  GstGooglenet *googlenet = GST_GOOGLENET (trans);

  GST_DEBUG_OBJECT (googlenet, "filter_meta");

  return TRUE;
}

/* propose allocation query parameters for input buffers */
static gboolean
gst_googlenet_propose_allocation (GstBaseTransform * trans,
    GstQuery * decide_query, GstQuery * query)
{
  GstGooglenet *googlenet = GST_GOOGLENET (trans);

  GST_DEBUG_OBJECT (googlenet, "propose_allocation");

  return TRUE;
}

/* transform size */
static gboolean
gst_googlenet_transform_size (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps, gsize size, GstCaps * othercaps,
    gsize * othersize)
{
  GstGooglenet *googlenet = GST_GOOGLENET (trans);

  GST_DEBUG_OBJECT (googlenet, "transform_size");

  return TRUE;
}

static gboolean
gst_googlenet_get_unit_size (GstBaseTransform * trans, GstCaps * caps,
    gsize * size)
{
  GstGooglenet *googlenet = GST_GOOGLENET (trans);

  GST_DEBUG_OBJECT (googlenet, "get_unit_size");

  return TRUE;
}

/* states */
static gboolean
gst_googlenet_start (GstBaseTransform * trans)
{
  GstGooglenet *googlenet = GST_GOOGLENET (trans);

  GST_DEBUG_OBJECT (googlenet, "start");

  return TRUE;
}

static gboolean
gst_googlenet_stop (GstBaseTransform * trans)
{
  GstGooglenet *googlenet = GST_GOOGLENET (trans);

  GST_DEBUG_OBJECT (googlenet, "stop");

  return TRUE;
}

/* sink and src pad event handlers */
static gboolean
gst_googlenet_sink_event (GstBaseTransform * trans, GstEvent * event)
{
  GstGooglenet *googlenet = GST_GOOGLENET (trans);

  GST_DEBUG_OBJECT (googlenet, "sink_event");

  return
      GST_BASE_TRANSFORM_CLASS (gst_googlenet_parent_class)->sink_event (trans,
      event);
}

static gboolean
gst_googlenet_src_event (GstBaseTransform * trans, GstEvent * event)
{
  GstGooglenet *googlenet = GST_GOOGLENET (trans);

  GST_DEBUG_OBJECT (googlenet, "src_event");

  return
      GST_BASE_TRANSFORM_CLASS (gst_googlenet_parent_class)->src_event (trans,
      event);
}

static GstFlowReturn
gst_googlenet_prepare_output_buffer (GstBaseTransform * trans,
    GstBuffer * input, GstBuffer ** outbuf)
{
  GstGooglenet *googlenet = GST_GOOGLENET (trans);

  GST_DEBUG_OBJECT (googlenet, "prepare_output_buffer");

  return GST_FLOW_OK;
}

/* metadata */
static gboolean
gst_googlenet_copy_metadata (GstBaseTransform * trans, GstBuffer * input,
    GstBuffer * outbuf)
{
  GstGooglenet *googlenet = GST_GOOGLENET (trans);

  GST_DEBUG_OBJECT (googlenet, "copy_metadata");

  return TRUE;
}

static gboolean
gst_googlenet_transform_meta (GstBaseTransform * trans, GstBuffer * outbuf,
    GstMeta * meta, GstBuffer * inbuf)
{
  GstGooglenet *googlenet = GST_GOOGLENET (trans);

  GST_DEBUG_OBJECT (googlenet, "transform_meta");

  return TRUE;
}

static void
gst_googlenet_before_transform (GstBaseTransform * trans, GstBuffer * buffer)
{
  GstGooglenet *googlenet = GST_GOOGLENET (trans);

  GST_DEBUG_OBJECT (googlenet, "before_transform");

}

/* transform */
static GstFlowReturn
gst_googlenet_transform (GstBaseTransform * trans, GstBuffer * inbuf,
    GstBuffer * outbuf)
{
  GstGooglenet *googlenet = GST_GOOGLENET (trans);

  GST_DEBUG_OBJECT (googlenet, "transform");

  return GST_FLOW_OK;
}
#endif

static GstFlowReturn
gst_googlenet_transform_ip (GstBaseTransform * trans, GstBuffer * buf)
{
  GstGooglenet *googlenet = GST_GOOGLENET (trans);

  GST_DEBUG_OBJECT (googlenet, "transform_ip");

  return GST_FLOW_OK;
}
