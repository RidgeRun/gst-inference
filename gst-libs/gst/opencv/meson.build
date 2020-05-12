opencv_sources = [
  'gstinferencebaseoverlay.cc'
]

opencv_headers = [
  'gstinferencebaseoverlay.h'
]

if opencv_dep.found()
  opencv_all_dependencies = [gst_base_dep, gst_video_dep, opencv_dep]

  inferencebaseoverlay = library('gstinferencebaseoverlay-@0@'.format(api_version),
    opencv_sources,
    c_args : c_args + ['-DBUILDING_GST_INFERENCE'],
    cpp_args: cpp_args + ['-DBUILDING_GST_INFERENCE'],
    include_directories : [configinc, inference_inc_dir],
    version : version_arr[0],
    install : true,
    dependencies : opencv_all_dependencies,
  )

  inferencebaseoverlay_dep = declare_dependency(link_with: inferencebaseoverlay,
    include_directories : [inference_inc_dir],
    dependencies : opencv_all_dependencies)

  install_headers(opencv_headers, subdir : 'gstreamer-1.0/gst/opencv')
else
  warning('OpenCV was not found in this system. Base overlay utils will not be built.')
endif
