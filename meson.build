project('GStreamer Inference', ['c', 'cpp'], default_options : ['cpp_std=c++11'],
  version : '0.12.0.1',
  meson_version : '>= 0.50',)

project_name = meson.project_name()
# Create an empty configuration object to set config.h information
cdata = configuration_data()
gst_inference_version = meson.project_version()
version_arr = gst_inference_version.split('.')
gst_inference_version_major = version_arr[0].to_int()
gst_inference_version_minor = version_arr[1].to_int()
gst_inference_version_micro = version_arr[2].to_int()
if version_arr.length() == 4
  gst_inference_version_nano = version_arr[3].to_int()
else
  gst_inference_version_nano = 0
endif
gst_inference_version_is_dev = gst_inference_version_minor % 2 == 1 and gst_inference_version_micro < 90
# Define gstreamer API version
api_version = '1.0'

# Required versions
gst_req = '>= 1.8.0.1'
r2i_req = '>= 0.12.0'

# Find external dependencies
gst_dep         = dependency('gstreamer-1.0',      version : gst_req)
gst_base_dep    = dependency('gstreamer-base-1.0', version : gst_req)
gst_check_dep   = dependency('gstreamer-check-1.0',version : gst_req)
gst_video_dep   = dependency('gstreamer-video-1.0', version : gst_req)
r2inference_dep = dependency('r2inference-0.0', version : r2i_req, required : true)

opencv_dep = dependency('opencv', version : ['>= 2.3.1', '< 2.4.13'], required : false)
if opencv_dep.found()
  cdata.set('OCV_VERSION_LT_3_2', 1)
endif

if not opencv_dep.found()
  opencv_dep = dependency('opencv', version : ['>= 3.2.0', '< 3.5.0'], required : false)

  if not opencv_dep.found()
    opencv_dep = dependency('opencv4', version : ['>= 4.0.0', '< 4.3.0'], required : false)
    if opencv_dep.found()
        cdata.set('OCV_VERSION_4_0', 1)
    endif
  endif
endif

## Dependencies
# Define plugin dependencies
plugin_deps = [gst_dep, gst_base_dep]
# Examples dependencies
example_deps = [gst_dep, gst_base_dep]
# Define test dependencies
test_deps=[gst_dep, gst_base_dep, gst_video_dep, gst_check_dep]

# Verify if profiling was enabled
if get_option('enable-profiling').enabled()
  profiler_dep = dependency('libprofiler')
  if(profiler_dep.found())
    message('Profiling enabled: Building examples with profiling support.')
    link_flags += ['-lprofiler']
    c_flags += ['-DPROFILING']
    # Update test test_deps to include profiler dependency
    test_deps += [profiler_dep]
  else
    message('MESON_FAIL gperftools profiling library not found.')
  endif
endif

# Define header directories
inference_inc_dir = include_directories('gst-libs/')
configinc = include_directories('.')

# Define compiler args and include directories
c_args = ['-DHAVE_CONFIG_H']
cpp_args = ['-DHAVE_CONFIG_H']

# Define installation directories
plugins = []
lib_install_dir = get_option('libdir')
plugins_install_dir = join_paths(get_option('libdir'), 'gstreamer-1.0')
plugins_pkgconfig_install_dir = join_paths(get_option('libdir'), 'pkgconfig')

# Internal installation directory
plugin_dir = []

# Get an object returns describing a compiler
cc = meson.get_compiler('c')
cxx = meson.get_compiler('cpp')

# Verify if the warning flags are available in the compiler
# If the flags is availale for the compiler it wiil be used in all compiler
# invocations with the exception of compile tests.
warning_flags = [
  '-Werror',
  '-Wmissing-declarations',
  '-Wmissing-prototypes',
  '-Wredundant-decls',
  '-Wundef',
  '-Wwrite-strings',
  '-Wformat',
  '-Wformat-nonliteral',
  '-Wformat-security',
  '-Wold-style-definition',
  '-Winit-self',
  '-Wmissing-include-dirs',
  '-Waddress',
  '-Waggregate-return',
  '-Wno-multichar',
  '-Wdeclaration-after-statement',
  '-Wvla',
  '-Wpointer-arith',
]
foreach extra_arg : warning_flags
  if cc.has_argument (extra_arg)
    # Add flag to the compiler command line
    add_project_arguments([extra_arg], language: 'c')
  endif
endforeach

