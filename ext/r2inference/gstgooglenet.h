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

#include <gst/r2inference/gstvideoinference.h>

G_BEGIN_DECLS

#define GST_TYPE_GOOGLENET gst_googlenet_get_type ()
G_DECLARE_FINAL_TYPE (GstGooglenet, gst_googlenet, GST, GOOGLENET, GstVideoInference)

G_END_DECLS

#endif
