inferenceutils_sources = [
	'cropelement.cc',
    'gstinferencebin.c',
	'gstinferencecrop.cc',
	'gstinferencedebug.c',
	'gstinferencefilter.c',
	'videocrop.cc',
	'gstinferenceutils.c'
]

inferenceutils_headers = [
	'cropelement.h',
	'gstinferencebin.h',
	'gstinferencecrop.h',
	'gstinferencedebug.h',
	'gstinferencefilter.h',
	'videocrop.h',
]

gstinferenceutils = library('gstinferenceutils',
  inferenceutils_sources,
  c_args : c_args,
  cpp_args : cpp_args,
  include_directories : [configinc, inference_inc_dir],
  dependencies : [gst_base_dep, gst_video_dep, gstinference_dep],
  install : true,
  install_dir : plugins_install_dir,
)
plugins += [gstinferenceutils]
