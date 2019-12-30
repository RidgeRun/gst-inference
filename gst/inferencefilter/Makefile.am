plugin_LTLIBRARIES = libgstinferencefilter.la

libgstinferencefilter_la_SOURCES =	\
	gstinferencefilter.cc

libgstinferencefilter_la_CFLAGS =	\
	$(GST_CFLAGS)			\
	$(GST_BASE_CFLAGS)		\
	$(GST_PLUGINS_BASE_CFLAGS)	\
	-I$(top_srcdir)/gst-libs        \
	$(R2INFERENCE_CFLAGS)

libgstinferencefilter_la_CXXFLAGS =	\
	$(GST_CFLAGS)			\
	$(GST_BASE_CFLAGS)		\
	$(GST_PLUGINS_BASE_CFLAGS)	\
	-I$(top_srcdir)/gst-libs	\
	-std=c++11                      \
	$(R2INFERENCE_CFLAGS)

libgstinferencefilter_la_LIBADD =	\
	$(GST_LIBS)			\
	$(GST_BASE_LIBS)		\
	$(GST_PLUGINS_BASE_LIBS)	\
	$(R2INFERENCE_LIBS) \
	$(top_builddir)/gst-libs/gst/r2inference/libgstinference-@GST_API_VERSION@.la

libgstinferencefilter_la_LDFLAGS =	\
$(GST_PLUGIN_LDFLAGS)

libgstinferencefilter_la_LIBTOOLFLAGS =	\
	$(GST_PLUGIN_LIBTOOLFLAGS)

noinst_HEADERS =			\
	gstinferencefilter.h
