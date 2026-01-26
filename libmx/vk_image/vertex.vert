#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

layout(binding = 1) uniform UniformBufferObject {
    float time;      
    vec3 tintColor;  
} ubo;

void main() {    
    fragTexCoord = inTexCoord;
    fragColor = ubo.tintColor;
    gl_Position = vec4(inPosition, 1.0);
}