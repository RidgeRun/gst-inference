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

GST_START_TEST (test_gst_fill_classification_meta)
{

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
