#version 330 core

in vec2 TexCoord;
out vec4 color;
uniform sampler2D textTexture;
uniform float time_f;
uniform float alpha;
uniform vec2 iResolution;

void main(void) {
    vec2 tc = TexCoord;
    vec2 normCoord = gl_FragCoord.xy / iResolution.xy;
    vec2 centeredCoord = normCoord - vec2(0.5, 0.5);
    centeredCoord.x *= iResolution.x / iResolution.y;
    float r = length(centeredCoord);
    float theta = atan(centeredCoord.y, centeredCoord.x);
    float twistAmount = 15.0;
    theta += (1.0 - r) * twistAmount * sin(time_f);
    vec2 twistedCoord = vec2(cos(theta), sin(theta)) * r;
    twistedCoord.x *= iResolution.y / iResolution.x;
    twistedCoord += vec2(0.5, 0.5);
    color = texture(textTexture, twistedCoord);
}