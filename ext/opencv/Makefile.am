plugin_LTLIBRARIES = libgstinferenceoverlay.la

libgstinferenceoverlay_la_SOURCES =		\
	gstclassificationoverlay.cc 			\
	gstinferenceoverlay.c

libgstinferenceoverlay_la_CFLAGS =		\
	$(GST_CFLAGS)			\
	$(GST_BASE_CFLAGS)		\
	$(GST_PLUGINS_BASE_CFLAGS)	\
	$(GST_VIDEO_CFLAGS)	\
	$(R2INFERENCE_CFLAGS)		\
	-I$(top_srcdir)/gst-libs


libgstinferenceoverlay_la_CXXFLAGS =		\
	$(GST_CXXFLAGS)			\
	$(GST_BASE_CFLAGS)		\
	$(GST_PLUGINS_BASE_CFLAGS)	\
	$(GST_VIDEO_CFLAGS)	\
	$(R2INFERENCE_CFLAGS)			\
	-std=c++11


libgstinferenceoverlay_la_LIBADD =		\
	$(GST_LIBS)			\
	$(GST_BASE_LIBS)		\
	$(GST_PLUGINS_BASE_LIBS)	\
	$(GST_VIDEO_LIBS)	\
	$(R2INFERENCE_LIBS)

libgstinferenceoverlay_la_LDFLAGS = 		\
	$(GST_PLUGIN_LDFLAGS)

libgstinferenceoverlay_la_LIBTOOLFLAGS = 	\
	$(GST_PLUGIN_LIBTOOLFLAGS)

noinst_HEADERS =			\
	gstclassificationoverlay.h
