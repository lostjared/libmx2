#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;
layout(binding = 0) uniform sampler2D texSampler;
layout(binding = 1) uniform UniformBufferObject {
    vec2 iResolution;
    float time_f;
    float _pad0;
    vec4 iMouse;
} ubo;

void main() {
    vec4 tex = texture(texSampler, fragTexCoord);
    float t = ubo.time_f * 2.0;
    float hue = mod(t + fragTexCoord.x * 3.0 + fragTexCoord.y * 3.0, 6.28);
    float r = sin(hue) * 0.5 + 0.5;
    float g = sin(hue + 2.09) * 0.5 + 0.5;
    float b = sin(hue + 4.18) * 0.5 + 0.5;
    vec3 cycleColor = vec3(r, g, b);
    outColor.rgb = mix(tex.rgb, cycleColor, 0.7);
    outColor.a = 1.0;
}
