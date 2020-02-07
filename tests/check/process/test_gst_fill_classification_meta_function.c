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
  class_meta =
      (GstClassificationMeta *) gst_classification_meta_api_get_type ();

  gst_fill_classification_meta (class_meta, prediction, predsize);
  fail_if (class_meta->num_labels != predsize);
  for (gint i = 0; i < class_meta->num_labels; i++) {
    fail_if (class_meta->label_probs[i] != values[i]);
  }
}

GST_END_TEST;

GST_START_TEST (test_gst_fill_classification_meta_zero_size)
{
  GstClassificationMeta *class_meta;
  gpointer prediction;
  gsize predsize;
  gfloat values[2] = { 0.15, 0.75 };

  prediction = values;
  predsize = 0;
  class_meta =
      (GstClassificationMeta *) gst_classification_meta_api_get_type ();

  gst_fill_classification_meta (class_meta, prediction, predsize);

  fail_if (class_meta->num_labels != 0);
}

GST_END_TEST;

GST_START_TEST (test_gst_fill_classification_meta_null_meta)
{
  gpointer prediction;
  gsize predsize;
  gfloat values[2] = { 0.15, 0.75 };

  prediction = values;
  predsize = 0;

  ASSERT_CRITICAL (gst_fill_classification_meta (NULL, prediction, predsize));
}

GST_END_TEST;

GST_START_TEST (test_gst_fill_classification_meta_null_predictions)
{
  GstClassificationMeta *class_meta;
  gsize predsize;

  predsize = 2;
  class_meta =
      (GstClassificationMeta *) gst_classification_meta_api_get_type ();

  ASSERT_CRITICAL (gst_fill_classification_meta (class_meta, NULL, predsize));
}

GST_END_TEST;

static Suite *
gst_fill_classification_meta_suite (void)
{
  Suite *suite = suite_create ("GstInference");
  TCase *tc = tcase_create ("gst_fill_classification_meta");

  suite_add_tcase (suite, tc);

  tcase_add_test (tc, test_gst_fill_classification_meta);
  tcase_add_test (tc, test_gst_fill_classification_meta_zero_size);
  tcase_add_test (tc, test_gst_fill_classification_meta_null_meta);
  tcase_add_test (tc, test_gst_fill_classification_meta_null_predictions);

  return suite;
}

GST_CHECK_MAIN (gst_fill_classification_meta);
