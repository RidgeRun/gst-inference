/* GstEmbedding
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
