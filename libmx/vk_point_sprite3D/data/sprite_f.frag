#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;
layout(binding = 0) uniform sampler2D spriteTexture;

layout(push_constant) uniform PushConstants {
    vec2 screenSize;
    vec2 spritePos;
    vec2 spriteSize;
    vec4 params;  // x=waveSpeed, y=distortion, z=brightness, w=time
} pc;

void main() {
    float time = pc.params.w;
    
    // 1. Calculate the center of this specific sprite in screen space
    vec2 center = pc.spritePos + (pc.spriteSize * 0.5);
    
    // 2. Create a global wave effect based on screen position
    // This makes a pulse move from the top-left to the bottom-right
    float wave = sin((center.x + center.y) * 0.005 - time * pc.params.x);
    wave = pow(max(0.0, wave), 5.0); // Sharpen the wave into a "beam"
    
    // 3. Coordinate Distortion (pulsing the texture zoom)
    vec2 uv = fragTexCoord;
    if(wave > 0.1) {
        uv = (uv - 0.5) * (1.0 - wave * 0.2 * pc.params.y) + 0.5;
    }

    vec4 texColor = texture(spriteTexture, uv);
    
    // 4. Color manipulation
    // The "wave" adds a bright highlight and increases saturation
    vec3 highlight = vec3(0.2, 0.6, 1.0) * wave; 
    vec3 baseColor = texColor.rgb * pc.params.z;
    
    // Add a subtle "flicker" to the wave
    float flicker = sin(time * 50.0 + center.x) * 0.1 + 0.9;
    
    vec3 finalRGB = baseColor + (highlight * flicker);
    
    // 5. Output with the wave influencing transparency
    outColor = vec4(finalRGB, texColor.a);
}