plugin_url='http://developer.ridgerun.com/wiki/index.php?title=GstInference'
# Set config.h information
cdata.set_quoted('GST_API_VERSION', api_version)
cdata.set_quoted('LOCALEDIR', join_paths(get_option('prefix'), get_option('localedir')))
cdata.set_quoted('LIBDIR', join_paths(get_option('prefix'), get_option('libdir')))
cdata.set_quoted('GST_API_VERSION', '1.0')
cdata.set_quoted('GST_LICENSE', 'LGPL')
cdata.set_quoted('PACKAGE', 'gst-inference')
cdata.set_quoted('PACKAGE_NAME', 'GstInference')
cdata.set_quoted('PACKAGE_STRING', 'GstInference @0@'.format(gst_inference_version))
cdata.set_quoted('PACKAGE_TARNAME', 'gst-inference')
cdata.set_quoted('PACKAGE_BUGREPORT', 'https://github.com/RidgeRun/gst-inference')
cdata.set_quoted('PACKAGE_URL', plugin_url)
cdata.set_quoted('PACKAGE_VERSION', gst_inference_version)
cdata.set_quoted('PLUGINDIR', plugin_url)
cdata.set_quoted('VERSION', gst_inference_version)

if gst_inference_version_nano > 0
    # Have GST_ERROR message printed when running from git
    cdata.set('GST_LEVEL_DEFAULT', 'GST_LEVEL_ERROR')
else
    cdata.set('GST_LEVEL_DEFAULT', 'GST_LEVEL_NONE')
endif

# GStreamer package name and origin url
gst_package_name = get_option('package-name')
if gst_package_name == ''
  if gst_inference_version_nano == 0
    gst_package_name = '@0@ source release'.format(project_name)
  elif gst_inference_version_nano == 1
    gst_package_name = 'GStreamer git'
  else
    gst_package_name = 'GStreamer prerelease'
  endif
endif
cdata.set_quoted('GST_PACKAGE_NAME', gst_package_name)
cdata.set_quoted('GST_PACKAGE_ORIGIN', get_option('package-origin'))

# These are only needed/used by the ABI tests
host_defines = [
  [ 'x86', 'HAVE_CPU_I386' ],
  [ 'x86_64', 'HAVE_CPU_X86_64' ],
  [ 'arm', 'HAVE_CPU_ARM' ],
  [ 'aarch64', 'HAVE_CPU_AARCH64' ],
  [ 'mips', 'HAVE_CPU_MIPS' ],
  [ 'powerpc', 'HAVE_CPU_PPC' ],
  [ 'powerpc64', 'HAVE_CPU_PPC64' ],
  [ 'alpha', 'HAVE_CPU_ALPHA' ],
  [ 'sparc', 'HAVE_CPU_SPARC' ],
  [ 'ia64', 'HAVE_CPU_IA64' ],
  [ 'hppa', 'HAVE_CPU_HPPA' ],
  [ 'm68k', 'HAVE_CPU_M68K' ],
  [ 's390', 'HAVE_CPU_S390' ],
]
foreach h : host_defines
  if h.get(0) == host_machine.cpu()
    cdata.set(h.get(1), 1)
  else
    cdata.set(h.get(1), false)
  endif
endforeach

cdata.set_quoted('HOST_CPU', host_machine.cpu())

# Verify if the specified header exists
check_headers = [
  'dlfcn.h',
  'inttypes.h',
  'memory.h',
  'poll.h',
  'stdint.h',
  'stdlib.h',
  'stdio_ext.h',
  'strings.h',
  'string.h',
  'sys/param.h',
  'sys/poll.h',
  'sys/prctl.h',
  'sys/socket.h',
  'sys/stat.h',
  'sys/times.h',
  'sys/time.h',
  'sys/types.h',
  'sys/utsname.h',
  'sys/wait.h',
  'ucontext.h',
  'unistd.h',
  'valgrind/valgrind.h',
  'sys/resource.h',
]

foreach h : check_headers
  if cc.has_header(h)
    define = 'HAVE_' + h.underscorify().to_upper()
    cdata.set(define, 1)
  endif
endforeach

# Gtk documentation
gnome = import('gnome')

# Imports pkgconfig module
pkgconfig = import('pkgconfig')

# Install git hooks
python3 = import('python').find_installation()
run_command(python3, '-c', 'import shutil; shutil.copy("hooks/pre-commit.hook", ".git/hooks/pre-commit")')

# Meson will generate a header file all the entries in the configuration data object
configure_file(output : 'config.h', configuration : cdata)

# Enter to each subdirectory and execute the meson.build
subdir('gst-libs')
subdir('gst')
subdir('ext')
subdir('tests')
subdir('docs')
