plugin_LTLIBRARIES = libgstinferencedebug.la

libgstinferencedebug_la_SOURCES =	\
	gstinferencedebug.c

libgstinferencedebug_la_CFLAGS =	\
	$(GST_CFLAGS)			\
	$(GST_BASE_CFLAGS)		\
	$(GST_PLUGINS_BASE_CFLAGS)	\
	-I$(top_srcdir)/gst-libs        \
	$(R2INFERENCE_CFLAGS)

libgstinferencedebug_la_LIBADD =	\
	$(GST_LIBS)			\
	$(GST_BASE_LIBS)		\
	$(GST_PLUGINS_BASE_LIBS)	\
	$(R2INFERENCE_LIBS) \
	$(top_builddir)/gst-libs/gst/r2inference/libgstinference-@GST_API_VERSION@.la

libgstinferencedebug_la_LDFLAGS =	\
$(GST_PLUGIN_LDFLAGS)

libgstinferencedebug_la_LIBTOOLFLAGS =	\
	$(GST_PLUGIN_LIBTOOLFLAGS)

noinst_HEADERS =			\
	gstinferencedebug.h
