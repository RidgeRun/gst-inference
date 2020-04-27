gstinference_sources = [
  'gstplugin.cc',
  'gstclassificationoverlay.cc',
  'gstinferenceoverlay.cc',
  'gstdetectionoverlay.cc',
  'gstembeddingoverlay.cc'
]

opencv_all_sources = []
opencv_all_dependencies = [gst_base_dep, gst_video_dep, opencv_dep, gstopencv_dep]
if gstinference_dep.found()
  opencv_all_sources += gstinference_sources
  opencv_all_dependencies += gstinference_dep
else
  error('GstInference dependencie were not found.')
endif

if opencv_dep.found()
  gstopencv = library('gstinferenceoverlayplugin',
    opencv_all_sources,
    cpp_args : cpp_args,
    include_directories : [configinc, inference_inc_dir],
    dependencies : opencv_all_dependencies,
    install : true,
    install_dir : plugins_install_dir,
  )
else
  error('OpenCV dependencies were not found.')
endif
