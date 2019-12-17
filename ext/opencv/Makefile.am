plugin_LTLIBRARIES = libgstinferenceoverlayplugin.la

libgstinferenceoverlayplugin_la_SOURCES =	\
	gstclassificationoverlay.cc	\
	gstdetectionoverlay.cc		\
	gstinferenceoverlayplugin.c		\
	gstembeddingoverlay.cc

libgstinferenceoverlayplugin_la_CFLAGS =	\
	$(GST_CFLAGS)			\
	$(GST_BASE_CFLAGS)		\
	$(GST_PLUGINS_BASE_CFLAGS)	\
	$(GST_VIDEO_CFLAGS)		\
	$(R2INFERENCE_CFLAGS)		\
	-I$(top_srcdir)/gst-libs


libgstinferenceoverlayplugin_la_CXXFLAGS =	\
	$(GST_CXXFLAGS)			\
	$(GST_BASE_CFLAGS)		\
	$(GST_PLUGINS_BASE_CFLAGS)	\
	$(GST_VIDEO_CFLAGS)		\
	$(R2INFERENCE_CFLAGS)		\
	$(OPENCV_CFLAGS)		\
	-std=c++11


libgstinferenceoverlayplugin_la_LIBADD =	\
	$(GST_LIBS)			\
	$(GST_BASE_LIBS)		\
	$(GST_PLUGINS_BASE_LIBS)	\
	$(GST_VIDEO_LIBS)		\
	$(OPENCV_LIBS)			\
	$(R2INFERENCE_LIBS)		\
	$(top_builddir)/gst-libs/gst/r2inference/libgstinference-@GST_API_VERSION@.la \
	$(top_builddir)/gst-libs/gst/opencv/libgstinferenceoverlay-@GST_API_VERSION@.la

libgstinferenceoverlayplugin_la_LDFLAGS = 	\
	$(GST_PLUGIN_LDFLAGS)

libgstinferenceoverlayplugin_la_LIBTOOLFLAGS = \
	$(GST_PLUGIN_LIBTOOLFLAGS)

noinst_HEADERS =			\
	gstclassificationoverlay.h 	\
	gstdetectionoverlay.h		\
	gstembeddingoverlay.h
