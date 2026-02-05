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
    float param0;  // time_f
    float param1;
    float param2;
    float param3;
} pc;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(hash(i), hash(i + vec2(1.0, 0.0)), u.x),
        mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), u.x),
        u.y
    );
}

float fbm(vec2 p) {
    float v = 0.0;
    float a = 0.5;
    for (int i = 0; i < 4; i++) {
        v += a * noise(p);
        p *= 2.1;
        a *= 0.5;
    }
    return v;
}

void main() {
    vec2 iResolution = vec2(pc.screenWidth, pc.screenHeight);
    float time_f = pc.param0;
    
    vec2 uv = fragTexCoord;
    float aspect = iResolution.x / iResolution.y;
    vec2 p = (uv - 0.5) * vec2(aspect, 1.0);
    
    float t = time_f * 0.4;
    
    // Multi-layer fbm for warping
    float f1 = fbm(p * 1.5 + t * 0.2);
    float f2 = fbm(p.yx * 1.8 - t * 0.15);
    float f3 = fbm(p * 2.2 + vec2(f1, f2) * 0.5);
    
    // Create swirl with noise
    float dist = length(p);
    float angle = atan(p.y, p.x);
    
    // Controlled warp amount - scaled down to avoid extreme distortion
    float warpAmount = (f1 - 0.5) * 0.5 + (f2 - 0.5) * 0.3;
    angle += warpAmount + sin(t * 0.5 + f3 * 3.0) * 0.6;
    
    // Add pulsing
    dist *= 1.0 + (f3 - 0.5) * 0.2;
    
    // Reconstruct position
    vec2 warped = vec2(cos(angle), sin(angle)) * dist;
    
    // Add secondary flow - SMALL offsets only
    vec2 flow = vec2(
        fbm(warped * 2.0 + t * 0.1) - 0.5,
        fbm(warped.yx * 2.3 - t * 0.12) - 0.5
    ) * 0.15;  // Keep flow small
    
    // Map back to UV - scale to keep in reasonable range
    vec2 finalPos = warped + flow;
    vec2 texUV = finalPos * 0.6 / vec2(aspect, 1.0) + 0.5;
    
    // Chromatic aberration
    float aberration = 0.003 * (1.0 + length(flow) * 2.0);
    vec2 dir = normalize(p) * aberration;
    
    float r = texture(samp, texUV + dir).r;
    float g = texture(samp, texUV).g;
    float b = texture(samp, texUV - dir).b;
    vec3 col = vec3(r, g, b);
    
    // Color modulation based on noise
    float bright = 0.9 + 0.3 * f3 + 0.2 * sin(t * 0.6 + f1 * 4.0);
    col *= bright;
    
    // Saturation boost
    float gray = dot(col, vec3(0.299, 0.587, 0.114));
    float sat = 1.3 + 0.4 * sin(t * 0.4 + f2 * 5.0);
    col = mix(vec3(gray), col, sat);
    
    // Vignette
    vec2 vUV = uv * (1.0 - uv.yx);
    float vig = pow(vUV.x * vUV.y * 15.0, 0.2);
    col *= vig;
    
    outColor = vec4(col, 1.0);
}
