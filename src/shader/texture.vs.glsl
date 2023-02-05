#version 450 core

layout (location = 0) in vec4 position;

out vec2 texCoord;



void main() {
  gl_Position = position;
  texCoord = (position.xy * vec2(1.f, -1.f) + vec2(1.f, 1.f)) / 2.f;
}
