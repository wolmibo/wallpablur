compiler = meson.get_compiler('cpp')

dependencies = []




# find a supported format library

code = '''
#include <format>
static_assert(__cpp_lib_format >= 201907L);
'''

if not compiler.compiles(code, args: ['-std=c++20'])
  dependencies += dependency('fmt')
  format_library = 'fmt::format'
else
  format_library = 'std::format'
endif
summary('logging::format', format_library)




sources = [
  'src/core.cpp',
  'src/format.cpp'
]

include_directories = ['include']

logging_lib = static_library(
  'logging-lib',
  sources,
  dependencies:        dependencies,
  include_directories: include_directories
)

logging_lib_dep = declare_dependency(
  link_with:           logging_lib,
  dependencies:        dependencies,
  include_directories: include_directories
)
