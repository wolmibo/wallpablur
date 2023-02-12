#version 450

in vec2 borderCoord;

out vec4 fragColor;

layout (location = 10) uniform vec4  color_rgba;
layout (location = 20) uniform float exponent;



void main() {
  float r = 1.f - length(borderCoord);

  fragColor = color_rgba * pow(r, exponent);
}
