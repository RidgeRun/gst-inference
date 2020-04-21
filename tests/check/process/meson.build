# name, condition when to skip the test, extra dependencies and extra files
gst_tests = [
  ['test_gst_fill_classification_meta_function', false, [gstinference_dep, test_deps],  [] ],
  ['test_gst_normalize_function', false, [gstinference_dep, test_deps],  [] ],
  ['test_gst_pixel_to_float_function', false, [gstinference_dep, test_deps],  [] ],
  ['test_gst_subtract_mean_function', false, [gstinference_dep, test_deps],  [] ],
]

# Add C Definitions for tests
test_defines = [
  '-DTESTFILE="' + meson.current_source_dir() + '/meson.build"',
]

# Define plugins path
plugins_dir = gst_dep.get_pkgconfig_variable('pluginsdir')

# Create environment object to  stores information about the environment
# variables set during tests.
# Define constant enviroment variable
env = environment()
env.set('GST_PLUGIN_SYSTEM_PATH_1_0', '')
env.set('CK_DEFAULT_TIMEOUT', '120')

# Build and run tests
foreach t : gst_tests
  fname = '@0@.c'.format(t.get(0))
  test_name = t.get(0).underscorify()
  extra_sources = t.get(3, [ ])
  extra_deps = t.get(2, [ ])
  skip_test = t.get(1, false)
  if not skip_test
    exe = executable(test_name, fname, extra_sources,
      include_directories : [configinc],
      c_args : ['-DHAVE_CONFIG_H=1' ] + test_defines,
      dependencies : test_deps + extra_deps,
    )

    # Define enviroment variable
    env.set('GST_REGISTRY', '@0@/@1@.registry'.format(meson.current_build_dir(), test_name))

    # Run tests
    test(test_name, exe, env: env, timeout : 60)
  endif
endforeach
