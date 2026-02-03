#version 450

layout(location = 0) in vec2 inPosition;  // Unit quad (0-1)
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;

layout(push_constant) uniform PushConstants {
    vec2 screenSize;
    vec2 spritePos;    // x, y position in screen coords
    vec2 spriteSize;   // width, height in screen coords
    vec4 params;       // Custom shader parameters
} pc;

void main() {
    // Transform unit quad to sprite screen position and size
    vec2 screenPos = pc.spritePos + inPosition * pc.spriteSize;
    
    // Convert screen coordinates to NDC
    vec2 ndc = screenPos / pc.screenSize * 2.0 - 1.0;
    gl_Position = vec4(ndc, 0.0, 1.0);
    fragTexCoord = inTexCoord;
}
