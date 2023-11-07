shader_files = [
  ['solid_color_vs',       'solid-color.vs.glsl'],
  ['solid_color_fs',       'solid-color.fs.glsl'],

  ['texture_vs',           'texture.vs.glsl'],
  ['texture_fs',           'texture.fs.glsl'],
  ['texture_aa_outer_fs',  'texture-aa-outer.fs.glsl'],

  ['rescale_texture_vs',   'rescale-texture.vs.glsl'],
  ['rescale_texture_fs',   'rescale-texture.fs.glsl'],

  ['filter_vs',            'filter.vs.glsl'],
  ['filter_invert_fs',     'filter-invert.fs.glsl'],
  ['filter_line_blur_fs',  'filter-line-blur.fs.glsl'],

  ['border_vs',            'border.vs.glsl'],
  ['border_linear_fs',     'border-linear.fs.glsl'],
  ['border_sinusoidal_fs', 'border-sinusoidal.fs.glsl']
]



shader_header = 'shader.hpp'
shader_source = 'shader.cpp'

shader_input = []
shader_name  = []

foreach shader: shader_files
  shader_name  += shader[0]
  shader_input += shader[1]
endforeach



resource_compiler = executable('static-resources',
  'static-resources.cpp', dependencies: logcerr_dep)

shader_resource = custom_target(
  'shader-resource',
  output: [shader_header, shader_source],
  input:  shader_input,
  command: [resource_compiler, '@OUTPUT@', shader_name, '@INPUT@']
)
