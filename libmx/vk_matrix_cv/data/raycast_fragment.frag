#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D texSampler;
layout(binding = 2) uniform sampler2D floorSampler;

layout(binding = 1) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 params;
    vec4 color;
    vec4 playerPos;
    vec4 playerPlane;
} ubo;

const int MAP_WIDTH = 32;
const int MAP_HEIGHT = 32;

const int worldMap[1024] = int[](
    // row 0  — top boundary
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    // row 1
    1,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    // row 2
    1,0,2,0,0,2,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,2,0,0,0,2,0,1,
    // row 3
    1,0,0,0,0,0,0,0,0,0,2,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    // row 4
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,2,0,0,0,0,2,0,0,0,1,
    // row 5
    1,0,2,0,0,2,0,1,0,0,2,0,0,2,0,1,0,0,0,0,0,0,0,0,0,2,0,0,0,2,0,1,
    // row 6
    1,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    // row 7
    1,1,1,1,0,1,1,1,1,1,1,0,1,1,1,1,1,0,1,1,1,1,1,1,1,1,0,1,1,1,1,1,
    // row 8
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,1,
    // row 9
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,1,
    // row 10
    1,0,0,0,2,0,0,0,2,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,2,0,0,0,1,
    // row 11
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    // row 12
    1,0,0,0,2,0,0,0,2,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,2,0,0,0,1,
    // row 13
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,1,
    // row 14
    1,1,1,1,1,1,0,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,0,1,1,1,
    // row 15
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    // row 16
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    // row 17
    1,0,0,0,2,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,2,0,0,0,0,0,1,
    // row 18
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    // row 19
    1,0,0,0,2,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,2,0,0,0,0,0,1,
    // row 20
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    // row 21
    1,1,1,0,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,1,1,1,
    // row 22
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    // row 23
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    // row 24
    1,0,0,0,2,0,0,0,0,2,0,0,0,2,0,0,0,0,0,0,2,0,0,0,0,0,2,0,0,0,0,1,
    // row 25
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    // row 26
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    // row 27
    1,0,0,0,2,0,0,0,0,2,0,0,0,2,0,0,0,3,0,0,2,0,0,0,0,0,2,0,0,0,0,1,
    // row 28
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    // row 29
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    // row 30
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    // row 31 — bottom boundary
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
);

int getMap(int x, int y) {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) return 1;
    return worldMap[y * MAP_WIDTH + x];
}

vec3 getWallColor(int wallType, int side) {
    vec3 col;
    if (wallType == 1) col = vec3(0.6, 0.6, 0.6);
    else if (wallType == 2) col = vec3(0.8, 0.2, 0.2);
    else if (wallType == 3) col = vec3(0.2, 0.8, 0.2);
    else if (wallType == 4) col = vec3(0.2, 0.2, 0.8);
    else if (wallType == 5) col = vec3(0.8, 0.8, 0.2);
    else col = vec3(1.0, 1.0, 1.0);
    
    if (side == 1) col *= 0.7;
    
    return col;
}

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

const float PI = 3.1415926535897932384626433832795;

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
    vec2 dx = dFdx(uv);
    vec2 dy = dFdy(uv);
    vec3 s00 = textureGrad(texSampler, uv + ts * vec2(-1.0, -1.0), dx, dy).rgb;
    vec3 s10 = textureGrad(texSampler, uv + ts * vec2( 0.0, -1.0), dx, dy).rgb;
    vec3 s20 = textureGrad(texSampler, uv + ts * vec2( 1.0, -1.0), dx, dy).rgb;
    vec3 s01 = textureGrad(texSampler, uv + ts * vec2(-1.0,  0.0), dx, dy).rgb;
    vec3 s11 = textureGrad(texSampler, uv, dx, dy).rgb;
    vec3 s21 = textureGrad(texSampler, uv + ts * vec2( 1.0,  0.0), dx, dy).rgb;
    vec3 s02 = textureGrad(texSampler, uv + ts * vec2(-1.0,  1.0), dx, dy).rgb;
    vec3 s12 = textureGrad(texSampler, uv + ts * vec2( 0.0,  1.0), dx, dy).rgb;
    vec3 s22 = textureGrad(texSampler, uv + ts * vec2( 1.0,  1.0), dx, dy).rgb;
    return (s00 + 2.0*s10 + s20 + 2.0*s01 + 4.0*s11 + 2.0*s21 + s02 + 2.0*s12 + s22) / 16.0;
}

