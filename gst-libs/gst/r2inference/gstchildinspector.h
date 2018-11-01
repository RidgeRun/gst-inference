/*  
 *  Copyright (C) 2016 RidgeRun, LLC (http://www.ridgerun.com)
 *  All Rights Reserved. 
 *
 *  The contents of this software are proprietary and confidential to RidgeRun, 
 *  LLC.  No part of this program may be photocopied, reproduced or translated 
 *  into another programming language without prior written consent of
 *  RidgeRun, LLC.  The user is free to modify the source code after obtaining 
 *  a software license from RidgeRun.  All source code changes must be provided 
 *  back to RidgeRun without any encumbrance. 
 */

#ifndef __GST_CHILD_INSPECTOR_H__
#define __GST_CHILD_INSPECTOR_H__

#include <gst/gst.h>

G_BEGIN_DECLS

gchar * gst_child_inspector_property_to_string (GObject * object,
    GParamSpec * param, guint alignment);
gchar * gst_child_inspector_properties_to_string (GObject * object,
    guint alignment, gchar * title);

G_END_DECLS

#endif /* __GST_CHILD_INSPECTOR_H__ */
