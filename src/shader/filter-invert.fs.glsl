#version 450

in vec2 texCoord;

out vec4 fragColor;

uniform sampler2D textureSampler;



void main() {
  vec4 orig = texture(textureSampler, texCoord);
  fragColor = vec4(vec3(1.f, 1.f, 1.f) - orig.rgb, orig.a);
}
