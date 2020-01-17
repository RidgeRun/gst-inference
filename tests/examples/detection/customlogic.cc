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
