# Compile examples
example_source = [
  'customlogic.cc',
  'gstembedding.c'
]

executable('embedding', example_source,
  include_directories: [configinc, inference_inc_dir],
  dependencies : [example_deps],
  install: false)
