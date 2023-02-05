version_data = configuration_data()
version_data.set('VERSION_STR', '"@0@"'.format(meson.project_version()))
configure_file(output: 'version.h', configuration: version_data)


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
  iconfigp_lib_dep,
  logging_lib_dep,
  gl_lib_dep,
  dependency('wayland-client'),
  dependency('wayland-egl'),
  dependency('egl'),
  dependency('libpng')
]

sources = [
  proto_source,
  shader_resource,

  'json/utils.cpp',

  'egl/context.cpp',

  'wayland/client.cpp',
  'wayland/output.cpp',

  'wm/unix-socket.cpp',
  'wm/i3ipc-socket.cpp',
  'wm/i3ipc.cpp',

  'config/config.cpp',
  'config/parse.cpp',

  'load-image.cpp',
  'texture-generator.cpp',
  'texture-generator-filter.cpp',
  'texture-provider.cpp',
  'layout-painter.cpp',

  'application.cpp',
  'application-args.cpp',
  'main.cpp',
]

executable(
  'wallpablur',
  sources,
  dependencies:        dependencies,
  install:             true,
  include_directories: ['../include']
)
