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
    float params[4]; // params[0] is time_f
} pc;

// Helper to calculate distance-based wave
float ripple(vec2 pos, float time, float speed, float frequency, float aspect) {
    vec2 p = pos;
    p.x *= aspect;
    // Calculate distance from center (adjusted for aspect ratio)
    float d = distance(p, vec2(0.5 * aspect, 0.5));
    // Sine wave that decays as it moves outward
    return sin(d * frequency - time * speed) * exp(-d * 3.0);
}

void main() {
    vec2 uv = fragTexCoord;
    float aspect = pc.screenWidth / max(pc.screenHeight, 1.0);
    
    // Stabilize time to prevent jitter at high tick counts
    float t = pc.params[0];
    float time_t = mod(t, 10.0);
    
    // Calculate the displacement factor
    float x = ripple(uv, t, 0.5, 12.0, aspect);
    
    // Apply displacement relative to the center (0.5, 0.5)
    // This prevents the texture from sliding off toward the corner
    vec2 distortedUV = uv - 0.5;
    distortedUV *= (1.0 + sin(x * time_t) * 0.1); // 0.1 controls distortion strength
    distortedUV += 0.5;
    
    // Sample texture with wrapped coordinates to prevent edge clamping artifacts
    // Using a simple fract() for wrapping
    outColor = texture(samp, fract(distortedUV));
    outColor.rgb *= 0.3;
}
