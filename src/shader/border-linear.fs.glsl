#version 450

in float falloff;

out vec4 fragColor;

layout (location = 10) uniform vec4  color_rgba;
layout (location = 20) uniform float exponent;



void main() {
  fragColor = color_rgba * pow(falloff, exponent);
}
