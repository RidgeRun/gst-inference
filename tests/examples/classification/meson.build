# Compile examples
example_source = [
  'customlogic.cc',
  'gstclassification.c'
]

executable('classification', example_source,
  include_directories: [configinc, inference_inc_dir],
  dependencies : [example_deps, gstinference_dep],
  install: false)
