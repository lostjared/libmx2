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
    float a = hash(i + vec2(0.0, 0.0));
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float fbm(vec2 p, bool ridges) {
    float v = 0.0;
    float a = 0.5;
    for (int i = 0; i < 4; i++) {
        float n = noise(p);
        if (ridges) n = 1.0 - abs(n * 2.0 - 1.0);
        v += a * n;
        p *= 2.05;
        a *= 0.5;
    }
    return v;
}

vec2 kaleido(vec2 p, float slices) {
    float pi = 3.14159265359;
    float r = length(p);
    float a = atan(p.y, p.x);
    float sector = pi * 2.0 / slices;
    a = mod(a, sector);
    a = abs(a - sector * 0.5);
    return vec2(cos(a), sin(a)) * r;
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
        fbm(p + vec2(0.0, 0.0) + t * 0.1, false),
        fbm(p + vec2(5.2, 1.3) - t * 0.1, false)
    );
    
    vec2 r = vec2(
        fbm(p + 4.0 * q + vec2(t * 0.2, 9.2), true),
        fbm(p + 4.0 * q + vec2(8.3, 2.8), false)
    );
    
    // Apply warp
    p += r * 0.08;
    
    // Kaleidoscope
    float slices = 8.0;
    p = kaleido(p, slices);
    
    // Iterative folding
    for(int i = 0; i < 4; i++) {
        p = abs(p);
        p -= 0.1;
        p *= 1.15;
        p.x += 0.5 * r.x;
    }
    
    // Map to texture coords - hardware REPEAT handles wrapping
    vec2 finalUV = p * 0.5 + 0.5;
    
    // Sample texture
    vec3 texCol = texture(samp, finalUV).rgb;
    
    // Electric glow
    float electric = pow(length(r), 3.0) * 0.8;
    
    // Color palette
    vec3 pal = palette(length(p) + length(q) + t);
    
    // Mix
    vec3 finalCol = mix(texCol, pal, 0.3);
    finalCol += pal * electric * 0.5;
    
    // Vignette
    vec2 vUV = uv * (1.0 - uv.yx);
    float vig = pow(vUV.x * vUV.y * 15.0, 0.15);
    finalCol *= vig;
    
    outColor = vec4(finalCol, 1.0);
}
