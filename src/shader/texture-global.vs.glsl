#version 450 core

layout (location = 0) in vec4 position;

layout (location = 0) uniform mat4 transform;

out vec2 texCoord;



void main() {
  gl_Position = transform * position;
  texCoord = (gl_Position.xy + vec2(1.f, 1.f)) / 2.f;
}
