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

#ifndef _GST_GOOGLENET_H_
#define _GST_GOOGLENET_H_

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

#define GST_TYPE_GOOGLENET   (gst_googlenet_get_type())
#define GST_GOOGLENET(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_GOOGLENET,GstGooglenet))
#define GST_GOOGLENET_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_GOOGLENET,GstGooglenetClass))
#define GST_IS_GOOGLENET(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_GOOGLENET))
#define GST_IS_GOOGLENET_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_GOOGLENET))

typedef struct _GstGooglenet GstGooglenet;
typedef struct _GstGooglenetClass GstGooglenetClass;

struct _GstGooglenet
{
  GstBaseTransform base_googlenet;

};

struct _GstGooglenetClass
{
  GstBaseTransformClass base_googlenet_class;
};

GType gst_googlenet_get_type (void);

G_END_DECLS

#endif
