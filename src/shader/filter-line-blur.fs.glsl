#version 450

in vec2 texCoord;

out vec4 fragColor;

uniform sampler2D textureSampler;
uniform int       samples;
uniform vec2      direction;



vec4 noise(vec2 position) {
  return (fract(dot(position, vec2(2718.28, 3141.592))
    * vec4(420.420f, 47.47f, 69.69f, 42.42f)) - 0.5f) / 255.f;
}



void main() {
  vec4 average = vec4(0.f, 0.f, 0.f, 0.f);

  for (int i = -samples; i <= samples; ++i) {
    average += texture(textureSampler, texCoord + i * direction);
  }

  average /= 2 * samples + 1;


  fragColor = noise(texCoord) + average;
}
