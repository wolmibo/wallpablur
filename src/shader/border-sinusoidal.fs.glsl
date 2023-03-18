#version 450

in float falloff;

out vec4 fragColor;



layout (location = 10) uniform vec4  color_rgba;

layout (location = 20) uniform float exponent;
layout (location = 21) uniform float falloff_scale;



void main() {
  float v = 0.5f * cos((1 - clamp(falloff * falloff_scale, 0, 1)) * 3.141592653) + 0.5f;
  fragColor = color_rgba * pow(v, exponent);
}
