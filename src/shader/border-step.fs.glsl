#version 450

in vec2 borderCoord;

out vec4 fragColor;

layout (location = 10) uniform vec4 color_rgba;



void main() {
  float r = 1.f - step(1.f, length(borderCoord));

  fragColor = color_rgba * r;
}
