#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;
layout(binding = 0) uniform sampler2D texSampler;

layout(binding = 1) uniform MyUniforms {
    float time;
    vec3 color;
} ubo;

void main() {
    outColor = texture(texSampler, fragTexCoord);
    outColor = sin(outColor * (ubo.time * 10.0));
}
