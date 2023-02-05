#version 450 core

layout (location = 0) in vec4 position;

layout (location = 0) uniform mat4 transform;



void main() {
  gl_Position = transform * position;
}
