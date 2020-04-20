# Copy the file provided in input: to a file in the build directory with
# the name output: in the current directory.

types = configure_file(input : 'gst-inference-plugin.types',
  output : 'gst-inference-plugin.types',
  copy : true)

version_conf = configuration_data()
version_conf.set('PACKAGE_VERSION', meson.project_version())
version_conf.set('GST_API_VERSION', api_version)
version_conf.set('PACKAGE_NAME', 'GstInference')
version_conf.set('PACKAGE_URL', plugin_url)

# Compiles and installs gtkdoc documentation into prefix/share/gtk-doc/html.
gnome.gtkdoc('gst-inference-plugin-1.0',
  main_sgml : '@0@/gst-inference-plugin-docs.sgml'.format(meson.current_build_dir()),
  src_dir : ['@0@/ext/'.format(meson.source_root()), meson.current_build_dir()],
  gobject_typesfile : types,
  #content_files : [version_entities],
  dependencies : [plugin_deps],
  install : true)
