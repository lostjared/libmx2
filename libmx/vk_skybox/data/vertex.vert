#version 450

layout(binding = 1) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 params; // x=time, y=width, z=height
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 localPos;

void main() {
    localPos = inPosition;
    vec4 pos = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    gl_Position = pos.xyww; // Force depth to maximum (skybox always behind)
}
