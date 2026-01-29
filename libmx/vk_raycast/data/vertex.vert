#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragWorldPos;

layout(binding = 1) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 params;
    vec4 color;
    vec4 playerPos;
    vec4 playerPlane;
} ubo;

void main() {    
    // Pass texture coords directly for raycasting (0-1 range)
    fragTexCoord = inTexCoord;
    fragColor = ubo.color.rgb;
    fragNormal = inNormal;
    fragWorldPos = inPosition;
    // Output position directly (full-screen quad in NDC)
    gl_Position = vec4(inPosition, 1.0);
}