plugin_LTLIBRARIES = libgstinference.la

libgstinference_la_SOURCES =		\
	gstinceptionv2.c		\
	gstinceptionv4.c		\
	gstinference.c			\
	gsttinyyolov2.c			\
	gstfacenetv1.c

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
	gstinceptionv2.h		\
	gstinceptionv4.h		\
	gsttinyyolov2.h			\
	gstfacenetv1.h
