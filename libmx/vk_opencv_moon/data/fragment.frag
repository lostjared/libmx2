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
    vec4 params;   // x=time, y=width, z=height, w=effectIndex (0=off,1=kaleidoscope,2=ripple/twist,3=rotate/warp,4=spiral,5=gravity/spiral,6=rotating/zoom,7=chromatic/barrel,8=bend/warp,9=bubble/distort)
    vec4 color;  
} ubo;

const float PI = 3.1415926535897932384626433832795;

// ---- Shared helpers ----

float pingPong(float x, float length) {
    float m = mod(x, length * 2.0);
    return m <= length ? m : length * 2.0 - m;
}

// ---- Effect 1: Kaleidoscope ----

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

vec3 preBlendColor(vec2 uv, vec2 iResolution) {
    vec3 tex = tentBlur3(uv, iResolution);
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

    vec3 baseCol = preBlendColor(tc, iResolution);

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

    vec2 u0 = fract(pwrap / ar + m);
    vec2 u1 = fract((pwrap * 1.045) / ar + m);
    vec2 u2 = fract((pwrap * 0.955) / ar + m);

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

// ---- Effect 2: Ripple Twist ----

vec4 rippleTwistEffect(vec2 tc, float time_f) {
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

    return texture(texSampler, mix(rippleTC, twistedTC, 0.5));
}

// ---- Effect 3: Rotate Warp ----

vec4 rotateWarpEffect(vec2 tc, float time_f) {
    float angle = atan(tc.y - 0.5, tc.x - 0.5);
    float modulatedTime = pingPong(time_f, 5.0);
    angle += modulatedTime;

    vec2 rotatedTC;
    rotatedTC.x = cos(angle) * (tc.x - 0.5) - sin(angle) * (tc.y - 0.5) + 0.5;
    rotatedTC.y = sin(angle) * (tc.x - 0.5) + cos(angle) * (tc.y - 0.5) + 0.5;

    vec2 warpedCoords;
    warpedCoords.x = pingPong(rotatedTC.x + time_f * 0.1, 1.0);
    warpedCoords.y = pingPong(rotatedTC.y + time_f * 0.1, 1.0);

    return texture(texSampler, warpedCoords);
}

// ---- Effect 4: Spiral ----

vec4 spiralEffect(vec2 tc, float time_f, vec2 iResolution) {
    vec2 centeredCoord = tc * 2.0 - vec2(1.0, 1.0);
    centeredCoord.x *= iResolution.x / iResolution.y;
    float angle = atan(centeredCoord.y, centeredCoord.x);
    float dist = length(centeredCoord);
    float spiralFactor = 5.0;
    angle += dist * spiralFactor + time_f;
    vec2 spiralCoord = vec2(cos(angle), sin(angle)) * dist;
    spiralCoord.x *= iResolution.y / iResolution.x;
    spiralCoord = (spiralCoord + vec2(1.0, 1.0)) / 2.0;
    return texture(texSampler, spiralCoord);
}

// ---- Effect 5: Gravity Spiral ----

vec4 gravitySpiralEffect(vec2 tc, float time_f) {
    vec2 uv = tc;
    float maxTime = pingPong(time_f, 10.0);
    float t = pingPong(time_f, maxTime);
    float gForceIntensity = 0.1;
    float waveFrequency = 10.0;
    float waveAmplitude = 0.03;
    float gravityPull = gForceIntensity * t * uv.y;
    float waveDistortion = waveAmplitude * sin(waveFrequency * uv.x + t * 2.0);

    vec2 centeredUV = uv - vec2(0.5, 0.5);
    float angle = t + length(centeredUV) * 4.0;
    float spiralAmount = 0.5 * (1.0 - uv.y);
    mat2 rotation = mat2(cos(angle), -sin(angle), sin(angle), cos(angle));
    vec2 spiraledUV = rotation * centeredUV * (1.0 + spiralAmount);
    vec2 distortedUV = spiraledUV + vec2(0.5, 0.5);
    distortedUV.y += gravityPull + waveDistortion;
    distortedUV = clamp(distortedUV, vec2(0.0), vec2(1.0));
    return texture(texSampler, distortedUV);
}

// ---- Effect 6: Rotating Zoom ----

vec4 rotatingZoomEffect(vec2 tc, float time_f) {
    vec2 uv = tc - vec2(0.5);
    float amplitude = sin(time_f) * 0.5 + 0.5;
    float angle = time_f + amplitude * 10.0;
    mat2 rotation = mat2(cos(angle), -sin(angle), sin(angle), cos(angle));
    vec2 rotatedUV = rotation * uv;
    return texture(texSampler, rotatedUV + vec2(0.5));
}

// ---- Effect 7: Chromatic Barrel ----

vec4 chromaticBarrelEffect(vec2 tc, vec2 iResolution) {
    float radius = 1.0;
    vec2 center = iResolution * 0.5;
    vec2 texCoord = tc * iResolution;
    vec2 delta = texCoord - center;
    float dist = length(delta);
    float maxRadius = min(iResolution.x, iResolution.y) * radius;

    if (dist < maxRadius) {
        float scaleFactor = 1.0 - pow(dist / maxRadius, 2.0);
        vec2 direction = normalize(delta);
        float offsetR = 0.015;
        float offsetG = 0.0;
        float offsetB = -0.015;

        vec2 texCoordR = center + delta * scaleFactor + direction * offsetR * maxRadius;
        vec2 texCoordG = center + delta * scaleFactor + direction * offsetG * maxRadius;
        vec2 texCoordB = center + delta * scaleFactor + direction * offsetB * maxRadius;

        texCoordR /= iResolution;
        texCoordG /= iResolution;
        texCoordB /= iResolution;
        texCoordR = clamp(texCoordR, 0.0, 1.0);
        texCoordG = clamp(texCoordG, 0.0, 1.0);
        texCoordB = clamp(texCoordB, 0.0, 1.0);
        float r = texture(texSampler, texCoordR).r;
        float g = texture(texSampler, texCoordG).g;
        float b = texture(texSampler, texCoordB).b;
        return vec4(r, g, b, 1.0);
    }

    vec2 newTexCoord = clamp(tc, 0.0, 1.0);
    return texture(texSampler, newTexCoord);
}

// ---- Effect 8: Bend Warp ----

vec4 bendWarpEffect(vec2 tc, float time_f) {
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
    return texture(texSampler, uv);
}

// ---- Effect 9: Bubble Distort ----

vec4 bubbleDistortEffect(vec2 tc, float time_f) {
    vec2 uv = tc * 2.0 - 1.0;
    float len = length(uv);
    float time_t = pingPong(time_f, 10.0);
    float bubble = smoothstep(0.8, 1.0, 1.0 - len);
    bubble = sin(bubble * time_t);

    vec2 distort = uv * (1.0 + 0.1 * sin(time_f + len * 20.0));
    distort = sin(distort * time_t);

    vec4 texColor = texture(texSampler, distort * 0.5 + 0.5);
    return mix(texColor, vec4(1.0, 1.0, 1.0, 1.0), bubble);
}

// ---- Main ----

void main() {
    float time_f = ubo.params.x;
    vec2 iResolution = ubo.params.yz;
    int effectIndex = int(ubo.params.w + 0.5);

    vec3 lightPos = vec3(0.0, 5.0, 0.0);
    vec3 lightDir = normalize(lightPos - fragWorldPos);
    vec3 normal = normalize(fragNormal);
    float ambientStrength = 0.8;
    vec3 ambient = ambientStrength * fragColor;
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * fragColor;
    float specularStrength = 0.8;
    vec3 viewDir = normalize(-fragWorldPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = specularStrength * spec * vec3(1.0, 1.0, 1.0);
    float dist = length(lightPos - fragWorldPos);
    float attenuation = 1.0 / (1.0 + 0.1 * dist + 0.01 * dist * dist);
    vec4 texColor;

    // Tri-planar mapping
    vec3 objN = normalize(transpose(mat3(ubo.model)) * normal);
    vec3 blend = abs(objN);
    blend = pow(blend, vec3(4.0));
    blend /= (blend.x + blend.y + blend.z);

    vec2 uvX = objN.yz * 0.5 + 0.5;
    vec2 uvY = objN.xz * 0.5 + 0.5;
    vec2 uvZ = objN.xy * 0.5 + 0.5;

    if (effectIndex == 1) {
        vec4 cX = kaleidoscopeEffect(uvX, time_f, iResolution);
        vec4 cY = kaleidoscopeEffect(uvY, time_f, iResolution);
        vec4 cZ = kaleidoscopeEffect(uvZ, time_f, iResolution);
        texColor = cX * blend.x + cY * blend.y + cZ * blend.z;
    } else if (effectIndex == 2) {
        vec4 cX = rippleTwistEffect(uvX, time_f);
        vec4 cY = rippleTwistEffect(uvY, time_f);
        vec4 cZ = rippleTwistEffect(uvZ, time_f);
        texColor = cX * blend.x + cY * blend.y + cZ * blend.z;
    } else if (effectIndex == 3) {
        vec4 cX = rotateWarpEffect(uvX, time_f);
        vec4 cY = rotateWarpEffect(uvY, time_f);
        vec4 cZ = rotateWarpEffect(uvZ, time_f);
        texColor = cX * blend.x + cY * blend.y + cZ * blend.z;
    } else if (effectIndex == 4) {
        vec4 cX = spiralEffect(uvX, time_f, iResolution);
        vec4 cY = spiralEffect(uvY, time_f, iResolution);
        vec4 cZ = spiralEffect(uvZ, time_f, iResolution);
        texColor = cX * blend.x + cY * blend.y + cZ * blend.z;
    } else if (effectIndex == 5) {
        vec4 cX = gravitySpiralEffect(uvX, time_f);
        vec4 cY = gravitySpiralEffect(uvY, time_f);
        vec4 cZ = gravitySpiralEffect(uvZ, time_f);
        texColor = cX * blend.x + cY * blend.y + cZ * blend.z;
    } else if (effectIndex == 6) {
        vec4 cX = rotatingZoomEffect(uvX, time_f);
        vec4 cY = rotatingZoomEffect(uvY, time_f);
        vec4 cZ = rotatingZoomEffect(uvZ, time_f);
        texColor = cX * blend.x + cY * blend.y + cZ * blend.z;
    } else if (effectIndex == 7) {
        vec4 cX = chromaticBarrelEffect(uvX, iResolution);
        vec4 cY = chromaticBarrelEffect(uvY, iResolution);
        vec4 cZ = chromaticBarrelEffect(uvZ, iResolution);
        texColor = cX * blend.x + cY * blend.y + cZ * blend.z;
    } else if (effectIndex == 8) {
        vec4 cX = bendWarpEffect(uvX, time_f);
        vec4 cY = bendWarpEffect(uvY, time_f);
        vec4 cZ = bendWarpEffect(uvZ, time_f);
        texColor = cX * blend.x + cY * blend.y + cZ * blend.z;
    } else if (effectIndex == 9) {
        vec4 cX = bubbleDistortEffect(uvX, time_f);
        vec4 cY = bubbleDistortEffect(uvY, time_f);
        vec4 cZ = bubbleDistortEffect(uvZ, time_f);
        texColor = cX * blend.x + cY * blend.y + cZ * blend.z;
    } else {
        vec4 cX = texture(texSampler, uvX);
        vec4 cY = texture(texSampler, uvY);
        vec4 cZ = texture(texSampler, uvZ);
        texColor = cX * blend.x + cY * blend.y + cZ * blend.z;
    }

    vec3 lighting = ambient + (diffuse + specular) * attenuation;
    vec3 result = texColor.rgb * lighting;
    outColor = vec4(result, texColor.a);
}
