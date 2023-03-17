#version 450

layout (location = 0) in vec4 position;

out float falloff;

layout (location = 0) uniform mat4 transform;



void main() {
  gl_Position = transform * position;
  falloff     = position.z;
}
