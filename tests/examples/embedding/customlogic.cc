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
#include <stdlib.h>
#include <iostream>

void
handle_prediction (unsigned char *image,
    int width, int height, unsigned int size,
    double *embeddings, int num_dimensions,
    int verbose, char **embeddings_list, char **labels,
    int num_embeddings)
{
  /* FILLME:
   * Put here your custom logic, you may use C++ here.
   */

  int i, j;
  double current, diff;
  double likeness_thresh = 0.9;

  for (i = 0; i < num_embeddings; ++i) {
    diff = 0.0;
    for (j = 0; j < num_dimensions; ++j) {
      current = embeddings[j] -
          atof (embeddings_list[i * num_dimensions + j]);
      current = current * current;
      diff = diff + current;
    }

    if (diff < likeness_thresh) {
      std::cout << "Authorized: " << labels[i] << std::endl;
      break;
    }
  }

  if (verbose) {
    std::cout << "Displaying embeddings from current frame" << std::endl;

    for (int i = 0; i < num_dimensions; ++i) {
      std::cout << embeddings[i] << " ";
    }

    std::cout << std::endl;
  }
}
