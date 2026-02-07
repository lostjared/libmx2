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
    float params[4]; // params[0] = time_f
} pc;

// Global Mappings for the original logic
const float PI = 3.14159265358979323846;
float time_f = pc.params[0];
vec2 iResolution = vec2(pc.screenWidth, pc.screenHeight);
vec4 iMouse = vec4(0.5 * pc.screenWidth, 0.5 * pc.screenHeight, 0.0, 0.0); // Default to center

// --- HELPER FUNCTIONS ---

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect) {
    float s = sin(angle), cc = cos(angle);
    vec2 p = uv - c;
    p.x *= aspect;
    p = mat2(cc, -s, s, cc) * p;
    p.x /= aspect;
    return p + c;
}

vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect) {
    vec2 p = uv - c;
    p.x *= aspect;
    float ang = atan(p.y, p.x);
    float rad = length(p);
    float stepA = 6.28318530718 / max(segments, 1.0);
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect) {
    vec2 p = uv;
    for (int i = 0; i < 6; i++) {
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 neonPalette(float t) {
    vec3 pink = vec3(1.0, 0.15, 0.75);
    vec3 blue = vec3(0.10, 0.55, 1.0);
    vec3 green = vec3(0.10, 1.00, 0.45);
    float ph = fract(t * 0.08);
    vec3 k1 = mix(pink, blue, smoothstep(0.00, 0.33, ph));
    vec3 k2 = mix(blue, green, smoothstep(0.33, 0.66, ph));
    vec3 k3 = mix(green, pink, smoothstep(0.66, 1.00, ph));
    float a = step(ph, 0.33);
    float b = step(0.33, ph) * step(ph, 0.66);
    float c = step(0.66, ph);
    return normalize(a * k1 + b * k2 + c * k3) * 1.05;
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    // Vulkan uses texture() instead of textureGrad if dFdx isn't strictly required
    return texture(img, uv).rgb * 0.5 + texture(img, uv + ts).rgb * 0.5; 
}

vec3 preBlendColor(vec2 uv) {
    vec3 tex = tentBlur3(samp, uv, iResolution);
    float aspect = iResolution.x / max(iResolution.y, 1.0);
    vec2 p = (uv - 0.5) * vec2(aspect, 1.0);
    float r = length(p);
    float t = time_f;
    vec3 neon = neonPalette(t + r * 1.3);
    float neonAmt = smoothstep(0.1, 0.8, r);
    neonAmt = 0.3 + 0.4 * (1.0 - neonAmt);
    vec3 grad = mix(tex, neon, neonAmt);
    grad = mix(grad, tex, 0.2);
    // Soft tone inline
    grad = pow(max(grad, 0.0), vec3(0.95));
    return clamp(grad, 0.0, 1.0);
}

float diamondRadius(vec2 p) {
    p = sin(abs(p));
    return max(p.x, p.y);
}

vec2 diamondFold(vec2 uv, vec2 c, float aspect) {
    vec2 p = (uv - c) * vec2(aspect, 1.0);
    p = abs(p);
    if (p.y > p.x) p = p.yx;
    p.x /= aspect;
    return p + c;
}

void main() {
    vec2 tc = fragTexCoord;
    vec4 baseTex = texture(samp, tc);
    float aspect = iResolution.x / max(iResolution.y, 1.0);
    
    // Safety wrap on time to prevent flashing at high tick counts
    float t = mod(time_f, 120.0);
    
    vec2 m = vec2(0.5); // Logic for center
    vec2 ar = vec2(aspect, 1.0);
    vec3 baseCol = preBlendColor(tc);
    
    float seg = 4.0 + 2.0 * sin(t * 0.33);
    vec2 kUV = reflectUV(tc, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    
    float foldZoom = 1.45 + 0.55 * sin(t * 0.42);
    kUV = fractalFold(kUV, foldZoom, t, m, aspect);
    kUV = rotateUV(kUV, t * 0.23, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    
    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if (q.y > q.x) q = q.yx;
    
    float base = 1.82 + 0.18 * pingPong(sin(t * 0.2) * (PI * t), 5.0);
    float period = log(max(base, 1.1)) * pingPong(t * PI, 5.0);
    float tz = pingPong(t * PI/4.0, 3.0) * 0.65;
    float rD = diamondRadius(p) + 1e-6;
    
    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35 * sin(rD * 18.0 + pingPong(t * PI/2.0, 3.0) * 0.6);
    float k = fract((log(rD) - tz) / max(period, 0.001));
    float rw = exp(k * period);
    
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;
    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap * 1.045) / ar + m);
    vec2 u2 = fract((pwrap * 0.955) / ar + m);
    
    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(t * 1.3)) * vec2(1.0, 1.0 / aspect);
    
    float vign = 1.0 - smoothstep(0.75, 1.2, length((tc - m) * ar));
    vign = mix(0.9, 1.15, vign);
    
    vec3 rC = preBlendColor(u0 + off);
    vec3 gC = preBlendColor(u1);
    vec3 bC = preBlendColor(u2 - off);
    
    vec3 outCol = vec3(rC.r, gC.g, bC.b);
    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + pingPong(t * PI, 5.0) * 1.2));
    ring = ring * pingPong((t * PI), 5.0);
    float pulse = 0.5 + 0.5 * sin(pingPong(t * PI, 5.0) * 2.0 + rD * 28.0 + k * 12.0);

    outCol *= (0.75 + 0.25 * ring) * (0.85 + 0.15 * pulse) * vign;
    outCol += outCol * outCol * 0.18 + pow(max(outCol - 0.6, 0.0), vec3(2.0)) * 0.12;
    
    outCol = mix(outCol, baseCol, pingPong(pulse * PI, 5.0) * 0.18);
    outCol = clamp(outCol, vec3(0.05), vec3(0.97));
    
    // Handle the "Glow" transition
    float radius = sqrt(aspect * aspect + 1.0) + 0.5;
    vec2 glowUV = tc * 2.0 - 1.0;
    glowUV.x *= aspect;
    float glowVal = pingPong(sin(length(glowUV) * t), 5.0);
    float glowFactor = smoothstep(radius, radius - 0.25, glowVal);
    
    vec3 finalRGB = mix(baseTex.rgb, outCol, pingPong(glowFactor * PI, 5.0) * 0.8);
    outColor = vec4(finalRGB, baseTex.a);
    outColor.rgb *= 0.3;
}