vec3 preBlendColor(vec2 uv, vec2 iRes) {
    vec3 tex = tentBlur3(uv, iRes);
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

vec3 computeBubbleKaleido(vec2 tc, vec2 iResolution, float time_f, vec3 sceneColor) {
    vec2 xuv = 1.0 - abs(1.0 - 2.0 * tc);
    xuv = xuv - floor(xuv);

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

    vec3 finalRGB = mix(sceneColor * 0.8, outCol, pingPong(glow * PI, 5.0) * 0.6);

    return finalRGB;
}

// Kaleidoscope applied per-tile when toggled on (K key -> params.z)
vec3 sampleWithKaleido(vec2 uv) {
    if (ubo.params.z < 0.5) {
        return texture(texSampler, uv).rgb;
    }
    float time_f = ubo.params.x;
    float screenW = ubo.playerPlane.z;
    float screenH = ubo.playerPlane.w;
    vec2 iResolution = vec2(screenW, screenH);
    float aspect = screenW / screenH;

    vec2 m = vec2(0.5);
    vec2 ar = vec2(aspect, 1.0);

    float seg = 4.0 + 2.0 * sin(time_f * 0.33);
    vec2 kUV = reflectUV(uv, seg, m, aspect);
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

    float vign = 1.0 - smoothstep(0.75, 1.2, length((uv - m) * ar));
    vign = mix(0.9, 1.15, vign);

    vec3 rC = preBlendColor(u0 + off, iResolution);
    vec3 gC = preBlendColor(u1, iResolution);
    vec3 bC = preBlendColor(u2 - off, iResolution);

    vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);

    float pulse = 0.5 + 0.5 * sin(time_f * 2.0 + rD * 28.0 + k * 12.0);

    vec3 outCol = kaleidoRGB;
    outCol *= (0.6 + 0.4 * pulse);
    outCol *= vign;
    outCol = clamp(outCol, vec3(0.0), vec3(1.0));

    // Blend kaleidoscope with original texture
    vec3 origTex = texture(texSampler, uv).rgb;
    vec2 centered = uv * 2.0 - 1.0;
    float glow = pingPong(sin(length(centered) * time_f), 5.0);
    float blendFactor = pingPong(glow * PI, 5.0) * 0.6;

    return mix(origTex * 0.8, outCol, blendFactor);
}

