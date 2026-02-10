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

// Bubble distortion effect applied to UV coordinates
vec2 applyBubbleDistort(vec2 uv) {
    float time_f = ubo.params.x;
    vec2 centered = uv * 2.0 - 1.0;
    float len = length(centered);
    float time_t = pingPong(time_f, 10.0);
    vec2 distort = centered * (1.0 + 0.1 * sin(time_f + len * 20.0));
    distort = sin(distort * time_t);
    return distort * 0.5 + 0.5;
}

float bubbleMix(vec2 uv) {
    float time_f = ubo.params.x;
    vec2 centered = uv * 2.0 - 1.0;
    float len = length(centered);
    float time_t = pingPong(time_f, 10.0);
    float bubble = smoothstep(0.8, 1.0, 1.0 - len);
    return sin(bubble * time_t);
}

vec3 sampleWithBubble(vec2 uv) {
    bool useBubble = ubo.params.y > 0.5;
    if (!useBubble) {
        return texture(texSampler, uv).rgb;
    }
    vec2 distUV = applyBubbleDistort(uv);
    vec3 texColor = texture(texSampler, distUV).rgb;
    float b = bubbleMix(uv);
    return mix(texColor, vec3(1.0), b);
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
            vec3 texColor = sampleWithBubble(texUV);
            
            // Apply side shading
            if (side == 1) texColor *= 0.7;
            
            col = texColor;
        } else if (y < drawStart) {
            float currentDist = screenH / (screenH - 2.0 * y);
            
            float floorX = posX + currentDist * rayDirX;
            float floorY = posY + currentDist * rayDirY;
            
            // Map matrix rain texture to ceiling (flipped)
            vec2 ceilUV = vec2(fract(floorX), 1.0 - fract(floorY));
            vec3 texColor = sampleWithBubble(ceilUV);
            
            // Apply distance fog
            col = texColor * (1.0 / (1.0 + currentDist * currentDist * 0.02));
        } else {
            float currentDist = screenH / (2.0 * y - screenH);
            
            float floorX = posX + currentDist * rayDirX;
            float floorY = posY + currentDist * rayDirY;
            
            // Map matrix rain texture to floor (flipped)
            vec2 floorUV = vec2(fract(floorX), 1.0 - fract(floorY));
            vec3 texColor = sampleWithBubble(floorUV);
            
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
            vec3 texColor = sampleWithBubble(ceilUV);
            col = texColor * (1.0 / (1.0 + currentDist * currentDist * 0.02));
        } else {
            float currentDist = screenH / (2.0 * y - screenH);
            float floorX = posX + currentDist * rayDirX;
            float floorY = posY + currentDist * rayDirY;
            
            // Map matrix rain texture to floor (flipped)
            vec2 floorUV = vec2(fract(floorX), 1.0 - fract(floorY));
            vec3 texColor = sampleWithBubble(floorUV);
            col = texColor * (1.0 / (1.0 + currentDist * currentDist * 0.02));
        }
    }
    
    outColor = vec4(col, 1.0);
}
