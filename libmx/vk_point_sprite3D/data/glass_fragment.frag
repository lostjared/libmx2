#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D texSampler;
layout(binding = 1) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 params; // x=time, y=width, z=height, w=kaleidoscope toggle (0 or 1)
    vec4 color;
} ubo;

// ========== Kaleidoscope Helper Functions ==========
const float PI = 3.1415926535897932384626433832795;

float pingPong(float x, float len) {
    float m = mod(x, len * 2.0);
    return m <= len ? m : len * 2.0 - m;
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

vec3 tentBlur3(vec2 uv, vec2 res) {
    vec2 ts = 1.0 / res;
    vec3 s00 = texture(texSampler, uv + ts * vec2(-1.0, -1.0)).rgb;
    vec3 s10 = texture(texSampler, uv + ts * vec2( 0.0, -1.0)).rgb;
    vec3 s20 = texture(texSampler, uv + ts * vec2( 1.0, -1.0)).rgb;
    vec3 s01 = texture(texSampler, uv + ts * vec2(-1.0,  0.0)).rgb;
    vec3 s11 = texture(texSampler, uv).rgb;
    vec3 s21 = texture(texSampler, uv + ts * vec2( 1.0,  0.0)).rgb;
    vec3 s02 = texture(texSampler, uv + ts * vec2(-1.0,  1.0)).rgb;
    vec3 s12 = texture(texSampler, uv + ts * vec2( 0.0,  1.0)).rgb;
    vec3 s22 = texture(texSampler, uv + ts * vec2( 1.0,  1.0)).rgb;
    return (s00 + 2.0*s10 + s20 + 2.0*s01 + 4.0*s11 + 2.0*s21 + s02 + 2.0*s12 + s22) / 16.0;
}

// Mirror-fold UV so texture seamlessly reflects instead of wrapping with a seam
vec2 mirrorUV(vec2 uv) {
    return 1.0 - abs(1.0 - 2.0 * fract(uv * 0.5));
}

vec3 preBlendColor(vec2 uv, vec2 iResolution) {
    return tentBlur3(mirrorUV(uv), iResolution) * 1.2;
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

// ========== Kaleidoscope Main Logic ==========
vec4 kaleidoscopeEffect(vec2 tc, float time_f, vec2 iResolution) {
    vec2 xuv = 1.0 - abs(1.0 - 2.0 * tc);
    xuv = xuv - floor(xuv);

    vec4 baseTex = texture(texSampler, xuv);

    vec2 uv = tc * 2.0 - 1.0;
    float aspect = iResolution.x / iResolution.y;
    uv.x *= aspect;

    float r = pingPong(sin(length(uv) * time_f), 5.0);
    float radius = sqrt(aspect * aspect + 1.0) + 0.5;
    float glow = smoothstep(radius, radius - 0.5, r);

    vec2 m = vec2(0.5);
    vec2 ar = vec2(aspect, 1.0);

    float seg = 4.0 + 2.0 * sin(time_f * 0.33);
    vec2 kUV = reflectUV(tc, seg, m, aspect);
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

    vec2 u0 = mirrorUV(pwrap / ar + m);
    vec2 u1 = mirrorUV((pwrap * 1.045) / ar + m);
    vec2 u2 = mirrorUV((pwrap * 0.955) / ar + m);

    vec2 dir = normalize(pwrap + 1e-6);
    vec2 off = dir * (0.0015 + 0.001 * sin(time_f * 1.3)) * vec2(1.0, 1.0 / aspect);

    float vign = 1.0 - smoothstep(0.75, 1.2, length((tc - m) * ar));
    vign = mix(0.9, 1.15, vign);

    vec3 rC = preBlendColor(u0 + off, iResolution);
    vec3 gC = preBlendColor(u1, iResolution);
    vec3 bC = preBlendColor(u2 - off, iResolution);

    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);

    float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + time_f * 1.2));
    ring = ring * pingPong((time_f * PI), 5.0);

    float pulse = 0.5 + 0.5 * sin(time_f * 2.0 + rD * 28.0 + k * 12.0);

    vec3 outCol = kaleidoRGB;
    outCol *= (0.6 + 0.4 * pulse);
    outCol *= vign;
    outCol = clamp(outCol, vec3(0.0), vec3(1.0));

    vec3 finalRGB = mix(baseTex.rgb * 0.8, outCol, pingPong(glow * PI, 5.0) * 0.6);

    return vec4(finalRGB, baseTex.a);
}

