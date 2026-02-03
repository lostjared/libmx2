#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;
layout(binding = 0) uniform sampler2D spriteTexture;

layout(push_constant) uniform PushConstants {
    vec2 screenSize;
    vec4 params;  // x=param1, y=param2, z=param3, w=param4
} pc;

void main() {
    vec4 texColor = texture(spriteTexture, fragTexCoord);
    
    // Default: pass through with alpha blending
    // You can modify the color based on pc.params for custom effects
    // Example effects you could add:
    // - params.x could be a tint intensity
    // - params.y could be a brightness multiplier
    // - params.z could be a saturation adjustment
    // - params.w could be time for animated effects
    
    outColor = texColor;
}
