sources = [
  'src/serialize.cpp',
  'src/format.cpp'
]

dependencies = []



# find format library

code = '''
#include <format>
static_assert(__cpp_lib_format >= 201907L);
'''

compiler = meson.get_compiler('cpp')

if not compiler.compiles(code, args: ['-std=c++20'])
  dependencies += dependency('fmt')
  format_library = 'fmt::format'
else
  format_library = 'std::format'
endif
summary('iconfigp::format', format_library)





iconfigp_lib = static_library(
  'iconfigp',
  sources,
  dependencies:        dependencies,
  include_directories: ['include']
)

iconfigp_lib_dep = declare_dependency(
  link_with:           iconfigp_lib,
  dependencies:        dependencies,
  include_directories: ['include']
)
