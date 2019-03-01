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

#ifndef __CLASSIFICATION_CUSTOM_LOGIC__
#define __CLASSIFICATION_CUSTOM_LOGIC__

#ifdef __cplusplus
extern "C" {
#endif

void
handle_prediction (unsigned char *image, 
                   int width, 
                   int height,
                   unsigned int size,
                   double *probabilities,
                   int num_probabilities);

#ifdef __cplusplus
}
#endif

#endif //__CLASSIFICATION_CUSTOM_LOGIC__
