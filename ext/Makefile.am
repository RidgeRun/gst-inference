if USE_R2INFERENCE
R2INFERENCE_DIR=r2inference
else
R2INFERENCE_DIR=
endif

if USE_OPENCV
OPENCV_DIR = opencv
else
OPENCV_DIR =
endif

SUBDIRS = \
  $(R2INFERENCE_DIR) \
  $(OPENCV_DIR)

DIST_SUBDIRS = \
  r2inference \
  opencv

include $(top_srcdir)/common/parallel-subdirs.mak
