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
    float colorR;      // params[0] - red tint
    float colorG;      // params[1] - green tint
    float colorB;      // params[2] - blue tint
    float brightness;  // params[3] - brightness multiplier
} pc;

void main() {
    vec4 texColor = texture(spriteTexture, fragTexCoord);
    
    // Apply color tint from push constants
    vec3 tintColor = vec3(pc.colorR, pc.colorG, pc.colorB);
    float bright = pc.brightness;
    
    // If all color params are zero, use default white
    if (tintColor.r + tintColor.g + tintColor.b < 0.01 && bright < 0.01) {
        outColor = texColor;
    } else {
        // Tint the texture with the orb color and apply brightness
        vec3 tintedColor = texColor.rgb * tintColor * bright;
        outColor = vec4(tintedColor, texColor.a);
    }
}
