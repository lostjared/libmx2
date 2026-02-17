#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

// Output matches what external fragment shaders expect
layout(location = 0) out vec2 tc;

// Set 1 so it doesn't conflict with the fragment shader's set 0 bindings
layout(set = 1, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 params;
    vec4 color;
} ubo;

void main() {
    tc = inTexCoord;
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
}
