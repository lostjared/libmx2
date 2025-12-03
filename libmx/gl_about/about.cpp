#include"mx.hpp"
#include"argz.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <GLES3/gl3.h>
#endif

#include"gl.hpp"
#include"loadpng.hpp"
#include<iostream>
#define CHECK_GL_ERROR() \
{ GLenum err = glGetError(); \
if (err != GL_NO_ERROR) \
printf("OpenGL Error: %d at %s:%d\n", err, __FILE__, __LINE__); }

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const char *srcShader1 = R"(#version 300 es
precision highp float;
out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;

void main(void) {
    vec2 uv = TexCoord * 2.0 - 1.0;
    float len = length(uv);
    float bubble = smoothstep(0.8, 1.0, 1.0 - len);
    vec2 distort = uv * (1.0 + 0.1 * sin(time_f + len * 20.0));
    vec4 texColor = texture(textTexture, distort * 0.5 + 0.5);
    FragColor = mix(texColor, vec4(1.0, 1.0, 1.0, 1.0), bubble);
}
)";

const char *srcShader3 = R"(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;

const float PI = 3.1415926535897932384626433832795;

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
    float stepA = 6.28318530718 / segments;
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

vec3 softTone(vec3 c) {
    c = pow(max(c, 0.0), vec3(0.95));
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, 0.9);
    return clamp(c, 0.0, 1.0);
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec3 s00 = textureGrad(img, uv + ts * vec2(-1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s10 = textureGrad(img, uv + ts * vec2(0.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s20 = textureGrad(img, uv + ts * vec2(1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s01 = textureGrad(img, uv + ts * vec2(-1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s11 = textureGrad(img, uv, dFdx(uv), dFdy(uv)).rgb;
    vec3 s21 = textureGrad(img, uv + ts * vec2(1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s02 = textureGrad(img, uv + ts * vec2(-1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s12 = textureGrad(img, uv + ts * vec2(0.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s22 = textureGrad(img, uv + ts * vec2(1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv) {
    vec3 tex = tentBlur3(textTexture, uv, iResolution);
    float aspect = iResolution.x / iResolution.y;
    vec2 p = (uv - 0.5) * vec2(aspect, 1.0);
    float r = length(p);
    float t = time_f;
    vec3 neon = neonPalette(t + r * 1.3);
    float neonAmt = smoothstep(0.1, 0.8, r);
    neonAmt = 0.3 + 0.4 * (1.0 - neonAmt);
    vec3 grad = mix(tex, neon, neonAmt);
    grad = mix(grad, tex, 0.2);
    grad = softTone(grad);
    return grad;
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

void main(void) {
    vec2 tc = TexCoord;
    vec4 baseTex = texture(textTexture, tc);
    vec2 uv = tc * 2.0 - 1.0;
    float aspect = iResolution.x / iResolution.y;
    uv.x *= aspect;
    float r = pingPong(sin(length(uv) * time_f), 5.0);
    float radius = sqrt(aspect * aspect + 1.0) + 0.5;
    vec2 m = (iMouse.z > 0.5) ? vec2(iMouse.x / iResolution.x, 1.0 - iMouse.y / iResolution.y) : vec2(0.5);
    float glow = smoothstep(radius, radius - 0.25, r);
    vec2 ar = vec2(aspect, 1.0);
    vec3 baseCol = preBlendColor(tc);
    float seg = 4.0 + 2.0 * sin(time_f * 0.33);
    vec2 kUV = reflectUV(tc, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    float foldZoom = 1.45 + 0.55 * sin(time_f * 0.42);
    kUV = fractalFold(kUV, foldZoom, time_f, m, aspect);
    kUV = rotateUV(kUV, time_f * 0.23, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    vec2 p = (kUV - m) * ar;
    vec2 q = abs(p);
    if (q.y > q.x) q = q.yx;
    float base = 1.82 + 0.18 * pingPong(sin(time_f * 0.2) * (PI * time_f), 5.0);
    float period = log(base) * pingPong(time_f * PI, 5.0);
    float tz = pingPong(time_f * PI/2.0, 3.0) * 0.65;  // Changed 2 to 2.0
    float rD = diamondRadius(p) + 1e-6;
    float ang = atan(q.y, q.x) + tz * 0.35 + 0.35 * sin(rD * 18.0 + pingPong(time_f * PI/2.0, 3.0) * 0.6);  // Changed 2 to 2.0
    float k = fract((log(rD) - tz) / period);
    float rw = exp(k * period);
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;
    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap * 1.045) / ar + m);
    vec2 u2 = fract((pwrap * 0.955) / ar + m);
    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(time_f * 1.3)) * vec2(1.0, 1.0 / aspect);
    float vign = 1.0 - smoothstep(0.75, 1.2, length((tc - m) * ar));
    vign = mix(0.9, 1.15, vign);
    vec3 rC = preBlendColor(u0 + off);
    vec3 gC = preBlendColor(u1);
    vec3 bC = preBlendColor(u2 - off);
    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);
    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + pingPong(time_f * PI, 5.0) * 1.2));
    ring = ring * pingPong((time_f * PI), 5.0);
    float pulse = 0.5 + 0.5 * sin(pingPong(time_f  *PI, 5.0) * 2.0 + rD * 28.0 + k * 12.0);

    vec3 outCol = kaleidoRGB;
    outCol *= (0.75 + 0.25 * ring) * (0.85 + 0.15 * pulse) * vign;
    vec3 bloom = outCol * outCol * 0.18 + pow(max(outCol - 0.6, 0.0), vec3(2.0)) * 0.12;
    outCol += bloom;
    outCol = mix(outCol, baseCol, pingPong(pulse * PI, 5.0) * 0.18);
    outCol = clamp(outCol, vec3(0.05), vec3(0.97));
    vec3 finalRGB = mix(baseTex.rgb, outCol, pingPong(glow * PI, 5.0) * 0.8);
    color = vec4(finalRGB, baseTex.a);
}
)";

const char *srcShader4 = R"(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

void main(void) {
    vec2 tc = TexCoord;
    vec2 uv = tc;
    float time_t =  pingPong(time_f, 10.0);
    float scrambleAmount = sin(time_f * 10.0) * 0.5 + 0.5;
    scrambleAmount = cos(scrambleAmount * time_t);
    uv.x += sin(uv.y * 50.0 + time_f * 10.0) * scrambleAmount * 0.05;
    uv.y += cos(uv.x * 50.0 + time_f * 10.0) * scrambleAmount * 0.05;
    color = texture(textTexture, tan(uv * time_t));
})";

const char *srcShader5 = R"(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;
uniform float alpha;


float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

float smoothNoise(vec2 uv) {
    return sin(uv.x * 12.0 + uv.y * 14.0 + time_f * 0.8) * 0.5 + 0.5;
}

void main(void) {
    vec2 tc = TexCoord;
    vec2 uv = (tc * iResolution - 0.5 * iResolution) / iResolution.y;
    float radius = length(uv);
    float angle = atan(uv.y, uv.x);
    float swirl = sin(time_f * 0.4) * 2.0;
    angle += swirl * radius * 1.5;
    float modRadius = pingPong(radius + time_f * 0.3, 0.8);
    float wave = sin(radius * 12.0 - time_f * 3.0) * 0.5 + 0.5;
    float cloudNoise = smoothNoise(uv * 3.0 + vec2(modRadius, angle * 0.5));
    cloudNoise += smoothNoise(uv * 6.0 - vec2(time_f * 0.2, time_f * 0.1));
    cloudNoise = pow(cloudNoise, 1.5);
    float r = sin(angle * 3.0 + modRadius * 8.0 + wave * 2.0) * cloudNoise;
    float g = sin(angle * 5.0 - modRadius * 6.0 + wave * 4.0) * cloudNoise;
    float b = sin(angle * 7.0 + modRadius * 10.0 - wave * 3.0) * cloudNoise;
    vec3 col = vec3(r, g, b) * 0.5 + 0.5;
    vec3 texColor = texture(textTexture, tc).rgb;
    col = mix(col, texColor, 0.25);
    color = vec4(sin(col * pingPong(time_f * 1.5, 6.0) + 1.5), alpha);

}
)";
 

const char *srcShader2 = R"(#version 300 es
precision highp float;
out vec4 color;in vec2 TexCoord;
uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
uniform vec4 iMouse;

const float PI = 3.1415926535897932384626433832795;

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
    float stepA = 6.28318530718 / segments;
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

vec3 softTone(vec3 c) {
    c = pow(max(c, 0.0), vec3(0.95));
    float l = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(l), c, 0.9);
    return clamp(c, 0.0, 1.0);
}

vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec3 s00 = textureGrad(img, uv + ts * vec2(-1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s10 = textureGrad(img, uv + ts * vec2(0.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s20 = textureGrad(img, uv + ts * vec2(1.0, -1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s01 = textureGrad(img, uv + ts * vec2(-1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s11 = textureGrad(img, uv, dFdx(uv), dFdy(uv)).rgb;
    vec3 s21 = textureGrad(img, uv + ts * vec2(1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s02 = textureGrad(img, uv + ts * vec2(-1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s12 = textureGrad(img, uv + ts * vec2(0.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    vec3 s22 = textureGrad(img, uv + ts * vec2(1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;
    return (s00 + 2.0 * s10 + s20 + 2.0 * s01 + 4.0 * s11 + 2.0 * s21 + s02 + 2.0 * s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv) {
    vec3 tex = tentBlur3(textTexture, uv, iResolution);
    float aspect = iResolution.x / iResolution.y;
    vec2 p = (uv - 0.5) * vec2(aspect, 1.0);
    float r = length(p);
    float t = time_f;
    vec3 neon = neonPalette(t + r * 1.3);
    float neonAmt = smoothstep(0.1, 0.8, r);
    neonAmt = 0.3 + 0.4 * (1.0 - neonAmt);
    vec3 grad = mix(tex, neon, neonAmt);
    grad = mix(grad, tex, 0.2);
    grad = softTone(grad);
    return grad;
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

void main(void) {
    vec2 tc = TexCoord;
    vec4 baseTex = texture(textTexture, tc);
    vec2 uv = tc * 2.0 - 1.0;
    float aspect = iResolution.x / iResolution.y;
    uv.x *= aspect;
    float r = pingPong(sin(length(uv) * time_f), 5.0); 
    float radius = sqrt(aspect * aspect + 1.0) + 0.5;
    float glow = smoothstep(radius, radius - 0.25, r);
    vec2 m = (iMouse.z > 0.5) ? (iMouse.xy / iResolution) : vec2(0.5);
    vec2 ar = vec2(aspect, 1.0);
    vec3 baseCol = preBlendColor(tc);
    float seg = 4.0 + 2.0 * sin(time_f * 0.33);
    vec2 kUV = reflectUV(tc, seg, m, aspect);
    kUV = diamondFold(kUV, m, aspect);
    float foldZoom = 1.45 + 0.55 * sin(time_f * 0.42);
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
    vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;
    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap * 1.045) / ar + m);
    vec2 u2 = fract((pwrap * 0.955) / ar + m);
    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(time_f * 1.3)) * vec2(1.0, 1.0 / aspect);
    float vign = 1.0 - smoothstep(0.75, 1.2, length((tc - m) * ar));
    vign = mix(0.9, 1.15, vign);
    vec3 rC = preBlendColor(u0 + off);
    vec3 gC = preBlendColor(u1);
    vec3 bC = preBlendColor(u2 - off);
    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);
    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + time_f * 1.2));
    ring = ring * pingPong((time_f * PI), 5.0);
    float pulse = 0.5 + 0.5 * sin(time_f * 2.0 + rD * 28.0 + k * 12.0);
    vec3 outCol = kaleidoRGB;
    outCol *= (0.75 + 0.25 * ring) * (0.85 + 0.15 * pulse) * vign;
    vec3 bloom = outCol * outCol * 0.18 + pow(max(outCol - 0.6, 0.0), vec3(2.0)) * 0.12;

    outCol += bloom;
    outCol = mix(outCol, baseCol, pingPong(pulse *  PI, 5.0) * 0.18);
    outCol = clamp(outCol, vec3(0.05), vec3(0.97));
    vec3 finalRGB = mix(baseTex.rgb, outCol, pingPong(glow * PI, 5.0) * 0.8);
    color = vec4(finalRGB, baseTex.a);
}
)";

const char *szShader = R"(#version 300 es
precision highp float;
in vec2 TexCoord;
out vec4 color;
uniform float time_f;
uniform sampler2D textTexture;
uniform vec2 iResolution;

const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}


void main(void) {
    vec2 tc = TexCoord;
    vec2 uv = tc;

    vec2 reflectedUV = vec2(1.0 - uv.x, uv.y);
    float noise = sin(reflectedUV.y * 50.0 + time_f * 5.0) * 0.01;
    noise = sin(noise * pingPong(time_f * PI, 6.0));
    float distortionX = noise;
    float distortionY = cos(reflectedUV.x * 50.0 + time_f * 5.0) * 0.01;
    vec2 distortion = vec2(distortionX, distortionY);
    float r = texture(textTexture, reflectedUV + distortion * 1.2).r;
    float g = texture(textTexture, reflectedUV + distortion * 0.8).g;
    float b = texture(textTexture, reflectedUV + distortion * 0.4).b;
    color = vec4(r, g, b, 1.0);
}
)";

const char *szShader2 = R"(#version 300 es
precision highp float;
in vec2 TexCoord;
out vec4 color;

uniform float time_f;
uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform vec4 iMouse;

float rand(vec2 co) {
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

void main(void) {
    vec2 tc = TexCoord;
    vec2 uv = tc;
    float rollSpeed = 0.2;
    uv.y = mod(uv.y + time_f * rollSpeed, 1.0);
    float bendAmplitude = 0.02;
    float bendFrequency = 10.0;
    uv.x += sin(uv.y * 3.1415 * bendFrequency + time_f * 2.0) * bendAmplitude;
    float jitterChance = rand(vec2(time_f, uv.y));
    if(jitterChance > 0.95) {
        float jitterAmount = (rand(vec2(time_f * 2.0, uv.y * 3.0)) - 0.5) * 0.1;
        uv.x += jitterAmount;
    }
    float chromaShift = 0.005;
    float r = texture(textTexture, uv + vec2(chromaShift, 0.0)).r;
    float g = texture(textTexture, uv).g;
    float b = texture(textTexture, uv - vec2(chromaShift, 0.0)).b;
    color = vec4(r, g, b, 1.0);
}
)";

const char *szTwist = R"(#version 300 es
precision highp float;
in vec2 TexCoord;
out vec4 color;
uniform sampler2D textTexture;
uniform float time_f;

void main(void) {
    vec2 tc = TexCoord;
    float rippleSpeed = 5.0;
    float rippleAmplitude = 0.03;
    float rippleWavelength = 10.0;
    float twistStrength = 1.0;
    float radius = length(tc - vec2(0.5, 0.5));
    float ripple = sin(tc.x * rippleWavelength + time_f * rippleSpeed) * rippleAmplitude;
    ripple += sin(tc.y * rippleWavelength + time_f * rippleSpeed) * rippleAmplitude;
    vec2 rippleTC = tc + vec2(ripple, ripple);
    
    float angle = twistStrength * (radius - 1.0) + time_f;
    float cosA = cos(angle);
    float sinA = sin(angle);
    mat2 rotationMatrix = mat2(cosA, -sinA, sinA, cosA);
    vec2 twistedTC = (rotationMatrix * (tc - vec2(0.5, 0.5))) + vec2(0.5, 0.5);
    
    vec4 originalColor = texture(textTexture, tc);
    vec4 twistedRippleColor = texture(textTexture, mix(rippleTC, twistedTC, 0.5));
//    color = mix(originalColor, twistedRippleColor, 0.5);
    color = twistedRippleColor;
}
)";

const char *szTime = R"(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

float h1(float n){ return fract(sin(n)*43758.5453123); }
vec2 h2(float n){ return fract(sin(vec2(n, n+1.0))*vec2(43758.5453,22578.1459)); }

void main(void){
    vec2 tc = TexCoord;
    float rate = 0.6;
    float t = time_f * rate;
    float t0 = floor(t);
    float a = fract(t);
    float w = a*a*(3.0-2.0*a);
    vec2 p0 = vec2(0.15) + h2(t0)*0.7;
    vec2 p1 = vec2(0.15) + h2(t0+1.0)*0.7;
    vec2 center = mix(p0, p1, w);

    vec2 p = tc - center;
    float r = length(p);
    float ang = atan(p.y, p.x);

    float swirl = 2.2 + 0.8*sin(time_f*0.35);
    float spin = 0.6*sin(time_f*0.2);
    ang += swirl * r + spin;

    float bend = 0.35;
    float rp = r + bend * r * r;

    vec2 uv = center + vec2(cos(ang), sin(ang)) * rp;

    uv += 0.02 * vec2(sin((tc.y+time_f)*4.0), cos((tc.x-time_f)*3.5));

    uv = vec2(pingPong(uv.x, 1.0), pingPong(uv.y, 1.0));

    color = texture(textTexture, uv);
})";

const char *szBend = R"(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

void main(void) {
    vec2 tc = TexCoord;
    vec2 uv = tc;
    float t = mod(time_f, 10.0) * 0.1;
    float angle = sin(t * 3.14159265) * -0.5;
    vec2 center = vec2(0.5, 0.5);
    uv -= center;
    float dist = length(uv);
    float bend = sin(dist * 6.0 + t * 2.0 * 3.14159265) * 0.05;
    uv = mat2(cos(angle), -sin(angle), sin(angle), cos(angle)) * uv;
    uv += center;
    float time_t = pingPong(time_f, 10.0);
    uv -= sin(bend * uv * time_t);
    color = texture(textTexture, uv);
}
)";

const char *szPong = R"(#version 300 es
precision highp float;
out vec4 color;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;

float pingPong(float x, float len){
    float m = mod(x, len*2.0);
    return m <= len ? m : len*2.0 - m;
}

void main(void){
    vec2 tc = TexCoord;
    float aspect = iResolution.x / iResolution.y;
    vec2 ar = vec2(aspect, 1.0);
    vec2 p = (tc - 0.5) * ar;
    float base = 1.65;
    float period = log(base);
    float t = time_f * 0.45;
    float r = length(p) + 1e-6;
    float a = atan(p.y, p.x);
    float k = fract((log(r) - t) / period);
    float rw = exp(k * period);
    a += t * 0.4;
    vec2 z = vec2(cos(a), sin(a)) * rw;
    float swirl = sin(time_f * 0.6 + r * 5.0) * 0.1;
    z += swirl * normalize(p);
    vec2 uv = z / ar + 0.5;
    uv = fract(uv);
    vec4 tex = texture(textTexture, uv);
    color = tex;
}
)";

const char *psychWave = R"(#version 300 es
precision highp float;
in vec2 TexCoord;
out vec4 color;
uniform float time_f;
uniform sampler2D textTexture;
uniform vec2 iResolution;
void main(void)
{
    vec2 tc = TexCoord;
    vec2 normCoord = ((gl_FragCoord.xy / iResolution.xy) * 2.0 - 1.0) * vec2(iResolution.x / iResolution.y, 1.0);
    float distanceFromCenter = length(normCoord);
    float wave = sin(distanceFromCenter * 12.0 - time_f * 4.0);
    vec2 tcAdjusted = tc + (normCoord * 0.301 * wave);
    vec4 textureColor = texture(textTexture, tcAdjusted);
    color = textureColor;
}
)";

const char *crystalBall = R"(#version 300 es
precision highp float;
in vec2 TexCoord;
out vec4 color;
uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;

// Random functions for chaos
float rand(vec2 co) { return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453); }
float rand1(float x) { return fract(sin(x) * 43758.5453); } // Fixed name collision

vec3 brutalColorDistortion(vec3 col, vec2 uv, float t) {
    // Channel separation madness
    col.r = texture(textTexture, uv + vec2(sin(t*0.7)*0.03, cos(t*0.3)*0.02)).r;
    col.g = texture(textTexture, uv + vec2(sin(t*0.5)*0.02, cos(t*0.4)*0.03)).g;
    col.b = texture(textTexture, uv + vec2(sin(t*0.9)*0.04, cos(t*0.6)*0.01)).b;
    
    // Random color channel swaps
    if (mod(t, 3.0) > 2.0) col = col.bgr;
    if (mod(t*0.7, 2.0) > 1.3) col = col.grb;
    
    return col;
}

void main(void) {
    vec2 tc = TexCoord;
    vec2 uv = tc;
    float t = time_f * 3.0;
    
    // Extreme geometric distortion
    uv.x += sin(uv.y * 50.0 + t * 10.0) * 0.1 * rand1(t);
    uv.y += cos(uv.x * 40.0 + t * 8.0) * 0.08 * rand1(t+0.3);
    
    // Tape stretch apocalypse
    uv.x += sin(t * 5.0 + uv.y * 50.0) * 0.05;
    uv.y += cos(t * 3.0 + uv.x * 60.0) * 0.03;
    
    // Violent vertical/horizontal jitter
    uv += vec2(rand1(t) - 0.5, rand1(t+0.5) - 0.5) * 0.1;
    
    // CRT curvature distortion
    vec2 crtUV = uv * 2.0 - 1.0;
    crtUV *= 1.0 + pow(length(crtUV), 3.0) * 0.5;
    uv = crtUV * 0.5 + 0.5;
    
    // Severe tracking errors (split screen)
    float split = step(0.5 + sin(t)*0.2, uv.x);
    uv.y += split * (sin(t*2.0) * 0.1 * rand1(t));
    
    // Burning VCR head effect
    float scanLine = fract(t * 0.3);
    if(abs(uv.y - scanLine) < 0.02) {
        uv.x += (rand1(uv.y + t) - 0.5) * 0.3;
        uv.y += sin(uv.x * 100.0) * 0.1;
    }
    
    // Total chromatic breakdown (FIXED texture sampling)
    vec3 col = brutalColorDistortion(texture(textTexture, uv).rgb, uv, t);
    
    // Tape damage zones (FIXED missing parenthesis)
    float damageZone = step(0.9, rand(vec2(floor(uv.y * 20.0 + t * 0.5))));
    col = mix(col, vec3(0.0), damageZone * 0.7);
    
    // Static electricity bursts
    float staticFlash = step(0.997, rand1(t * 0.1));
    col += vec3(staticFlash * 2.0);
    
    // Magnetic interference waves
    col *= 0.7 + 0.3 * sin(uv.x * 300.0 + t * 10.0);
    
    // Extreme noise pollution
    vec3 noise = vec3(rand(uv * t), rand(uv * t + 0.3), rand(uv * t + 0.7)) * 0.8;
    col += noise * smoothstep(0.3, 0.7, rand1(t * 0.3));
    
    // Tape crease distortion
    float crease = sin(uv.y * 30.0 + t * 5.0) * 0.1;
    col *= 1.0 - smoothstep(0.3, 0.7, abs(crease));
    
    // Overload red channel
    col.r += sin(t * 5.0) * 0.3 + rand1(uv.x) * 0.2;
    
    // VCR menu burn-through
    float osd = step(0.98, rand1(uv.x * 10.0 + t * 0.1));
    col = mix(col, vec3(0.0, 1.0, 0.0), osd * 0.3);
    
    // Final output with scanline darkness
    color = vec4(col, 1.0);
    color *= 0.8 + 0.2 * sin(uv.y * 800.0 + t * 10.0);
    
    // Add random full-screen flicker
    color *= 0.7 + 0.3 * rand1(t * 0.1);
    
    // Edge collapse
    color *= smoothstep(0.0, 0.2, uv.y) * smoothstep(1.0, 0.8, uv.y);
    color *= smoothstep(0.0, 0.1, uv.x) * smoothstep(1.0, 0.9, uv.x);
}
)";

class About : public gl::GLObject {
    GLuint texture = 0;
    gl::ShaderProgram shader;
    float animation = 0.0f;
public:
    

    About() = default;
    virtual ~About() override {

        if(texture != 0) {
            glDeleteTextures(1, &texture);
        }
    }

    void load(gl::GLWindow *win) override {
        texture = gl::loadTexture(win->util.getFilePath("data/jared.png"));
        if(texture == 0) {
            throw mx::Exception("Error loading texture");
        }
        if(!shader.loadProgramFromText(gl::vSource, crystalBall)) {
            throw mx::Exception("Error loading texture");
        }
        shader.useProgram();
        shader.setUniform("alpha", 1.0f);
        shader.setUniform("iResolution", glm::vec2(win->w, win->h));
        //shader.setUniform("iMouse", mouse);
        sprite.initSize(win->w, win->h);
        sprite.initWithTexture(&shader, texture, 0, 0, win->w, win->h);
    }

    void draw(gl::GLWindow *win) override {
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; // Convert to seconds
        lastUpdateTime = currentTime;
        animation += deltaTime;
        shader.useProgram();
        shader.setUniform("time_f", animation);
        //shader.setUniform("iMouse", mouse);
        update(deltaTime);
        sprite.draw();
    }

    bool mouseDown = false;
    glm::vec4 mouse = glm::vec4(0.0f);

    void event(gl::GLWindow *win, SDL_Event &e) override {
        switch(e.type) {
            case SDL_MOUSEBUTTONDOWN:
                if(e.button.button == SDL_BUTTON_LEFT) {
                    mouseDown = true;
                    mouse.x = static_cast<float>(e.button.x);
                    mouse.y = static_cast<float>(e.button.y);
                    mouse.z = 1.0f;  
                    mouse.w = 1.0f;  
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if(e.button.button == SDL_BUTTON_LEFT) {
                    mouseDown = false;
                    mouse.z = 0.0f;  
                    mouse.w = 0.0f;
                }
                break;
            case SDL_MOUSEMOTION:
                mouse.x = static_cast<float>(e.motion.x);
                mouse.y = static_cast<float>(e.motion.y);
                if(mouseDown) {
                    mouse.z = 1.0f;
                }
                break;
        }
    }
    void update(float deltaTime) {}
    
private:
    Uint32 lastUpdateTime = SDL_GetTicks();
    gl::GLSprite sprite;
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("About", tw, th) {
        setPath(path);
        setObject(new About());
        object->load(this);
    }
    
    ~MainWindow() override {}
    
    virtual void event(SDL_Event &e) override {
        object->event(this, e);
    }
    
    virtual void draw() override {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glViewport(0, 0, w, h);
        object->draw(this);
        swap();
        delay();
    }
};

MainWindow *main_w = nullptr;

void eventProc() {
    main_w->proc();
}

int main(int argc, char **argv) {
#ifdef __EMSCRIPTEN__
    try {
    MainWindow main_window("/", 960, 720);
    main_w =&main_window;
    emscripten_set_main_loop(eventProc, 0, 1);
    } catch (mx::Exception &e) {
        std::cerr << e.text() << "\n";
        return EXIT_FAILURE;
    }
#else
    Argz<std::string> parser(argc, argv);
    parser.addOptionSingle('h', "Display help message")
        .addOptionSingleValue('p', "assets path")
        .addOptionDoubleValue('P', "path", "assets path")
        .addOptionSingleValue('r',"Resolution WidthxHeight")
        .addOptionDoubleValue('R',"resolution", "Resolution WidthxHeight");
    Argument<std::string> arg;
    std::string path;
    int value = 0;
    int tw = 960, th = 720;
    try {
        while((value = parser.proc(arg)) != -1) {
            switch(value) {
                case 'h':
                case 'v':
                    parser.help(std::cout);
                    exit(EXIT_SUCCESS);
                    break;
                case 'p':
                case 'P':
                    path = arg.arg_value;
                    break;
                case 'r':
                case 'R': {
                    auto pos = arg.arg_value.find("x");
                    if(pos == std::string::npos)  {
                        mx::system_err << "Error invalid resolution use WidthxHeight\n";
                        mx::system_err.flush();
                        exit(EXIT_FAILURE);
                    }
                    std::string left, right;
                    left = arg.arg_value.substr(0, pos);
                    right = arg.arg_value.substr(pos+1);
                    tw = atoi(left.c_str());
                    th = atoi(right.c_str());
                }
                    break;
            }
        }
    } catch (const ArgException<std::string>& e) {
        mx::system_err << e.text() << "\n";
    }
    if(path.empty()) {
        mx::system_out << "mx: No path provided trying default current directory.\n";
        path = ".";
    }
    try {
        MainWindow main_window(path, tw, th);
        main_window.loop();
    } catch(const mx::Exception &e) {
        mx::system_err << "mx: Exception: " << e.text() << "\n";
        mx::system_err.flush();
        exit(EXIT_FAILURE);
    }
#endif
    return 0;
}