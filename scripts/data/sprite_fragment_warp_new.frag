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
    float a = hash(i + vec2(0.0, 0.0));
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float fbm(vec2 p) {
    float v = 0.0;
    float a = 0.5;
    for (int i = 0; i < 4; i++) {
        v += a * noise(p);
        p *= 2.1;
        a *= 0.55;
    }
    return v;
}

vec2 kaleido(vec2 p, float slices) {
    float pi = 3.14159265359;
    float r = length(p);
    float ang = atan(p.y, p.x);
    float sector = pi * 2.0 / slices;
    ang = mod(ang, sector);
    ang = abs(ang - sector * 0.5);
    return vec2(cos(ang), sin(ang)) * r;
}

void main() {
    vec2 iResolution = vec2(pc.screenWidth, pc.screenHeight);
    float time_f = pc.param0;
    
    vec2 uv = fragTexCoord;
    float aspect = iResolution.x / iResolution.y;
    
    // Center and apply aspect ratio
    vec2 p = (uv - 0.5) * vec2(aspect, 1.0);
    
    float t = time_f * 0.5;
    
    // Create flowing noise field
    float f1 = fbm(p * 1.8 + t * 0.3);
    float f2 = fbm(p.yx * 2.3 - t * 0.25);
    float f3 = fbm(p * 3.1 + vec2(1.3, -0.7) * t * 0.2);
    
    // Swirl effect
    vec2 swirl = p;
    float r = length(swirl);
    float ang = atan(swirl.y, swirl.x);
    ang += (f1 * 4.0 + f2 * 2.0) * 0.4;
    swirl = vec2(cos(ang), sin(ang)) * r;
    
    // Kaleidoscope
    float slices = 8.0 + 8.0 * (0.5 + 0.5 * sin(t * 0.17));
    vec2 k = kaleido(swirl + vec2(f2, f3) * 0.3, slices);
    
    // Add flow
    vec2 flow = k;
    flow.x += f1 * 0.6;
    flow.y += (f2 - f3) * 0.6;
    
    // Convert back to UV space - hardware REPEAT handles wrapping
    vec2 texUV = flow / vec2(aspect, 1.0) + 0.5;
    
    // Chromatic aberration
    vec2 offset = 0.003 * vec2(sin(t + f1 * 6.0), cos(t * 1.3 + f2 * 6.0));
    
    float rC = texture(samp, texUV + offset).r;
    float gC = texture(samp, texUV).g;
    float bC = texture(samp, texUV - offset).b;
    vec3 col = vec3(rC, gC, bC);
    
    // Color modulation
    float bright = 0.8 + 0.4 * f3 + 0.3 * sin(t * 0.6 + f1 * 3.0);
    col *= bright;
    
    float sat = 1.2 + 0.5 * sin(t * 0.43 + f2 * 5.0);
    float gray = dot(col, vec3(0.299, 0.587, 0.114));
    col = mix(vec3(gray), col, sat);
    
    outColor = vec4(col, 1.0);
}
