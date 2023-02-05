#version 450 core

layout (location = 0) in vec4 position;

out vec2 texCoord;

layout (location = 0) uniform mat2 transform;



void main() {
  gl_Position = position;
  texCoord = (transform * position.xy + vec2(1.f, 1.f)) / 2.f;
}
