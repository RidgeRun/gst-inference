opencv_sources = [
  'gstinferencebaseoverlay.cc'
]

opencv_headers = [
  'gstinferencebaseoverlay.h'
]

gstinference_headers = [
  'gstinferenceoverlay.h'
]

opencv_dep = dependency('opencv', version : '>= 3.0.0', required : false)
r2inference_dep = dependency('r2inference-0.0', required : true)

if not opencv_dep.found()
  opencv_dep = dependency('opencv4', version : '>= 4.0.0', required : false)
endif
if opencv_dep.found()
	opencv_all_sources = opencv_sources
	opencv_all_headers = opencv_headers
	opencv_all_dependencies = [gst_base_dep, gst_video_dep, opencv_dep]
	if r2inference_dep.found()
	  opencv_all_dependencies += r2inference_dep
	endif
  gstopencv = library('gstinferencebaseoverlay-@0@'.format(api_version),
    opencv_all_sources,
    c_args : c_args + ['-DBUILDING_GST_OPENCV']+ ['-DBUILDING_GST_INFERENCE'],
    cpp_args: cpp_args + ['-DBUILDING_GST_OPENCV']+ ['-DBUILDING_GST_INFERENCE'],
    include_directories : [configinc, inference_inc_dir],
    version : '0.0.0', # TODO verify this version number
    install : true,
    dependencies : opencv_all_dependencies,
  )

  gstopencv_dep = declare_dependency(link_with: gstopencv,
    include_directories : [inference_inc_dir],
    dependencies : opencv_all_dependencies)

  install_headers(opencv_all_headers, subdir : 'gstreamer-1.0/gst/opencv')
else
  error('OpenCV  dependencies were not found.')
endif
