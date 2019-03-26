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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstclassificationoverlay.h"
#include "gstdetectionoverlay.h"
#include "gstembeddingoverlay.h"

static gboolean
plugin_init (GstPlugin * plugin)
{
  gboolean ret = TRUE;

  ret =
      gst_element_register (plugin, "classificationoverlay", GST_RANK_NONE,
      GST_TYPE_CLASSIFICATION_OVERLAY);
  if (!ret) {
    goto out;
  }

  ret =
      gst_element_register (plugin, "detectionoverlay", GST_RANK_NONE,
      GST_TYPE_DETECTION_OVERLAY);
  if (!ret) {
    goto out;
  }

  ret =
      gst_element_register (plugin, "embeddingoverlay", GST_RANK_NONE,
      GST_TYPE_EMBEDDING_OVERLAY);
  if (!ret) {
    goto out;
  }


out:
  return ret;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    inferenceoverlay,
    "Create overlays on incomming image frames with proper inference metadata",
    plugin_init, VERSION, "LGPL", PACKAGE_NAME, GST_PACKAGE_ORIGIN)
