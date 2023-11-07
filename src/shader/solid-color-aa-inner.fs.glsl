#version 450 core

in vec2 genCoord;

out vec4 fragColor;

uniform vec4 color_rgba;

layout (location = 4) uniform float cutoff;



void main() {
  fragColor = color_rgba
              * (1.f - smoothstep(1.0f - cutoff, 1.0f + cutoff, length(genCoord)));
}
