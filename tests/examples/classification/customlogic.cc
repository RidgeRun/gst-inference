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
                   int width, 
                   int height,
                   int size,
                   double *probabilities,
                   int num_probabilities)
{
  /* FILLME:
   * Put here your custom logic, you may use C++ here.
   */

  double max = 0;
  double imax = 0;
  
  for (int i = 0; i < num_probabilities; ++i) {
    double current = probabilities[i];

    if (current > max) {
      max = current;
      imax = i;
    }
  }

  std::cout << "Highest probability is label " << imax
	    << ": (" << max << ")" << std::endl;
}
