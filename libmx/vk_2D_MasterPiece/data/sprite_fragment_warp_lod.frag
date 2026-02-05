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

// Use explicit LOD to avoid derivative issues
vec4 sampleTex(vec2 uv) {
    // Clamp to valid range then sample with explicit LOD 0
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

float fbm(vec2 p) {
    float v = 0.0;
    float a = 0.5;
    for (int i = 0; i < 3; i++) {
        v += a * noise(p);
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
    
    float t = time_f * 0.4;
    
    // Gentle warping
    float f1 = fbm(p * 2.0 + t * 0.15);
    float f2 = fbm(p.yx * 2.2 - t * 0.12);
    
    // Swirl
    float dist = length(p);
    float angle = atan(p.y, p.x);
    angle += (f1 - 0.5) * 0.8 + sin(t * 0.5) * 0.4;
    
    vec2 warped = vec2(cos(angle), sin(angle)) * dist;
    
    // Small flow offset
    vec2 flow = vec2(f1 - 0.5, f2 - 0.5) * 0.15;
    
    // Map to UV - keep in 0-1 range
    vec2 finalPos = warped + flow;
    vec2 texUV = finalPos * 0.5 / vec2(aspect, 1.0) + 0.5;
    
    // Chromatic aberration with explicit LOD sampling
    vec2 dir = normalize(p + 0.001) * 0.004;
    
    float r = sampleTex(texUV + dir).r;
    float g = sampleTex(texUV).g;
    float b = sampleTex(texUV - dir).b;
    vec3 col = vec3(r, g, b);
    
    // Enhance
    col *= 1.0 + 0.3 * fbm(warped * 1.5 + t * 0.2);
    
    float gray = dot(col, vec3(0.299, 0.587, 0.114));
    col = mix(vec3(gray), col, 1.4);
    
    outColor = vec4(col, 1.0);
}
