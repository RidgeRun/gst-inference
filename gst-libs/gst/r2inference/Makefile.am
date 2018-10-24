
lib_LTLIBRARIES = libgstinference-@GST_API_VERSION@.la

libgstinference_@GST_API_VERSION@_la_SOURCES=	\
	gstvideoinference.cc

libgstinference_@GST_API_VERSION@_la_CXXFLAGS=	\
	$(GST_CXXFLAGS)				\
	$(R2INFERENCE_CFLAGS)

libgstinference_@GST_API_VERSION@_la_CFLAGS=	\
	$(GST_CFLAGS)				\
	$(R2INFERENCE_CFLAGS)

libgstinference_@GST_API_VERSION@_la_LIBADD=	\
	$(GST_LIBS)				\
	$(R2INFERENCE_LIBS)

gstinferenceincludedir=@includedir@/gstreamer-@GST_API_VERSION@/gst/r2inference/

gstinferenceinclude_HEADERS=	\
	gstvideoinference.h
