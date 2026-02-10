#version 450

layout(location = 0) in vec3 localPos;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform samplerCube cubemapTexture;
layout(binding = 1) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 params; // x=time, y=width, z=height
} ubo;

const float PI = 3.1415926535897932384626433832795;

vec4 getCubemapColor(vec3 apos, samplerCube cube) {
    float r = length(apos.xy);
    float theta = atan(apos.y, apos.x);
    float spiralEffect = ubo.params.x * 0.2;
    r -= mod(spiralEffect, 4.0);
    theta += spiralEffect;
    vec2 distortedXY = vec2(cos(theta), sin(theta)) * r;
    vec3 newApos = normalize(vec3(distortedXY, apos.z));
    vec4 texColor = texture(cube, newApos);
    return vec4(texColor.rgb, 1.0);
}

vec2 octahedralMap(vec3 n) {
    n = normalize(n);
    n /= (abs(n.x) + abs(n.y) + abs(n.z));
    vec2 uv = n.xy;
    if (n.z < 0.0) {
        uv = (1.0 - abs(uv.yx)) * sign(uv.xy);
    }
    return uv * 0.5 + 0.5;
}

vec3 octahedralUnmap(vec2 uv) {
    vec2 f = uv * 2.0 - 1.0;
    vec3 n = vec3(f.x, f.y, 1.0 - abs(f.x) - abs(f.y));
    float t = clamp(-n.z, 0.0, 1.0);
    n.x += (n.x >= 0.0) ? -t : t;
    n.y += (n.y >= 0.0) ? -t : t;
    return normalize(n);
}

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
    float stepA = 6.28318530718 / segments;
    ang = mod(ang, stepA);
    ang = abs(ang - stepA * 0.5);
    vec2 r = vec2(cos(ang), sin(ang)) * rad;
    r.x /= aspect;
    return r + c;
}

vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect) {
    vec2 p = uv;
    for (int i = 0; i < 3; i++) {
        p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;
        p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);
    }
    return p;
}

vec3 sampleBase(vec2 uv) {
    vec3 dir = octahedralUnmap(fract(uv));
    return texture(cubemapTexture, dir).rgb;
}

vec3 tentBlur3(vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec3 s00 = sampleBase(uv + ts * vec2(-1.0, -1.0));
    vec3 s10 = sampleBase(uv + ts * vec2(0.0, -1.0));
    vec3 s20 = sampleBase(uv + ts * vec2(1.0, -1.0));
    vec3 s01 = sampleBase(uv + ts * vec2(-1.0, 0.0));
    vec3 s11 = sampleBase(uv);
    vec3 s21 = sampleBase(uv + ts * vec2(1.0, 0.0));
    vec3 s02 = sampleBase(uv + ts * vec2(-1.0, 1.0));
    vec3 s12 = sampleBase(uv + ts * vec2(0.0, 1.0));
    vec3 s22 = sampleBase(uv + ts * vec2(1.0, 1.0));
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv, vec2 res) {
    vec3 tex = tentBlur3(uv, res);
    return tex * 0.7;
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
    vec3 tc = normalize(localPos);
    vec4 texColor = getCubemapColor(localPos, cubemapTexture);
    float sparkle = abs(sin(ubo.params.x * 10.0 + tc.x * 100.0) * cos(ubo.params.x * 15.0 + tc.y * 100.0));
    vec3 magicalColor = vec3(
        sin(ubo.params.x * 2.0) * 0.5 + 0.5,
        cos(ubo.params.x * 3.0) * 0.5 + 0.5,
        sin(ubo.params.x * 4.0) * 0.5 + 0.5
    );
    vec4 baseTex = texColor + vec4(magicalColor * sparkle, 1.0);

    vec2 res = vec2(max(ubo.params.y, 1.0), max(ubo.params.z, 1.0));
    float time_f = ubo.params.x;
    vec2 tcUV = octahedralMap(tc);
    vec2 xuv = 1.0 - abs(1.0 - 2.0 * tcUV);
    xuv = xuv - floor(xuv);

    vec2 uv = tcUV * 2.0 - 1.0;
    float aspect = res.x / res.y;
    uv.x *= aspect;

    float r = pingPong(sin(length(uv) * time_f), 5.0);
    float radius = sqrt(aspect * aspect + 1.0) + 0.5;
    float glow = smoothstep(radius, radius - 0.5, r);

    vec2 m = vec2(0.5);
    vec2 ar = vec2(aspect, 1.0);
    vec3 baseCol = preBlendColor(tcUV, res);

    float seg = 4.0 + 2.0 * sin(time_f * 0.33);
    vec2 kUV = reflectUV(tcUV, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    float foldZoom = 1.15 + 0.15 * sin(time_f * 0.42);
    kUV = fractalFold(kUV, foldZoom, time_f, m, aspect);
    kUV = rotateUV(kUV, time_f * 0.23, m, aspect);
    kUV = diamondFold(kUV, m, aspect);

    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if (q.y > q.x) q = q.yx;

    float base = 1.82 + 0.18 * pingPong(sin(time_f * 0.2) * (PI * time_f), 5.0);
    float period = log(base) * pingPong(time_f * PI, 5.0);
    float tz = time_f * 0.65;

    float rD = diamondRadius(p) + 1e-6;
    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35 * sin(rD * 18.0 + time_f * 0.6);
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw * 0.5;

    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap * 1.045) / ar + m);
    vec2 u2 = fract((pwrap * 0.955) / ar + m);

    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(time_f * 1.3)) * vec2(1.0, 1.0 / aspect);

    float vign = 1.0 - smoothstep(0.75, 1.2, length((tcUV - m) * ar));
    vign = mix(0.9, 1.15, vign);

    vec3 rC = preBlendColor(u0 + off, res);
    vec3 gC = preBlendColor(u1, res);
    vec3 bC = preBlendColor(u2 - off, res);
    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);

    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + time_f * 1.2));
    ring = ring * pingPong((time_f * PI), 5.0);

    float pulse = 0.5 + 0.5 * sin(time_f * 2.0 + rD * 28.0 + k * 12.0);
    vec3 outCol = kaleidoRGB;
    outCol *= (0.6 + 0.4 * pulse);
    outCol *= vign;
    outCol = clamp(outCol, vec3(0.0), vec3(1.0));

    vec3 finalRGB = mix(baseTex.rgb * 0.8, outCol, pingPong(glow * PI, 5.0) * 0.6);
    outColor = vec4(finalRGB, baseTex.a);
}