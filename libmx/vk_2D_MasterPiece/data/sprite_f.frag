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
    float param0;  // waveSpeed
    float param1;  // distortion
    float param2;  // brightness
    float param3;  // time
} pc;

void main() {
    float time = pc.param3;
    
    // 1. Calculate the center of this specific sprite in screen space
    vec2 center = vec2(pc.spritePosX, pc.spritePosY) + (vec2(pc.spriteSizeW, pc.spriteSizeH) * 0.5);
    
    // 2. Create a global wave effect based on screen position
    // This makes a pulse move from the top-left to the bottom-right
    float wave = sin((center.x + center.y) * 0.005 - time * pc.param0);
    wave = pow(max(0.0, wave), 5.0); // Sharpen the wave into a "beam"
    
    // 3. Coordinate Distortion (pulsing the texture zoom)
    vec2 uv = fragTexCoord;
    if(wave > 0.1) {
        uv = (uv - 0.5) * (1.0 - wave * 0.2 * pc.param1) + 0.5;
    }

    vec4 texColor = texture(spriteTexture, uv);
    
    // 4. Color manipulation
    // The "wave" adds a bright highlight and increases saturation
    vec3 highlight = vec3(0.2, 0.6, 1.0) * wave; 
    vec3 baseColor = texColor.rgb * pc.param2;
    
    // Add a subtle "flicker" to the wave
    float flicker = sin(time * 50.0 + center.x) * 0.1 + 0.9;
    
    vec3 finalRGB = baseColor + (highlight * flicker);
    
    // 5. Output with the wave influencing transparency
    outColor = vec4(finalRGB, texColor.a);
}
