/*
 * GStreamer
 * Copyright (C) 2019 RidgeRun
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

#ifndef _GST_CLASSIFICATION_OVERLAY_H_
#define _GST_CLASSIFICATION_OVERLAY_H_

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>

G_BEGIN_DECLS

#define GST_TYPE_CLASSIFICATION_OVERLAY   (gst_classification_overlay_get_type())
#define GST_CLASSIFICATION_OVERLAY(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_CLASSIFICATION_OVERLAY,GstClassificationOverlay))
#define GST_CLASSIFICATION_OVERLAY_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_CLASSIFICATION_OVERLAY,GstClassificationOverlayClass))
#define GST_IS_CLASSIFICATION_OVERLAY(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_CLASSIFICATION_OVERLAY))
#define GST_IS_CLASSIFICATION_OVERLAY_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_CLASSIFICATION_OVERLAY))

typedef struct _GstClassificationOverlay GstClassificationOverlay;
typedef struct _GstClassificationOverlayClass GstClassificationOverlayClass;

struct _GstClassificationOverlay
{
  GstVideoFilter base_classification_overlay;

  gdouble font_scale;
  gint box_thickness;
  gchar* labels;
  gchar** labels_list;
  gint num_labels;

};

struct _GstClassificationOverlayClass
{
  GstVideoFilterClass base_classification_overlay_class;
};

GType gst_classification_overlay_get_type (void);

G_END_DECLS

#endif
