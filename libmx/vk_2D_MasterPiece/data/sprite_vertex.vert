#version 450

layout(location = 0) in vec2 inPosition;  // Typically a unit quad: (0,0) to (1,1)
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
    // 1. Calculate pixel position on screen
    vec2 screenPos = vec2(pc.spritePosX, pc.spritePosY) + (inPosition * vec2(pc.spriteSizeW, pc.spriteSizeH));
    
    // 2. Convert to Normalized Device Coordinates (0 to 1 range -> -1 to 1 range)
    // screenPos / screenSize gives us 0.0 to 1.0
    // Multiplying by 2.0 and subtracting 1.0 gives us -1.0 to 1.0
    vec2 ndc = (screenPos / vec2(pc.screenWidth, pc.screenHeight)) * 2.0 - 1.0;

    // 3. VULKAN Y-FLIP: 
    // In Vulkan, NDC Y is positive DOWN. 
    // If your math assumes (0,0) is top-left, we don't need to flip ndc.y 
    // unless your coordinate system is inverted. 
    gl_Position = vec4(ndc, 0.0, 1.0);
    
    fragTexCoord = inTexCoord;
}