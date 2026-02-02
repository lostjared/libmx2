#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in float inSize;
layout(location = 2) in vec4 inColor;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out float twinkleFactor;

layout(binding = 1) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 params;  // params.x = time (seconds) for example
    vec4 color;
} ubo;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    gl_PointSize = inSize;
    fragColor = inColor;

    float t = ubo.params.x;
    twinkleFactor = 0.5 + 0.5 * sin(t + inPosition.x * 12.9898 + inPosition.y * 78.233 + inPosition.z * 37.719);
}
