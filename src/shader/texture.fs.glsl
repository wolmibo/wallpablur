#version 450 core

in vec2 texCoord;

out vec4 fragColor;

uniform sampler2D textureSampler;



vec4 noise(vec2 position) {
  return (fract(dot(position, vec2(3141.592, 2718.28))
    * vec4(420.420f, 47.47f, 69.69f, 42.42f)) - 0.5f) / 255.f;
}



void main() {
  fragColor = texture(textureSampler, texCoord) + noise(texCoord);
}
