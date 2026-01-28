#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 params;
    vec4 color;
} ubo;

void main() {
    gl_Position = ubo.proj * ubo.view * vec4(inPosition, 1.0);
    gl_PointSize = 8.0;
    fragColor = inColor;
}
