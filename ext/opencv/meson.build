gstinference_sources = [
  'gstplugin.cc',
  'gstinferenceoverlay.cc',
]

if opencv_dep.found()
  opencv_all_dependencies = [gst_base_dep, gst_video_dep, opencv_dep, inferencebaseoverlay_dep, gstinference_dep]

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
  warning('OpenCV not found in this system. Overlay plug-ins will not be built.')
endif
