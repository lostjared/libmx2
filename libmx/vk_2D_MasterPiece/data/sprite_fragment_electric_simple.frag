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

vec3 palette(float t) {
    vec3 a = vec3(0.5, 0.5, 0.5);
    vec3 b = vec3(0.5, 0.5, 0.5);
    vec3 c = vec3(1.0, 1.0, 1.0);
    vec3 d = vec3(0.263, 0.416, 0.557);
    return a + b * cos(6.28318 * (c * t + d));
}

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

void main() {
    vec2 iResolution = vec2(pc.screenWidth, pc.screenHeight);
    float time_f = pc.param0;
    
    vec2 uv = fragTexCoord;
    float aspect = iResolution.x / iResolution.y;
    vec2 centered = (uv - 0.5) * vec2(aspect, 1.0);
    
    float t = time_f * 0.3;
    
    // Noise-based domain warping (gentle)
    float n1 = noise(centered * 2.0 + t * 0.1);
    float n2 = noise(centered * 2.5 + vec2(5.2, 1.3) - t * 0.12);
    
    vec2 warp = vec2(n1 - 0.5, n2 - 0.5) * 0.25;
    vec2 pos = centered + warp;
    
    // Simple kaleidoscope effect
    float slices = 8.0;
    float pi = 3.14159265359;
    float r = length(pos);
    float ang = atan(pos.y, pos.x);
    float sector = pi * 2.0 / slices;
    ang = mod(ang, sector);
    ang = abs(ang - sector * 0.5);
    
    vec2 kaleidoPos = vec2(cos(ang), sin(ang)) * r;
    
    // Add gentle flow
    float flowNoise1 = noise(kaleidoPos * 1.8 + t * 0.15);
    float flowNoise2 = noise(kaleidoPos.yx * 2.2 - t * 0.18);
    
    vec2 flow = vec2(flowNoise1 - 0.5, flowNoise2 - 0.5) * 0.15;
    vec2 finalPos = kaleidoPos + flow;
    
    // Map to texture UV (keep reasonable range)
    vec2 texUV = finalPos * 0.8 / vec2(aspect, 1.0) + 0.5;
    
    // Sample texture
    vec3 texCol = texture(samp, texUV).rgb;
    
    // Electric palette
    float palInput = length(finalPos) * 2.0 + length(warp) * 3.0 + t;
    vec3 pal = palette(palInput);
    
    // Mix texture with electric colors
    vec3 col = mix(texCol, pal, 0.4);
    
    // Add electric glow
    float glow = pow(length(warp) * 2.0, 2.0) * 0.8;
    col += pal * glow;
    
    // Vignette
    vec2 vUV = uv * (1.0 - uv.yx);
    float vig = pow(vUV.x * vUV.y * 15.0, 0.2);
    col *= vig;
    
    outColor = vec4(col, 1.0);
}
