gstinference_sources = [
	'gstbackend.cc',
	'gstchildinspector.c',
	'gstinferencebackends.cc',
	'gstinferencedebug.c',
	'gstinferenceclassification.c',
	'gstinferencemeta.c',
	'gstinferenceprediction.c',
	'gstinferencepostprocess.c',
	'gstinferencepreprocess.c',
	'gstncsdk.cc',
	'gsttensorflow.cc',
	'gsttflite.cc',
	'gstvideoinference.c'
]

gstinference_headers = [
	'gstbackend.h',
	'gstbackendsubclass.h',
	'gstchildinspector.h',
	'gstinferencebackends.h',
	'gstinferencedebug.h',
	'gstinferencemeta.h',
	'gstinferencepostprocess.h',
	'gstinferencepreprocess.h',
	'gstinferenceclassification.h',
	'gstinferenceprediction.h',
	'gstncsdk.h',
	'gsttensorflow.h',
	'gsttflite.h',
	'gstvideoinference.h'
]

r2inference_dep = dependency('r2inference-0.0', required : true)
if r2inference_dep.found()
  gstinference = library('gstinference-1.0',
    gstinference_sources,
    c_args : c_args,
    include_directories : [configinc, inference_inc_dir],
    version : api_version,
    install : true,
    dependencies : [gst_base_dep, gst_video_dep, r2inference_dep],
  )

  gstinference_dep = declare_dependency(link_with: gstinference,
    include_directories : [inference_inc_dir],
    dependencies : [gst_video_dep, r2inference_dep])

  install_headers(gstinference_headers, subdir : 'gstreamer-1.0/gst/r2inference')
  pkgconfig.generate(gstinference, install_dir : plugins_pkgconfig_install_dir)
else
  error('r2inference dependencies were not found.')
endif
