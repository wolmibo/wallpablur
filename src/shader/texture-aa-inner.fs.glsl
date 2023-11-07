#version 450 core

in vec2 texCoord;
in vec2 genCoord;

out vec4 fragColor;

uniform sampler2D textureSampler;
uniform float     alpha;

layout (location = 4) uniform float cutoff;



void main() {
  fragColor = texture(textureSampler, texCoord) * alpha
              * (1.f - smoothstep(1.0f - cutoff, 1.0f + cutoff, length(genCoord)));
}
