conf_data = configuration_data()
conf_data.set_quoted('PACKAGE', meson.project_name())
conf_data.set_quoted('VERSION', meson.project_version())
configure_file(output : 'config.h', configuration : conf_data)

recoverjpeg = executable('recoverjpeg',
  ['recoverjpeg.c', 'utils.c'],
  install : true)

executable('recovermov',
  ['recovermov.cpp', 'utils.c'],
  install : true)
