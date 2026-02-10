#version 450

layout(location = 0) in vec3 localPos;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform samplerCube cubemapTexture;
layout(binding = 1) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 params; // x=time, y=width, z=height
} ubo;

vec4 getCubemapColor(vec3 apos, samplerCube cube) {
    float r = length(apos.xy);
    float theta = atan(apos.y, apos.x);
    float spiralEffect = ubo.params.x * 0.2;
    r -= mod(spiralEffect, 4.0);
    theta += spiralEffect;
    vec2 distortedXY = vec2(cos(theta), sin(theta)) * r;
    vec3 newApos = normalize(vec3(distortedXY, apos.z));
    vec4 texColor = texture(cube, newApos);
    return vec4(texColor.rgb, 1.0);
}

void main() {
    vec3 tc = normalize(localPos);
    vec4 texColor = getCubemapColor(localPos, cubemapTexture);
    float sparkle = abs(sin(ubo.params.x * 10.0 + tc.x * 100.0) * cos(ubo.params.x * 15.0 + tc.y * 100.0));
    vec3 magicalColor = vec3(
        sin(ubo.params.x * 2.0) * 0.5 + 0.5,
        cos(ubo.params.x * 3.0) * 0.5 + 0.5,
        sin(ubo.params.x * 4.0) * 0.5 + 0.5
    );
    outColor = texColor + vec4(magicalColor * sparkle, 1.0);
}
