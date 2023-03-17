#version 450

in float falloff;

out vec4 fragColor;

layout (location = 10) uniform vec4  color_rgba;
layout (location = 20) uniform float exponent;



void main() {
  float v = 0.5f * cos((1 - falloff) * 3.141592653) + 0.5f;
  fragColor = color_rgba * pow(v, exponent);
}
