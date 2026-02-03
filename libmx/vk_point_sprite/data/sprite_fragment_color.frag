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
    float time;        // params[4] - time for pulsing effect
    float pulseSpeed;  // params[5] - pulse speed multiplier
} pc;

void main() {
    vec4 texColor = texture(spriteTexture, fragTexCoord);

    // Discard dark/background pixels from the sprite
    float texLuminance = dot(texColor.rgb, vec3(0.299, 0.587, 0.114));
    if (texLuminance < 0.3 || texColor.a < 0.5)
        discard;

    // Calculate distance from center for light falloff
    vec2 center = vec2(0.5, 0.5);
    float dist = distance(fragTexCoord, center);
    
    // Create a soft radial falloff for light effect
    float falloff = 1.0 - smoothstep(0.0, 0.5, dist);
    float glow = exp(-dist * 4.0); // Exponential glow for more natural light
    
    // Combine falloff and glow for final intensity with boost
    float intensity = mix(falloff, glow, 0.6) * 1.5;
    
    // Discard pixels outside the light radius
    if (intensity < 0.01)
        discard;

    vec3 tintColor = vec3(pc.colorR, pc.colorG, pc.colorB);
    float bright = pc.brightness;
    
    // Add pulsing effect based on time
    float pulse = 0.5 + 0.5 * sin(pc.time * pc.pulseSpeed);
    
    if (tintColor.r + tintColor.g + tintColor.b < 0.01 && bright < 0.01) {
        // Default white light if no color specified
        vec3 lightColor = vec3(1.0, 1.0, 1.0) * intensity * pulse * 2.0;
        outColor = vec4(lightColor, intensity * pulse);
    } else {
        // Apply tint color with brightness and intensity falloff
        vec3 lightColor = tintColor * bright * intensity * pulse * 1.5;
        // Add a bright core
        float core = exp(-dist * 8.0);
        lightColor += vec3(1.0) * core * bright * 1.0 * pulse;
        outColor = vec4(lightColor, intensity * pulse);
    }
}
