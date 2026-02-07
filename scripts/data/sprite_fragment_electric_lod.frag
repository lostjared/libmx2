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
    vec3 a = vec3(0.5);
    vec3 b = vec3(0.5);
    vec3 c = vec3(1.0);
    vec3 d = vec3(0.263, 0.416, 0.557);
    return a + b * cos(6.28318 * (c * t + d));
}

// Use explicit LOD to avoid derivative issues
vec4 sampleTex(vec2 uv) {
    uv = clamp(uv, 0.001, 0.999);
    return textureLod(samp, uv, 0.0);
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

float fbm(vec2 p, bool ridges) {
    float v = 0.0;
    float a = 0.5;
    for (int i = 0; i < 3; i++) {
        float n = noise(p);
        if (ridges) n = 1.0 - abs(n * 2.0 - 1.0);
        v += a * n;
        p *= 2.0;
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
    
    float t = time_f * 0.3;
    
    // Domain warping
    vec2 q = vec2(
        fbm(p * 1.8 + t * 0.1, false),
        fbm(p * 1.8 + vec2(5.2, 1.3) - t * 0.1, false)
    );
    
    vec2 r = vec2(
        fbm(p * 1.5 + q * 2.5 + vec2(t * 0.2, 9.2), true),
        fbm(p * 1.5 + q * 2.5 + vec2(8.3, 2.8), false)
    );
    
    // Gentle warp
    p += (r - 0.5) * 0.25;
    
    // Smooth kaleidoscope
    float slices = 6.0;
    float pi = 3.14159265359;
    float dist = length(p);
    float ang = atan(p.y, p.x);
    
    float sectorSize = pi * 2.0 / slices;
    float sector = ang / sectorSize;
    sector = sector - floor(sector);
    float mirror = abs(sector * 2.0 - 1.0);
    ang = mirror * sectorSize;
    
    vec2 kaleidoPos = vec2(cos(ang), sin(ang)) * dist;
    
    // Map to UV - keep in valid range
    vec2 texUV = kaleidoPos * 0.5 / vec2(aspect, 1.0) + 0.5;
    texUV = clamp(texUV, 0.001, 0.999);
    
    // Sample texture with explicit LOD
    vec3 texCol = sampleTex(texUV).rgb;
    
    // Electric effects
    float electric = pow(length(r - 0.5) * 2.5, 2.0) * 0.7;
    vec3 pal = palette(dist * 2.0 + length(q) * 2.0 + t);
    
    // Mix
    vec3 col = mix(texCol, pal, 0.35);
    col += pal * electric * 0.5;
    
    // Vignette
    vec2 vUV = uv * (1.0 - uv.yx);
    float vig = pow(vUV.x * vUV.y * 15.0, 0.25);
    col *= vig;
    
    outColor = vec4(col, 1.0);
}
