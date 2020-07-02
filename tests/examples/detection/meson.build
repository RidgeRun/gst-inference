# Compile examples
example_source = [
  'customlogic.cc',
  'gstdetection.c'
]

executable('detection', example_source,
  include_directories: [configinc, inference_inc_dir],
  dependencies : [example_deps, gstinference_dep],
  install: false)
