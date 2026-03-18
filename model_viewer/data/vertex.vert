#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;

layout(binding = 1) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 params;
    vec4 color;
} ubo;

void main() {
    vec4 viewPos4 = ubo.view * ubo.model * vec4(inPosition, 1.0);
    fragPos = viewPos4.xyz;
    fragNormal = mat3(transpose(inverse(ubo.view * ubo.model))) * inNormal;
    fragTexCoord = inTexCoord;
    gl_Position = ubo.proj * viewPos4;
}