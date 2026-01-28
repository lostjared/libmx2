#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragWorldPos;

// UBO with same structure as base class for descriptor set compatibility
layout(binding = 1) uniform UniformBufferObject {
    mat4 model;  // Not used - we use push constants
    mat4 view;
    mat4 proj;
    vec4 params;
    vec4 color;  // Not used - we use push constants
} ubo;

// Push constants for per-object model matrix and color
layout(push_constant) uniform PushConstants {
    mat4 model;
    vec4 color;
} pc;

void main() {    
    fragTexCoord = inTexCoord;
    fragColor = pc.color.rgb;
    fragNormal = mat3(pc.model) * inNormal;
    fragWorldPos = (pc.model * vec4(inPosition, 1.0)).xyz;
    gl_Position = ubo.proj * ubo.view * pc.model * vec4(inPosition, 1.0);
}
