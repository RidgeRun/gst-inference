gstinference_sources = [
	'gstfacenetv1.c',
	'gstinceptionv1.c',
	'gstinceptionv2.c',
	'gstinceptionv3.c',
	'gstinceptionv4.c',
	'gstinference.c',
	'gstmobilenetv2.c',
	'gstresnet50v1.c',
	'gsttinyyolov2.c',
	'gsttinyyolov3.c'
]

gstinference_headers = [
	'gstfacenetv1.h',
	'gstinceptionv1.h',
	'gstinceptionv2.h',
	'gstinceptionv3.h',
	'gstinceptionv4.h',
	'gstmobilenetv2.h',
	'gstresnet50v1.h',
	'gsttinyyolov2.h',
	'gsttinyyolov3.h'
]

gstinference = library('gstinference',
  gstinference_sources,
  c_args : c_args,
  include_directories : [configinc, inference_inc_dir],
  dependencies : [gst_base_dep, gst_video_dep, gstinference_dep],
  install : true,
  install_dir : plugins_install_dir,
)
#pkgconfig.generate(gstinference, install_dir : plugins_pkgconfig_install_dir)
plugins += [gstinference]
