gstinference_sources = [
  'gstplugin.cc',
  'gstclassificationoverlay.cc',
  'gstinferenceoverlay.cc',
  'gstdetectionoverlay.cc',
  'gstembeddingoverlay.cc'
]

opencv_all_sources = []
opencv_all_dependencies = [gst_base_dep, gst_video_dep, opencv_dep, gstopencv_dep, gstinference_dep]

if opencv_dep.found()
  gstinferenceoverlay = library('gstinferenceoverlayplugin',
    gstinference_sources,
    cpp_args : cpp_args,
    include_directories : [configinc, inference_inc_dir],
    dependencies : opencv_all_dependencies,
    install : true,
    install_dir : plugins_install_dir,
  )

  plugins += [gstinferenceoverlay]
else
  warning('OpenCV dependencies were not found for gstinferenceoverlayplugin.')
endif
