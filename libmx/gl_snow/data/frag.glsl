#version 300 es
precision highp float;
in vec2 TexCoord;
out vec4 color;
uniform sampler2D textTexture; 
uniform float time_f;
uniform vec2 iResolution;

void main() {
    vec2 uv = TexCoord;
    float blockSize = 0.1 + 0.05 * sin(time_f * 2.0);
    float blockY = floor(uv.y / blockSize) * blockSize;
    vec2 resizedUV = vec2(uv.x, blockY + mod(uv.y, blockSize) * (0.5 + 0.5 * sin(time_f + blockY * 10.0)));
    vec4 texColor = texture(textTexture, resizedUV);
    color = texColor;
}
