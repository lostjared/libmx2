#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D samp;

// MATCHES YOUR C++ STRUCT EXACTLY
layout(push_constant) uniform PushConstants {
    float screenWidth;
    float screenHeight;
    float spritePosX;
    float spritePosY;
    float spriteSizeW;
    float spriteSizeH;
    float padding1;
    float padding2;
    float param0; // Time
    float param1; // Amplitude
    float param2; // Frequency
    float param3; // Contrast
} pc;

// Define globals so functions can see them
float iAmplitude  = (pc.param1 <= 0.0) ? 1.0 : pc.param1; 
float iFrequency  = (pc.param2 <= 0.0) ? 1.0 : pc.param2;
float iContrast   = (pc.param3 <= 0.0) ? 1.0 : pc.param3;
float iBrightness = 1.0;
float iSaturation = 1.0;
float iHueShift   = 0.0;
float iZoom       = 1.0;
float iRotation   = 0.0;

// --- COLOR HELPER FUNCTIONS ---
vec3 adjustBrightness(vec3 col, float b) { return col * b; }
vec3 adjustContrast(vec3 col, float c) { return (col - 0.5) * c + 0.5; }
vec3 adjustSaturation(vec3 col, float s) {
    float gray = dot(col, vec3(0.299, 0.587, 0.114));
    return mix(vec3(gray), col, s);
}

vec3 palette(float t) {
    vec3 a = vec3(0.5, 0.5, 0.5);
    vec3 b = vec3(0.5, 0.5, 0.5);
    vec3 c = vec3(1.0, 1.0, 1.0);
    vec3 d = vec3(0.263, 0.416, 0.557); 
    return a + b * cos(6.28318 * (c * t + d + iHueShift));
}

vec2 wrapUV(vec2 tc) {
    return 1.0 - abs(1.0 - 2.0 * fract(tc * 0.5));
}

vec2 rotate2D(vec2 p, float a) {
    float c = cos(a); float s = sin(a);
    return mat2(c, -s, s, c) * p;
}

// --- NOISE ---
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
    float sector = (pi * 2.0) / max(slices, 1.0);
    a = mod(a, sector);
    a = abs(a - sector * 0.5);
    return vec2(cos(a), sin(a)) * r;
}

vec3 sampleHybrid(vec2 uv, float t, float strength, vec2 center, vec2 res) {
    float aspect = res.x / max(res.y, 1.0);
    vec2 p = (uv - center) * vec2(aspect, 1.0);
    
    p = rotate2D(p, iRotation);
    p /= max(iZoom, 0.01);

    vec2 q = vec2(fbm(p + t * 0.1, false), fbm(p - t * 0.1 + 5.2, false));
    vec2 r = vec2(fbm(p + 4.0 * q + vec2(t * 0.2, 9.2), true), fbm(p + 4.0 * q + 8.3, false));

    p += r * (0.1 * strength);
    p = kaleido(p, 6.0 + floor(iAmplitude * 4.0));
    
    float scale = 1.1 + (iFrequency * 0.3);
    for(int i = 0; i < 5; i++) {
        p = abs(p) - (0.1 * strength);
        p *= scale;
        p = rotate2D(p, (t * 0.2) + float(i)*0.5 + (r.x * 0.2)); 
    }

    vec2 finalUV = p * 0.5 + center;
    vec3 texCol = texture(samp, wrapUV(finalUV)).rgb;
    float electric = pow(length(r), 3.0) * iContrast; 
    vec3 pal = palette(length(p) + length(q) + t);
    
    return mix(texCol, pal, 0.4 * iSaturation) + (pal * electric * strength);
}

void main() {
    vec2 res = vec2(max(pc.screenWidth, 1.0), max(pc.screenHeight, 1.0));
    vec2 uv = fragTexCoord;
    
    float t = pc.param0 * (0.2 + iFrequency * 0.2);
    float strength = 0.5 + (iAmplitude * 1.0);
    vec2 center = vec2(0.5);

    vec3 col;
    col.r = sampleHybrid(uv + vec2(0.003, 0.0), t, strength, center, res).r;
    col.g = sampleHybrid(uv, t, strength, center, res).g;
    col.b = sampleHybrid(uv - vec2(0.003, 0.0), t, strength, center, res).b;

    col = adjustBrightness(col, iBrightness);
    col = adjustContrast(col, iContrast);
    col = adjustSaturation(col, iSaturation);
    
    vec2 vUV = uv * (1.0 - uv.yx); 
    col *= pow(vUV.x * vUV.y * 15.0, 0.15);

    outColor = vec4(col, 1.0);
}