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

#ifndef __DETECTION_CUSTOM_LOGIC__
#define __DETECTION_CUSTOM_LOGIC__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _PredictionBox PredictionBox;
struct _PredictionBox {
  int category;
  double probability;
  int x;
  int y;
  int width;
  int height;
};
  
void
handle_prediction (unsigned char *image, 
                   int width, 
                   int height,
                   unsigned int size,
                   PredictionBox *boxes,
                   int num_boxes);

#ifdef __cplusplus
}
#endif

#endif //__DETECTION_CUSTOM_LOGIC__
