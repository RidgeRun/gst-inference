plugin_LTLIBRARIES = libgstinferencecrop.la

libgstinferencecrop_la_SOURCES =	\
	gstinferencecrop.cc		\
	cropelement.cc			\
	videocrop.cc

libgstinferencecrop_la_CFLAGS =	\
	$(GST_CFLAGS)			\
	$(GST_BASE_CFLAGS)		\
	$(GST_PLUGINS_BASE_CFLAGS)	\
	-I$(top_srcdir)/gst-libs

libgstinferencecrop_la_CXXFLAGS =	\
	$(GST_CFLAGS)			\
	$(GST_BASE_CFLAGS)		\
	$(GST_PLUGINS_BASE_CFLAGS)	\
	-I$(top_srcdir)/gst-libs	\
	-std=c++11

libgstinferencecrop_la_LIBADD =	\
	$(GST_LIBS)			\
	$(GST_BASE_LIBS)		\
	$(GST_PLUGINS_BASE_LIBS)	\
	$(top_builddir)/gst-libs/gst/r2inference/libgstinference-@GST_API_VERSION@.la

libgstinferencecrop_la_LDFLAGS =	\
	$(GST_PLUGIN_LDFLAGS)

libgstinferencecrop_la_LIBTOOLFLAGS =	\
	$(GST_PLUGIN_LIBTOOLFLAGS)

noinst_HEADERS =			\
	gstinferencecrop.h		\
	cropelement.h			\
	videocrop.h
