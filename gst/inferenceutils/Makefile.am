plugin_LTLIBRARIES = libgstinferenceutils.la

libgstinferenceutils_la_SOURCES =      \
        gstinferenceutils.c            \
        gstinferencefilter.c           \
        gstinferencedebug.c            \
        gstinferencecrop.cc            \
        cropelement.cc                 \
        videocrop.cc                   \
        gstinferencebin.c

libgstinferenceutils_la_CFLAGS =       \
        $(GST_CFLAGS)                  \
        $(GST_BASE_CFLAGS)             \
        $(GST_PLUGINS_BASE_CFLAGS)     \
        -I$(top_srcdir)/gst-libs

libgstinferenceutils_la_CXXFLAGS =     \
        $(GST_CFLAGS)                  \
        $(GST_BASE_CFLAGS)             \
        $(GST_PLUGINS_BASE_CFLAGS)     \
        -I$(top_srcdir)/gst-libs       \
        -std=c++11

libgstinferenceutils_la_LIBADD =       \
        $(GST_LIBS)                    \
        $(GST_BASE_LIBS)               \
        $(GST_PLUGINS_BASE_LIBS)       \
        $(top_builddir)/gst-libs/gst/r2inference/libgstinference-@GST_API_VERSION@.la

libgstinferenceutils_la_LDFLAGS =      \
        $(GST_PLUGIN_LDFLAGS)

libgstinferenceutils_la_LIBTOOLFLAGS = \
        $(GST_PLUGIN_LIBTOOLFLAGS)

noinst_HEADERS =                       \
        gstinferencefilter.h           \
        gstinferencedebug.h            \
        gstinferencecrop.h             \
        cropelement.h                  \
        videocrop.h                    \
        gstinferencebin.h
