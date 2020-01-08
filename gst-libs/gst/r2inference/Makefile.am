
lib_LTLIBRARIES = libgstinference-@GST_API_VERSION@.la

libgstinference_@GST_API_VERSION@_la_SOURCES=	\
	gstvideoinference.c			\
	gstchildinspector.c			\
	gstinferencemeta.c			\
	gstinferencebackends.cc			\
	gstbackend.cc				\
	gstncsdk.cc				\
	gsttensorflow.cc			\
	gstinferencepreprocess.c		\
	gstinferencepostprocess.c		\
	gstinferencedebug.c			\
	gstinferenceprediction.c		\
	gstinferenceclassification.c

libgstinference_@GST_API_VERSION@_la_CXXFLAGS=	\
	$(GST_CXXFLAGS)				\
	$(GST_BASE_CFLAGS)			\
	$(GST_PLUGINS_BASE_CFLAGS)		\
	$(R2INFERENCE_CFLAGS)			\
	-std=c++11

libgstinference_@GST_API_VERSION@_la_CFLAGS=	\
	$(GST_CFLAGS)				\
	$(GST_BASE_CFLAGS)			\
	$(GST_PLUGINS_BASE_CFLAGS)		\
	$(R2INFERENCE_CFLAGS)

libgstinference_@GST_API_VERSION@_la_LIBADD=	\
	$(GST_LIBS)				\
	$(GST_BASE_LIBS)			\
	-lgstvideo-@GST_API_VERSION@		\
	$(GST_PLUGINS_BASE_LIBS)		\
	$(R2INFERENCE_LIBS)

gstinferenceincludedir=@includedir@/gstreamer-@GST_API_VERSION@/gst/r2inference/

gstinferenceinclude_HEADERS=		\
	gstvideoinference.h		\
	gstchildinspector.h		\
	gstinferencebackends.h		\
	gstbackend.h			\
	gstbackendsubclass.h		\
	gstncsdk.h			\
	gsttensorflow.h			\
	gstinferencepreprocess.h	\
	gstinferencepostprocess.h	\
	gstinferencedebug.h		\
	gstinferencemeta.h		\
	gstinferenceprediction.h	\
	gstinferenceclassification.h
