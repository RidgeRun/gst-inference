gstinference_sources = [
  'gstclassificationoverlay.cc',
  'gstinferenceoverlay.cc',
  'gstdetectionoverlay.cc',
  'gstembeddingoverlay.cc'
]

libopencv_headers = [
  'opencv2/bgsegm.hpp',
  'opencv2/calib3d.hpp',
  'opencv2/core.hpp',
  'opencv2/imgproc.hpp',
  'opencv2/objdetect.hpp',
  'opencv2/opencv.hpp',
  'opencv2/video.hpp',
]

libopencv4_headers = [
  'opencv4/opencv2/calib3d.hpp',
  'opencv4/opencv2/core.hpp',
  'opencv4/opencv2/imgproc.hpp',
  'opencv4/opencv2/objdetect.hpp',
  'opencv4/opencv2/opencv.hpp',
  'opencv4/opencv2/video.hpp',
]

gstopencv_cargs = ['-DGST_HAAR_CASCADES_DIR="@0@"']

opencv_dep = dependency('opencv', version : ['>= 3.0.0', '< 3.5.0'], required : false)
opencv_found = opencv_dep.found()

if opencv_found
  foreach h : libopencv_headers
    if not cxx.has_header(h)
      message('Needed header "' + h + '" not found')
      opencv_found = false
    endif
  endforeach
else
  opencv_dep = dependency('opencv4', version : ['>= 4.0.0', '< 4.3.0'], required : false)
  opencv_found = opencv_dep.found()
  if opencv_found
    foreach h : libopencv4_headers
      if not cxx.has_header(h)
        message('Needed header "' + h + '" not found')
        opencv_found = false
      endif
    endforeach
  endif
endif

if opencv_found
  opencv_prefix = opencv_dep.get_pkgconfig_variable('prefix')
  gstopencv_cargs += ['-DOPENCV_PREFIX="' + opencv_prefix + '"']

  # Check the data dir used by opencv for its xml data files
  # Use prefix from pkg-config to be compatible with cross-compilation
  r = run_command('test', '-d', opencv_prefix + '/share/opencv')
  if r.returncode() == 0
    gstopencv_cargs += '-DOPENCV_PATH_NAME="opencv"'
  else
    r = run_command('test', '-d', opencv_prefix + '/share/OpenCV')
    if r.returncode() == 0
      gstopencv_cargs += '-DOPENCV_PATH_NAME="OpenCV"'
    else
      r = run_command('test', '-d', opencv_prefix + '/share/opencv4')
      if r.returncode() == 0
        gstopencv_cargs += '-DOPENCV_PATH_NAME="opencv4"'
      else
        message('Unable to detect OpenCV data directory')
        opencv_found = false
      endif
    endif
  endif
endif

gst_opencv_all_sources = []
gst_opencv_all_dependencies = [gst_base_dep, gst_video_dep, opencv_dep, gstopencv_dep]
if gstinference_dep.found()
  gst_opencv_all_sources += gstinference_sources
  gst_opencv_all_dependencies += gstinference_dep
endif



if opencv_found
  gstopencv = library('gstinferenceoverlayplugin',
    gst_opencv_all_sources,
    c_args : c_args,
    cpp_args : cpp_args,
    include_directories : [configinc, inference_inc_dir],
    dependencies : gst_opencv_all_dependencies,
    install : true,
    install_dir : plugins_install_dir,
  )
else
  error('OpenCV  dependencies were not found.')
endif
