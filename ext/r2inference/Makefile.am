plugin_LTLIBRARIES = libgstinference.la

libgstinference_la_SOURCES =		\
	gstgooglenet.c 			\
	gstinference.c			\
	gsttinyyolo.c

libgstinference_la_CFLAGS =		\
	$(GST_CFLAGS)			\
	$(GST_BASE_CFLAGS)		\
	$(GST_PLUGINS_BASE_CFLAGS)	\
	-I$(top_srcdir)/gst-libs

libgstinference_la_LIBADD =		\
	$(GST_LIBS)			\
	$(GST_BASE_LIBS)		\
	$(GST_PLUGINS_BASE_LIBS)	\
	$(top_builddir)/gst-libs/gst/r2inference/libgstinference-@GST_API_VERSION@.la

libgstinference_la_LDFLAGS = 		\
	$(GST_PLUGIN_LDFLAGS)

libgstinference_la_LIBTOOLFLAGS = 	\
	$(GST_PLUGIN_LIBTOOLFLAGS)

noinst_HEADERS =		\
	gstgooglenet.h		\
	gsttinyyolo.h
