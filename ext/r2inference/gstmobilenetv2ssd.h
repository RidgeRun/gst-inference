/*
 * GStreamer
 * Copyright (C) 2021 RidgeRun <support@ridgerun.com>
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

#ifndef _GST_MOBILENETV2SSD_H_
#define _GST_MOBILENETV2SSD_H_

#include <gst/r2inference/gstvideoinference.h>

G_BEGIN_DECLS

#define GST_TYPE_MOBILENETV2SSD gst_mobilenetv2ssd_get_type ()
G_DECLARE_FINAL_TYPE (GstMobilenetv2ssd, gst_mobilenetv2ssd, GST, MOBILENETV2SSD, GstVideoInference)

G_END_DECLS

#endif /* _GST_MOBILENETV2SSD_H_ */
