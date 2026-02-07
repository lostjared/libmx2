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
    float params[4]; 
    // Mapping:
    // params[0] = time_f
    // params[1] = amp (Audio amplitude)
    // params[2] = uamp (Alternative audio/user amplitude)
} pc;

const float PI = 3.14159265358979323846;

// Global simulation variables
float time_f     = pc.params[0];
float amp        = pc.params[1];
float uamp       = pc.params[2];
vec2 iResolution = vec2(pc.screenWidth, pc.screenHeight);

// --- HELPER FUNCTIONS ---

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
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

vec3 preBlendColor(vec2 uv) {
    vec3 tex = texture(samp, uv).rgb;
    float aspect = iResolution.x / max(iResolution.y, 1.0);
    vec2 p = (uv - 0.5) * vec2(aspect, 1.0);
    float r = length(p);
    vec3 neon = neonPalette(time_f + r * 1.3);
    float neonAmt = smoothstep(0.1, 0.8, r);
    neonAmt = 0.3 + 0.4 * (1.0 - neonAmt);
    vec3 grad = mix(tex, neon, neonAmt);
    // Soft tone
    grad = pow(max(grad, 0.0), vec3(0.95));
    float l = dot(grad, vec3(0.299, 0.587, 0.114));
    return clamp(mix(vec3(l), grad, 0.9), 0.0, 1.0);
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
    // 1. Audio and Time normalization
    float audio = clamp(amp * 0.8 + uamp * 0.6, 0.0, 5.0);
    float audioNorm = clamp(audio * 0.5, 0.0, 2.5);
    float t = time_f * (1.0 + audioNorm * 0.25);
    float aspect = iResolution.x / max(iResolution.y, 1.0);
    vec2 ar = vec2(aspect, 1.0);
    vec2 m = vec2(0.5); // Fixed center

    // 2. Bubble Masking Logic
    vec2 rel = fragTexCoord - m;
    float dist = length(rel * ar); // Aspect corrected distance
    float bubSize = 0.38 + 0.12 * audioNorm;
    float bubEdge = 0.28 + 0.10 * audioNorm;
    float bubMask = 1.0 - smoothstep(bubSize, bubSize + bubEdge, dist);

    float zoom = mix(1.0, 1.3 + 0.7 * audioNorm, 0.5 + 0.5 * sin(t * 1.6 + audioNorm * 2.3));
    vec2 tcZoom = m + rel / zoom;
    vec2 tcF = mix(fragTexCoord, tcZoom, bubMask);

    vec4 baseTex = texture(samp, tcF);
    vec2 uv = tcF * 2.0 - 1.0;
    uv.x *= aspect;

    // 3. Glow and Kaleidoscope
    float glow = smoothstep(sqrt(aspect * aspect + 1.0) + 0.5, sqrt(aspect * aspect + 1.0) + 0.25, pingPong(sin(length(uv) * t), 5.0));
    vec3 baseCol = preBlendColor(tcF);

    float seg = 4.0 + 2.0 * sin(t * 0.33);
    vec2 kUV = reflectUV(tcF, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    kUV = fractalFold(kUV, 1.45 + 0.55 * sin(t * 0.42), t, m, aspect);
    kUV = rotateUV(kUV, t * 0.23, m, aspect);
    kUV = diamondFold(kUV, m, aspect);

    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p); if (q.y > q.x) q = q.yx;

    float period = log(1.82 + 0.18 * pingPong(sin(t * 0.2) * (PI * t), 5.0)) * pingPong(t * PI, 5.0);
    float rD = diamondRadius(p) + 1e-6;
    float k = fract((log(rD) - t * 0.65) / max(period, 0.001));
    vec2 pwrap = vec2(cos(atan(q.y, q.x) + t * 0.2275 + 0.35 * sin(rD * 18.0 + t * 0.6)), 
                      sin(atan(q.y, q.x) + t * 0.2275 + 0.35 * sin(rD * 18.0 + t * 0.6))) * exp(k * period);

    // 4. Color Compositing
    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(t * 1.3)) * vec2(1.0, 1.0 / aspect);
    vec3 kaleidoRGB = vec3(preBlendColor(fract(pwrap / ar + m) + off).r, 
                           preBlendColor(fract((pwrap * 1.045) / ar + m)).g, 
                           preBlendColor(fract((pwrap * 0.955) / ar + m) - off).b);

    float angDir = atan(p.y, p.x) / (2.0 * PI) + 0.5;
    vec3 gradCol = hsv2rgb(vec3(fract(angDir + (clamp(length(p) * 0.9, 0.0, 1.0) - 0.5) * 0.4 + (pingPong(time_f * 0.18 + audioNorm * 0.12, 1.0) - 0.5) * 0.6), 0.4 + 0.4 * clamp(length(p) * 0.9, 0.0, 1.0), 0.45 + 0.45 * (1.0 - clamp(length(p) * 0.9, 0.0, 1.0))));

    vec3 outCol = mix(kaleidoRGB, kaleidoRGB * gradCol, (0.18 + 0.25 * audioNorm) * (0.3 + 0.7 * bubMask));
    outCol *= (0.60 + 0.20 * smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + t * 1.2)) * pingPong(t * PI, 5.0)) * (0.80 + 0.10 * (0.5 + 0.5 * sin(t * 2.0 + rD * 28.0 + k * 12.0 + audioNorm * 3.0))) * mix(0.9, 1.15, 1.0 - smoothstep(0.75, 1.2, length((tcF - m) * ar)));
    
    outCol += (outCol * outCol * 0.08) * 0.3;
    outCol = mix(outCol, baseTex.rgb, 0.18 + audioNorm * 0.04);
    outCol += hsv2rgb(vec3(fract(angDir), 0.7, 0.9)) * smoothstep(bubSize + 0.02, bubSize - 0.02, dist) * (0.10 + 0.20 * audioNorm) * bubMask * 0.5;

    vec3 finalRGB = mix(baseTex.rgb, clamp(outCol, 0.05, 0.90), pingPong(glow * PI, 5.0) * 0.7 + 0.1);
    outColor = vec4(clamp(finalRGB, 0.03, 0.97), baseTex.a);
    outColor.rgb *= 0.6;
}
