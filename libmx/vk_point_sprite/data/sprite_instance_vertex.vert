#version 450

// Per-vertex (unit quad) - binding 0
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;

// Per-instance data - binding 1
layout(location = 2) in vec4 instancePosSize;   // x, y, w, h
layout(location = 3) in vec4 instanceParams;    // r, g, b, time

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec4 fragParams;
layout(location = 2) out vec2 fragSpriteSize;

layout(push_constant) uniform PushConstants {
    float screenWidth;
    float screenHeight;
} pc;

void main() {
    vec2 spritePos = instancePosSize.xy;
    vec2 spriteSize = instancePosSize.zw;

    vec2 screenPos = spritePos + inPosition * spriteSize;
    vec2 ndc = screenPos / vec2(pc.screenWidth, pc.screenHeight) * 2.0 - 1.0;
    gl_Position = vec4(ndc, 0.0, 1.0);
    fragTexCoord = inTexCoord;
    fragParams = instanceParams;
    fragSpriteSize = spriteSize;
}
