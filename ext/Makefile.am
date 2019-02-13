if USE_R2INFERENCE
R2INFERENCE_DIR=r2inference
else
R2INFERENCE_DIR=
endif

SUBDIRS=$(R2INFERENCE_DIR)
SUBDIRS+= opencv

DIST_SUBDIRS=r2inference
DIST_SUBDIRS+= opencv

include $(top_srcdir)/common/parallel-subdirs.mak
