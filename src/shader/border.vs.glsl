#version 450

layout (location = 0) in vec4 position;

out vec2 borderCoord;

layout (location = 0) uniform mat4 transform;
layout (location = 1) uniform vec2 direction;



void main() {
  gl_Position = transform * position;
  borderCoord = (position.xy * direction + abs(direction)) / 2.f;
}
