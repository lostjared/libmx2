#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D samp;

layout(push_constant) uniform PushConstants {
    float screenWidth;
    float screenHeight;
    float spritePosX;
    float spritePosY;
    float spriteSizeW;
    float spriteSizeH;
    float padding1;
    float padding2;
    float param0;
    float param1;
    float param2;
    float param3;
} pc;

void main() {
    // Just pass through the texture directly - no effects
    vec4 col = texture(samp, fragTexCoord);
    outColor = col;
}
