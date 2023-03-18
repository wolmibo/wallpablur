#version 450

in float falloff;

out vec4 fragColor;



layout (location = 10) uniform vec4  color_rgba;

layout (location = 20) uniform float exponent;
layout (location = 21) uniform float falloff_scale;



void main() {
  fragColor = color_rgba * pow(clamp(falloff * falloff_scale, 0, 1), exponent);
}