void main() {
    float screenW = ubo.playerPlane.z;
    float screenH = ubo.playerPlane.w;
    
    float x = fragTexCoord.x * screenW;
    float y = fragTexCoord.y * screenH;
    
    float posX = ubo.playerPos.x;
    float posY = ubo.playerPos.y;
    float dirX = ubo.playerPos.z;
    float dirY = ubo.playerPos.w;
    float planeX = ubo.playerPlane.x;
    float planeY = ubo.playerPlane.y;
    
    float cameraX = 2.0 * x / screenW - 1.0;
    float rayDirX = dirX + planeX * cameraX;
    float rayDirY = dirY + planeY * cameraX;
    
    int mapX = int(floor(posX));
    int mapY = int(floor(posY));
    
    float deltaDistX = (rayDirX == 0.0) ? 1e30 : abs(1.0 / rayDirX);
    float deltaDistY = (rayDirY == 0.0) ? 1e30 : abs(1.0 / rayDirY);
    
    float sideDistX, sideDistY;
    int stepX, stepY;
    if (rayDirX < 0.0) {
        stepX = -1;
        sideDistX = (posX - float(mapX)) * deltaDistX;
    } else {
        stepX = 1;
        sideDistX = (float(mapX) + 1.0 - posX) * deltaDistX;
    }
    if (rayDirY < 0.0) {
        stepY = -1;
        sideDistY = (posY - float(mapY)) * deltaDistY;
    } else {
        stepY = 1;
        sideDistY = (float(mapY) + 1.0 - posY) * deltaDistY;
    }
    
    int hit = 0;
    int side = 0;
    int wallType = 0;
    
    for (int i = 0; i < 128; i++) {
        if (sideDistX < sideDistY) {
            sideDistX += deltaDistX;
            mapX += stepX;
            side = 0;
        } else {
            sideDistY += deltaDistY;
            mapY += stepY;
            side = 1;
        }
        
        wallType = getMap(mapX, mapY);
        if (wallType > 0) {
            hit = 1;
            break;
        }
    }
    
    vec3 col;
    
    if (hit == 1) {
        float perpWallDist;
        if (side == 0) {
            perpWallDist = (float(mapX) - posX + (1.0 - float(stepX)) / 2.0) / rayDirX;
        } else {
            perpWallDist = (float(mapY) - posY + (1.0 - float(stepY)) / 2.0) / rayDirY;
        }
        
        float wallHeightFactor = 1.0;
        float lineHeight = (screenH / perpWallDist) * wallHeightFactor;
        
        float drawStart = -lineHeight / 2.0 + screenH / 2.0;
        float drawEnd = lineHeight / 2.0 + screenH / 2.0;
        
        if (y >= drawStart && y <= drawEnd) {
            float wallX;
            if (side == 0) {
                wallX = posY + perpWallDist * rayDirY;
            } else {
                wallX = posX + perpWallDist * rayDirX;
            }
            wallX -= floor(wallX);
            
            float wallY = (y - drawStart) / (drawEnd - drawStart);
            
            // Simple matrix rain texture mapping (flipped)
            vec2 texUV = vec2(wallX, wallY);
            vec3 texColor = sampleWithKaleido(texUV);
            
            // Apply side shading
            if (side == 1) texColor *= 0.7;
            
            col = texColor;
        } else if (y < drawStart) {
            float currentDist = screenH / (screenH - 2.0 * y);
            
            float floorX = posX + currentDist * rayDirX;
            float floorY = posY + currentDist * rayDirY;
            
            // Map matrix rain texture to ceiling (flipped)
            vec2 ceilUV = vec2(fract(floorX), 1.0 - fract(floorY));
            vec3 texColor = sampleWithKaleido(ceilUV);
            
            // Apply distance fog
            col = texColor * (1.0 / (1.0 + currentDist * currentDist * 0.02));
        } else {
            float currentDist = screenH / (2.0 * y - screenH);
            
            float floorX = posX + currentDist * rayDirX;
            float floorY = posY + currentDist * rayDirY;
            
            // Map matrix rain texture to floor (flipped)
            vec2 floorUV = vec2(fract(floorX), 1.0 - fract(floorY));
            vec3 texColor = sampleWithKaleido(floorUV);
            
            // Apply distance fog
            col = texColor * (1.0 / (1.0 + currentDist * currentDist * 0.02));
        }
    } else {
        // No wall hit - show ceiling/floor based on y position
        if (y < screenH / 2.0) {
            float currentDist = screenH / (screenH - 2.0 * y);
            float floorX = posX + currentDist * rayDirX;
            float floorY = posY + currentDist * rayDirY;
            
            // Map matrix rain texture to ceiling (flipped)
            vec2 ceilUV = vec2(fract(floorX), 1.0 - fract(floorY));
            vec3 texColor = sampleWithKaleido(ceilUV);
            col = texColor * (1.0 / (1.0 + currentDist * currentDist * 0.02));
        } else {
            float currentDist = screenH / (2.0 * y - screenH);
            float floorX = posX + currentDist * rayDirX;
            float floorY = posY + currentDist * rayDirY;
            
            // Map matrix rain texture to floor (flipped)
            vec2 floorUV = vec2(fract(floorX), 1.0 - fract(floorY));
            vec3 texColor = sampleWithKaleido(floorUV);
            col = texColor * (1.0 / (1.0 + currentDist * currentDist * 0.02));
        }
    }
    
    // Apply bubble effect as post-processing on the final scene
    if (ubo.params.y > 0.5) {
        float time_f = ubo.params.x;
        vec2 iResolution = vec2(screenW, screenH);
        col = computeBubbleKaleido(fragTexCoord, iResolution, time_f, col);
    }

    outColor = vec4(col, 1.0);
}
