project(
  'wallpablur',
  ['c', 'cpp'],
  version: '0.2.1',
  license: 'MIT',
  default_options: ['warning_level=3']
)



# Manually set C++ version until meson supports 'cpp_std=c++2b'
add_project_arguments('-std=c++2b', language: 'cpp')

utils_dep = declare_dependency(include_directories: ['extern/utils/include'])

logcerr_dep = dependency(
  'logcerr',
  fallback: ['logcerr', 'logcerr_dep'],
  default_options: ['default_library=static', 'install_as_subproject=false']
)

iconfigp_dep = dependency(
  'iconfigp',
  version: '>=0.1.3',
  fallback: ['iconfigp', 'iconfigp_dep'],
  default_options: [
    'default_library=static',
    'install_as_subproject=false',
    'tests=false'
  ]
)

subdir('extern/gl')

subdir('src')
