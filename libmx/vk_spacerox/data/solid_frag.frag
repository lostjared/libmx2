#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;
layout(binding = 0) uniform sampler2D spriteTexture;

layout(push_constant) uniform PushConstants {
    float screenWidth;
    float screenHeight;
    float spritePosX;
    float spritePosY;
    float spriteSizeW;
    float spriteSizeH;
    float padding1;
    float padding2;
    float colorR;
    float colorG;
    float colorB;
    float colorA;
} pc;

void main() {
    outColor = vec4(pc.colorR, pc.colorG, pc.colorB, pc.colorA);
}
