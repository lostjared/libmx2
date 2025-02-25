#version 300 es
precision mediump float;
out vec4 color;
in vec3 localPos; 
uniform samplerCube cubemapTexture;
uniform float time_f;
uniform vec2 iResolution;

vec4 getCubemapColor(vec3 apos, samplerCube cube) {
    float r = length(apos.xy);
    float theta = atan(apos.y, apos.x);
    float spiralEffect = time_f * 0.2;
    r -= mod(spiralEffect, 4.0);
    theta += spiralEffect;
    vec2 distortedXY = vec2(cos(theta), sin(theta)) * r;
    vec3 newApos = normalize(vec3(distortedXY, apos.z));
    vec4 texColor = texture(cube, newApos);
    return vec4(texColor.rgb, 1.0);
}

void main(void) {
    vec3 tc = normalize(localPos);
    vec4 texColor = getCubemapColor(localPos, cubemapTexture);
    float sparkle = abs(sin(time_f * 10.0 + tc.x * 100.0) * cos(time_f * 15.0 + tc.y * 100.0));
    vec3 magicalColor = vec3(sin(time_f * 2.0) * 0.5 + 0.5, cos(time_f * 3.0) * 0.5 + 0.5, sin(time_f * 4.0) * 0.5 + 0.5);
    color = texColor + vec4(magicalColor * sparkle, 1.0);
}
