/*
 * GStreamer
 * Copyright (C) 2018-2020 RidgeRun <support@ridgerun.com>
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

#include "gstinferencedebug.h"

void
gst_inference_print_predictions (GstVideoInference * vi,
    GstDebugCategory * category, GstInferenceMeta * inference_meta)
{
  GstInferencePrediction *pred = NULL;
  gchar *spred = NULL;

  g_return_if_fail (vi != NULL);
  g_return_if_fail (category != NULL);
  g_return_if_fail (inference_meta != NULL);

  pred = inference_meta->prediction;
  spred = gst_inference_prediction_to_string (pred);

  GST_CAT_LOG (category, "\n%s", spred);

  g_free (spred);
}
