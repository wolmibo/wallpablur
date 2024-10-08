# Wayland stuff

wlscanner = dependency('wayland-scanner', native: true)
scanner = find_program(wlscanner.get_variable('wayland_scanner'))

scanner_src = generator(
  scanner,
  output: '@BASENAME@-protocol.c',
  arguments: ['private-code', '@INPUT@', '@OUTPUT@'],
)

scanner_header = generator(
  scanner,
  output: '@BASENAME@-client-protocol.h',
  arguments: ['client-header', '@INPUT@', '@OUTPUT@'],
)

wlprotocols = dependency('wayland-protocols')
protocols = [
  wlprotocols.get_variable('pkgdatadir') / 'stable/viewporter/viewporter.xml',

  wlprotocols.get_variable('pkgdatadir') / 'stable/xdg-shell/xdg-shell.xml',
  '../protocols/wlr-layer-shell-unstable-v1.xml'
]

proto_source = []

foreach proto: protocols
  proto_source += scanner_src.process(proto)
  proto_source += scanner_header.process(proto)
endforeach

subdir('shader')

dependencies = [
  logcerr_dep,
  iconfigp_dep,
  utils_dep,

  gl_lib_dep,
  dependency('wayland-client'),
  dependency('wayland-egl'),
  dependency('egl'),
]

sources = [
  proto_source,
  shader_resource,

  'json/utils.cpp',

  'egl/context.cpp',

  'gl/quad.cpp',
  'gl/sector.cpp',
  'gl/utils.cpp',

  'wayland/client.cpp',
  'wayland/output.cpp',
  'wayland/surface.cpp',

  'wm/unix-socket.cpp',
  'wm/i3ipc-socket.cpp',
  'wm/i3ipc.cpp',

  'config/config.cpp',
  'config/panel.cpp',

  'rectangle.cpp',

  'surface-expression.cpp',
  'workspace-expression.cpp',
  'surface-workspace-expression.cpp',

  'expression/string-compare.cpp',
  'expression/tokenizer.cpp',

  'texture-generator.cpp',
  'texture-provider.cpp',
  'layout-painter.cpp',

  'output.cpp',
  'workspace.cpp',

  'exception.cpp',
  'application.cpp',
  'application-args.cpp',
  'main.cpp',

  'version.cpp'
]



build_options = ''

gdkpixbuf = dependency(
  'gdk-pixbuf-2.0',
  required: get_option('gdk-pixbuf'),
  version:  '>=2.32'
)

if gdkpixbuf.found()
  sources       += 'load-image-gdk-pixbuf.cpp'
  dependencies  += gdkpixbuf
  build_options += 'gdk-pixbuf=enabled, '
else
  sources       += 'load-image-png.cpp'
  dependencies  += dependency('libpng')
  build_options += 'gdk-pixbuf=disabled, '
endif

cpp_args = []

if get_option('debug')
  cpp_args += '-DLOGCERR_ENABLE_DEBUGGING'
  build_options += 'debug=enabled'
else
  cpp_args += '-DLOGCERR_DISABLE_DEBUGGING'
  build_options += 'debug=disabled'
endif





build_info = configuration_data()
build_info.set('BUILD_OPTIONS', '"@0@"'.format(build_options))
build_info.set('VERSION_STR', '"@0@"'.format(meson.project_version()))

configure_file(output: 'buildinfo.h', configuration: build_info)

vcs_tag(input: 'versioninfo.h.in', output: 'versioninfo.h')


executable(
  'wallpablur',
  sources,
  dependencies:        dependencies,
  cpp_args:            cpp_args,
  install:             true,
  include_directories: ['../include']
)
