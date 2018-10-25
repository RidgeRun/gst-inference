
lib_LTLIBRARIES = libgstinference-@GST_API_VERSION@.la

libgstinference_@GST_API_VERSION@_la_SOURCES=	\
	gstvideoinference.cc

libgstinference_@GST_API_VERSION@_la_CXXFLAGS=	\
	$(GST_CXXFLAGS)				\
	$(GST_BASE_CFLAGS)			\
	$(GST_PLUGINS_BASE_CFLAGS)		\
	$(R2INFERENCE_CFLAGS)

libgstinference_@GST_API_VERSION@_la_CFLAGS=	\
	$(GST_CFLAGS)				\
	$(GST_BASE_CFLAGS)			\
	$(GST_PLUGINS_BASE_CFLAGS)		\
	-lgstvideo-@GST_API_VERSION@ 		\
	$(R2INFERENCE_CFLAGS)

libgstinference_@GST_API_VERSION@_la_LIBADD=	\
	$(GST_LIBS)				\
	$(GST_BASE_LIBS)			\
	$(GST_PLUGINS_BASE_LIBS)		\
	$(R2INFERENCE_LIBS)

gstinferenceincludedir=@includedir@/gstreamer-@GST_API_VERSION@/gst/r2inference/

gstinferenceinclude_HEADERS=	\
	gstvideoinference.h
