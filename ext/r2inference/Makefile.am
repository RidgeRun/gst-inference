plugin_LTLIBRARIES = libgstinference.la

libgstinference_la_SOURCES =		\
	gstinceptionv1.c 		\
	gstinceptionv2.c		\
	gstinceptionv3.c 		\
	gstinceptionv4.c		\
	gstinference.c			\
	gsttinyyolov2.c			\
	gsttinyyolov3.c                 \
	gstfacenetv1.c                  \
	gstresnet50v1.c			\
	gstmobilenetv2.c    \
	gstmobilenetv2ssd.c


libgstinference_la_CFLAGS =		\
	$(GST_CFLAGS)			\
	$(GST_BASE_CFLAGS)		\
	$(GST_PLUGINS_BASE_CFLAGS)	\
	$(R2INFERENCE_CFLAGS)		\
	-I$(top_srcdir)/gst-libs

libgstinference_la_CXXFLAGS =		\
	$(GST_CFLAGS)			\
	$(GST_BASE_CFLAGS)		\
	$(GST_PLUGINS_BASE_CFLAGS)	\
	$(R2INFERENCE_CFLAGS)		\
	-I$(top_srcdir)/gst-libs

libgstinference_la_LIBADD =		\
	$(GST_LIBS)			\
	$(GST_BASE_LIBS)		\
	$(GST_PLUGINS_BASE_LIBS)	\
	$(R2INFERENCE_LIBS)		\
	$(top_builddir)/gst-libs/gst/r2inference/libgstinference-@GST_API_VERSION@.la

libgstinference_la_LDFLAGS =		\
	$(GST_PLUGIN_LDFLAGS)

libgstinference_la_LIBTOOLFLAGS =	\
	$(GST_PLUGIN_LIBTOOLFLAGS)

noinst_HEADERS =			\
	gstinceptionv1.h 		\
	gstinceptionv2.h		\
	gstinceptionv3.h 		\
	gstinceptionv4.h		\
	gsttinyyolov2.h			\
	gsttinyyolov3.h                 \
	gstfacenetv1.h                  \
	gstresnet50v1.h			\
	gstmobilenetv2.h    \
	gstmobilenetv2ssd.h
