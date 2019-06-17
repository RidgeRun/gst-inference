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
#include <gst/check/gstcheck.h>
#include "gst/r2inference/gstinferencepostprocess.h"
#include "gst/r2inference/gstinferencemeta.h"

GST_START_TEST (test_gst_fill_classification_meta)
{
  GstClassificationMeta *class_meta;
  gpointer prediction;
  gsize predsize;
  gfloat values[2] = { 0.15, 0.75 };

  prediction = values;
  predsize = sizeof (values);
  class_meta = (void *) gst_classification_meta_api_get_type ();

  gst_fill_classification_meta (class_meta, prediction, predsize);

  fail_if (class_meta->num_labels != predsize / sizeof (gfloat));

  for (gint i = 0; i < class_meta->num_labels; i++) {
    fail_if (class_meta->label_probs[i] != values[i]);
  }
}

GST_END_TEST;

static Suite *
gst_inference_postprocess_suite (void)
{
  Suite *suite = suite_create ("GstInference");
  TCase *tc = tcase_create ("postprocess");

  suite_add_tcase (suite, tc);

  tcase_add_test (tc, test_gst_fill_classification_meta);

  return suite;
}

GST_CHECK_MAIN (gst_inference_postprocess);
