#version 450

in vec2 borderCoord;

out vec4 fragColor;

layout (location = 10) uniform vec4  color_rgba;
layout (location = 20) uniform float exponent;



void main() {
  float r = clamp(length(borderCoord), 0.f, 1.f);
  r = 0.5f * cos(r * 3.141592653f) + 0.5f;

  fragColor = color_rgba * pow(r, exponent);
}
