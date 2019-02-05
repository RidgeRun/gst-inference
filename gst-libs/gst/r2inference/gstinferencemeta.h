/* Copyright (C) 2019 RidgeRun, LLC (http://www.ridgerun.com)
 * All Rights Reserved.
 *
 * The contents of this software are proprietary and confidential to RidgeRun,
 * LLC.  No part of this program may be photocopied, reproduced or translated
 * into another programming language without prior written consent of
 * RidgeRun, LLC.  The user is free to modify the source code after obtaining
 * a software license from RidgeRun.  All source code changes must be provided
 * back to RidgeRun without any encumbrance.
*/

#ifndef GST_INFERENCE_META_H
#define GST_INFERENCE_META_H

/**
 * Basic bounding box structure for detection
 */
struct BBox {
  gint x;
  gint y;
  gint w;
  gint h;
  gint label;
  gdouble prob;
};

/**
 * Implements the placeholder for classification information.
 */
struct ClassificationMeta {
  GstMeta meta;
  gint num_labels;
  gdouble * probs;
};

/**
 * Implements the placeholder for detection information.
 */
struct DetectionMeta {
  GstMeta meta;
  gint num_boxes;
  BBox * boxes;
};

#endif // GST_INFERENCE_META_H
