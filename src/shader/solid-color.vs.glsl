#version 450 core

layout (location = 0) in vec4 position;

layout (location = 0) uniform mat4 transform;

out vec2 genCoord;



void main() {
  gl_Position = transform * position;
  genCoord    = 0.5f * (position.xy + vec2(1.f, 1.f));
}
