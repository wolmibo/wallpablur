sources = [
  'src/framebuffer.cpp',
  'src/plane.cpp',
  'src/program.cpp',
  'src/texture.cpp'
]

dependencies = [
  dependency('gl'),
  dependency('epoxy')
]




gl_lib = static_library(
  'gl_lib',
  sources,
  dependencies:        dependencies,
  include_directories: ['include'],
)

gl_lib_dep = declare_dependency(
  link_with:           gl_lib,
  dependencies:        dependencies,
  include_directories: ['include']
)
