#version 450

layout(location = 0) in vec2 inPosition;  // Must match your C++ Vertex struct
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;

layout(push_constant) uniform PushConstants {
    float screenWidth;
    float screenHeight;
    float spritePosX;
    float spritePosY;
    float spriteSizeW;
    float spriteSizeH;
    float padding1; 
    float padding2;
    float params[4];
} pc;

void main() {
    vec2 screenPos = vec2(pc.spritePosX, pc.spritePosY) + inPosition * vec2(pc.spriteSizeW, pc.spriteSizeH);
    vec2 ndc = (screenPos / vec2(pc.screenWidth, pc.screenHeight)) * 2.0 - 1.0;
    
    gl_Position = vec4(ndc, 0.0, 1.0);
    fragTexCoord = inTexCoord;
}