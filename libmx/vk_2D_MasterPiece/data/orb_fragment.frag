#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D texSampler;

layout(push_constant) uniform PushConstants {
    float screenWidth;
    float screenHeight;
    float spritePosX;
    float spritePosY;
    float spriteSizeW;
    float spriteSizeH;
    float padding1;
    float padding2;
    float colorR;      // params[0] - red
    float colorG;      // params[1] - green
    float colorB;      // params[2] - blue
    float brightness;  // params[3] - per-orb brightness (0.0 to 1.0+)
} pc;

void main() {
    vec4 texColor = texture(texSampler, fragTexCoord);

    // Use star texture intensity (luminance) as the shape mask
    float texIntensity = max(texColor.r, max(texColor.g, texColor.b));
    
    // Per-orb brightness from push constant
    float orbBrightness = pc.brightness;
    
    // Base color from push constants
    vec3 orbColor = vec3(pc.colorR, pc.colorG, pc.colorB);
    
    // Use smoothstep to fade out dark pixels completely - no hard edge
    // This creates a smooth falloff from bright to transparent
    float alphaMask = smoothstep(0.1, 0.5, texIntensity);
    
    // If nearly invisible, discard to save fill rate
    if (alphaMask < 0.01) {
        discard;
    }
    
    // Create colored glow from the star shape
    vec3 glowColor = orbColor * texIntensity * orbBrightness * 1.5;
    
    // Add white core for brighter pixels
    float whiteMix = smoothstep(0.5, 1.0, texIntensity);
    glowColor = mix(glowColor, vec3(1.0) * orbBrightness, whiteMix * 0.5);
    
    // Alpha: use the mask and brightness, ensuring dark areas are fully transparent
    float alpha = alphaMask * orbBrightness;

    // Standard alpha blending output (not premultiplied)
    outColor = vec4(glowColor, alpha);
}
