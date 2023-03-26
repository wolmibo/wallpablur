project(
  'wallpablur',
  ['c', 'cpp'],
  version: '0.2.1',
  license: 'MIT',
  default_options: ['warning_level=3']
)



# Manually set C++ version until meson supports 'cpp_std=c++2b'
add_project_arguments('-std=c++2b', language: 'cpp')

logcerr_dep = dependency('logcerr', fallback: ['logcerr', 'logcerr_dep'])


subdir('extern/iconfigp')
subdir('extern/gl')

subdir('src')