// ========== Main ==========
void main() {
    float time = ubo.params.x;
    vec2 iResolution = vec2(ubo.params.y, ubo.params.z);
    float kaleidoToggle = ubo.params.w; // 0.0 = off, 1.0 = on

    // 1. Setup Top-Down Light
    vec3 lightPos = vec3(0.0, 5.0, 0.0); 
    vec3 lightDir = normalize(lightPos - fragWorldPos);
    vec3 normal = normalize(fragNormal);
    vec3 viewDir = normalize(-fragWorldPos);

    // 2. Bubble Effect (Expanding/Contracting Sphere)
    vec3 bubbleCenter = vec3(0.0, 0.5, 0.0);
    float distToBubbleCenter = length(fragWorldPos - bubbleCenter);
    
    float bubbleRadius = 1.0 + 0.3 * sin(time * 2.0);
    float distToBubbleSurface = abs(distToBubbleCenter - bubbleRadius);
    
    float bubbleThickness = 0.15;
    float insideBubble = smoothstep(bubbleThickness, 0.0, distToBubbleSurface);
    
    // 3. Underwater Refraction Distortion
    vec3 toCenter = normalize(fragWorldPos - bubbleCenter);
    
    float waviness = sin(toCenter.x * 8.0 + time * 3.0) * 0.5 + 
                     cos(toCenter.z * 8.0 + time * 2.5) * 0.5;
    
    float secondaryWave = sin(toCenter.y * 6.0 + time * 2.2) * 0.4 +
                          sin(toCenter.x * 5.0 - time * 1.8) * 0.3;
    
    float combinedWaviness = waviness * 0.6 + secondaryWave * 0.4;
    
    float refractionStrength = insideBubble * (1.0 - distToBubbleCenter / bubbleRadius) * 0.6;
    
    vec3 refractionNormal = toCenter * combinedWaviness * refractionStrength;
    vec3 distortedNormal = normalize(normal + refractionNormal * 0.8);
    
    // 4. Texture sampling - normal or kaleidoscope
    vec2 refractedTexCoord = fragTexCoord + refractionNormal.xy * 0.15;

    vec4 texColor;
    if (kaleidoToggle > 0.5) {
        // Kaleidoscope mode: apply fractal texture effect to the refracted coords
        texColor = kaleidoscopeEffect(refractedTexCoord, time, iResolution);
        
        // Apply chromatic aberration to kaleidoscope result
        vec4 chromaR = kaleidoscopeEffect(refractedTexCoord + vec2(0.06, 0.03), time, iResolution);
        vec4 chromaB = kaleidoscopeEffect(refractedTexCoord - vec2(0.06, -0.03), time, iResolution);
        vec3 chromaShift = vec3(chromaR.r, texColor.g, chromaB.b);
        texColor.rgb = mix(texColor.rgb, chromaShift, refractionStrength * 0.8);
    } else {
        // Normal bubble mode
        texColor = texture(texSampler, refractedTexCoord);
        vec3 chromaShift = vec3(
            texture(texSampler, refractedTexCoord + vec2(0.06, 0.03)).r,
            texture(texSampler, refractedTexCoord + vec2(0.0, 0.0)).g,
            texture(texSampler, refractedTexCoord - vec2(0.06, -0.03)).b
        );
        texColor.rgb = mix(texColor.rgb, chromaShift, refractionStrength * 0.8);
    }
    
    // 5. Lighting Calculations
    float ambientStrength = kaleidoToggle > 0.5 ? 0.7 : 0.3;
    vec3 ambient = ambientStrength * fragColor;

    float diff = max(dot(distortedNormal, lightDir), 0.0);
    vec3 diffuse = diff * fragColor;

    vec3 reflectDir = reflect(-lightDir, distortedNormal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64.0);
    vec3 specular = vec3(1.2) * spec * insideBubble;

    // 6. Fresnel & Bubble Surface Highlights
    float fresnel = pow(1.0 - max(dot(distortedNormal, viewDir), 0.0), 3.0);
    
    float causticsPattern = sin(distToBubbleCenter * 15.0 - time * 4.0) * 0.3;
    float bubbleSheen = insideBubble * max(0.0, causticsPattern) * 0.3;

    // 7. Final Composition
    vec3 baseLighting = ambient + (diffuse + specular);
    
    vec3 underwaterTint = vec3(0.5, 0.8, 1.0);
    vec3 result = (texColor.rgb * baseLighting) + (fresnel * underwaterTint * insideBubble) + bubbleSheen;
    
    result = mix(result, result * underwaterTint, insideBubble * 0.2);

    float alpha;
    if (kaleidoToggle > 0.5) {
        // Kaleidoscope mode: semi-transparent so you can see inside
        alpha = 0.25 + 0.15 * fresnel;
    } else {
        // Normal glass mode: transparent bubble
        alpha = ubo.color.a * (insideBubble * 0.6 + fresnel * 0.4);
    }

    outColor = vec4(result, alpha);
}