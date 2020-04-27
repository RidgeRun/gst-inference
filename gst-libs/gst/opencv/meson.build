opencv_sources = [
  'gstinferencebaseoverlay.cc'
]

opencv_headers = [
  'gstinferencebaseoverlay.h'
]

if opencv_dep.found()
	opencv_all_sources = opencv_sources
	opencv_all_headers = opencv_headers
	opencv_all_dependencies = [gst_base_dep, gst_video_dep, opencv_dep]
	if r2inference_dep.found()
	  opencv_all_dependencies += r2inference_dep
  else
    error('R2Inference dependency not found.')
  endif

  gstopencv = library('gstinferencebaseoverlay-@0@'.format(api_version),
    opencv_all_sources,
    c_args : c_args + ['-DBUILDING_GST_INFERENCE'],
    cpp_args: cpp_args + ['-DBUILDING_GST_INFERENCE'],
    include_directories : [configinc, inference_inc_dir],
    version : api_version,
    install : true,
    dependencies : opencv_all_dependencies,
  )

  gstopencv_dep = declare_dependency(link_with: gstopencv,
    include_directories : [inference_inc_dir],
    dependencies : opencv_all_dependencies)

  install_headers(opencv_all_headers, subdir : 'gstreamer-1.0/gst/opencv')
else
  warning('OpenCV dependencies were not found for gstinferencebaseoverlay.')
endif
