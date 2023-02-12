#version 450

in vec2 borderCoord;

out vec4 fragColor;

layout (location = 10) uniform vec4  color_rgba;
layout (location = 30) uniform float shift;



void main() {
  float r = 1.f - smoothstep(1.f, 1.f + shift, length(borderCoord));

  fragColor = color_rgba * r;
}
