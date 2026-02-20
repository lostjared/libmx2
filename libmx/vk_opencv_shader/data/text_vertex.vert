#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec4 fragColor;

layout(push_constant) uniform PushConstants {
    vec2 screenSize;
} pc;

void main() {
    vec2 ndc = inPosition / pc.screenSize * 2.0 - 1.0;
    gl_Position = vec4(ndc, 0.0, 1.0);
    fragTexCoord = inTexCoord;
    fragColor = vec4(1.0, 1.0, 1.0, 1.0);
}

