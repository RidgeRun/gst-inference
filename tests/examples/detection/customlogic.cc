/* GstClassification
 * Copyright (C) 2019 RidgeRun <support@ridgerun.com>
 * All Rights Reserved.
 *
 *The contents of this software are proprietary and confidential to RidgeRun,
 *LLC.  No part of this program may be photocopied, reproduced or translated
 *into another programming language without prior written consent of
 *RidgeRun, LLC.  The user is free to modify the source code after obtaining
 *a software license from RidgeRun.  All source code changes must be provided
 *back to RidgeRun without any encumbrance.
 */

#include "customlogic.h"

#include <iostream>

void
handle_prediction (unsigned char *image,
    int width, int height, unsigned int size, PredictionBox * boxes,
    int num_boxes)
{
  /* FILLME:
   * Put here your custom logic, you may use C++ here.
   */

  for (int i = 0; i < num_boxes; ++i) {
    std::cout << "BBox: [class:" << boxes->category <<
        ", x:" << boxes->x << ", y:" << boxes->y <<
        ", width:" << boxes->width << ", height:" << boxes->height <<
        ", prob:" << boxes->probability << "]" << std::endl;
    boxes++;
  }
}
