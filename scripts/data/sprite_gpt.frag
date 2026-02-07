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
    float params[4]; // params[0] = time_f, params[1] = iAmplitude, params[2] = iFrequency
} pc;

// Global Mappings to keep original logic intact
float time_f     = pc.params[0];
float iAmplitude = (pc.params[1] <= 0.0) ? 1.0 : pc.params[1];
float iFrequency = (pc.params[2] <= 0.0) ? 1.0 : pc.params[2];
vec2 iResolution = vec2(pc.screenWidth, pc.screenHeight);

// Static Controls
const float iBrightness = 1.0;
const float iContrast   = 1.0;
const float iSaturation = 1.0;
const float iHueShift   = 0.0;
const float iZoom       = 1.0;
const float iRotation   = 0.0;

// --- Helper Functions ---

vec3 adjustBrightness(vec3 col, float b) { return col * b; }
vec3 adjustContrast(vec3 col, float c) { return (col - 0.5) * c + 0.5; }
vec3 adjustSaturation(vec3 col, float s) {
    float gray = dot(col, vec3(0.299, 0.587, 0.114));
    return mix(vec3(gray), col, s);
}

vec3 rotateHue(vec3 col, float angle) {
    float U = cos(angle); float W = sin(angle);
    mat3 R = mat3(
        0.299 + 0.701*U + 0.168*W, 0.587 - 0.587*U + 0.330*W, 0.114 - 0.114*U - 0.497*W,
        0.299 - 0.299*U - 0.328*W, 0.587 + 0.413*U + 0.035*W, 0.114 - 0.114*U + 0.292*W,
        0.299 - 0.300*U + 1.250*W, 0.587 - 0.588*U - 1.050*W, 0.114 + 0.886*U - 0.203*W
    );
    return clamp(R * col, 0.0, 1.0);
}

vec2 applyZoomRotation(vec2 uv, vec2 center) {
    vec2 p = uv - center;
    float c = cos(iRotation); float s = sin(iRotation);
    p = mat2(c, -s, s, c) * p;
    p /= max(abs(iZoom), 0.001);
    return p + center;
}

vec2 wrapUV(vec2 tc) {
    return 1.0 - abs(1.0 - 2.0 * fract(tc * 0.5));
}

// Procedural Noise
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float noise(vec2 p) {
    vec2 i = floor(p); vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i + vec2(0.0, 0.0)), hash(i + vec2(1.0, 0.0)), u.x),
               mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), u.x), u.y);
}

float fbm(vec2 p) {
    float v = 0.0; float a = 0.5;
    for (int i = 0; i < 5; i++) {
        v += a * noise(p);
        p *= 2.1; a *= 0.55;
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

vec3 sampleWarp(vec2 uv, float t, float strength, vec2 center, vec2 res) {
    float aspect = res.x / max(res.y, 1.0); 

    float ampControl  = clamp(iAmplitude,  0.0, 2.0);
    float freqControl = clamp(iFrequency, 0.0, 2.0);

    vec2 p = (uv - center) * vec2(aspect, 1.0);
    float f1 = fbm(p * 1.8 + t * 0.3);
    float f2 = fbm(p.yx * 2.3 - t * 0.25);
    float f3 = fbm(p * 3.1 + vec2(1.3, -0.7) * t * 0.2);

    float a = atan(p.y, p.x) + (f1 * 4.0 + f2 * 2.0) * strength * 0.6;
    vec2 swirl = vec2(cos(a), sin(a)) * length(p);

    float slices = 8.0 + 8.0 * (0.3 + 0.7 * ampControl) + 4.0 * sin(t * 0.17);
    vec2 k = kaleido(swirl + vec2(f2, f3) * 0.4 * strength, slices);

    vec2 flow = k;
    flow.x += f1 * 0.8 * strength;
    flow.y += (f2 - f3) * 0.8 * strength;

    vec2 base = fract(flow / vec2(aspect, 1.0) + center); 

    vec2 chromaShift = 0.0035 * strength * (0.5 + 0.5 * ampControl) *
                       vec2(sin(t + f1 * 6.0), cos(t * 1.3 + f2 * 6.0));

    // Using texture() directly for smoother Vulkan sampling
    float rC = texture(samp, wrapUV(base + chromaShift)).r;
    float gC = texture(samp, wrapUV(base)).g;
    float bC = texture(samp, wrapUV(base - chromaShift)).b;
    vec3 col = vec3(rC, gC, bC);

    col *= (0.7 + 0.6 * f3 + 0.4 * sin(t * 0.6 + f1 * 3.0)) * (0.6 + 0.8 * freqControl);

    float sat = (1.3 + 0.7 * sin(t * 0.43 + f2 * 5.0)) * (0.6 + 0.8 * freqControl);
    return mix(vec3(dot(col, vec3(0.299, 0.587, 0.114))), col, sat);
}

void main() {
    // 1. Get UVs and stabilize time
    vec2 uv = wrapUV(fragTexCoord);
    uv = applyZoomRotation(uv, vec2(0.5));
    float t = mod(time_f, 100.0) * (0.3 + 1.7 * (clamp(iFrequency, 0.0, 2.0) * 0.5));
    
    float ampControl = clamp(iAmplitude, 0.0, 2.0);
    float strength = 0.6 + 1.6 * (ampControl * 0.5);
    vec2 center = vec2(0.5);

    vec3 colA = sampleWarp(uv, t, strength, center, iResolution);
    vec3 colB = sampleWarp(uv + vec2(0.01, -0.007), t + 3.1415, strength * 0.9, center, iResolution);

    vec3 col = mix(colA, colB, 0.5 + 0.5 * sin(t * 0.25));

    col = adjustBrightness(col, iBrightness);
    col = adjustContrast(col, iContrast);
    col = adjustSaturation(col, iSaturation);
    
    outColor = vec4(rotateHue(col, iHueShift), 1.0);
    outColor.rgb *= 0.3;
}
