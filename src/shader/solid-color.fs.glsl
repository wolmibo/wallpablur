#version 450 core

out vec4 fragColor;

uniform vec4 color_rgba;



void main() {
  fragColor = color_rgba;
}
