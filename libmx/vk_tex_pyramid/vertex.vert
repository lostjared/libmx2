#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragPos;

layout(push_constant) uniform PushConstants {
    mat4 mvp;
} pushConstants;

void main() {
    gl_Position = pushConstants.mvp * vec4(inPos, 1.0);
    fragTexCoord = inTexCoord;
    fragNormal = inNormal;
    fragPos = inPos;
}
