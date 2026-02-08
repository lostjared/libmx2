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
            
            if (wallType == 1) {
                vec2 uv = vec2(wallX, wallY) * 2.0 - 1.0;
                float len = length(uv);
                float time_f = ubo.params.x;
                float bubble = smoothstep(0.8, 1.0, 1.0 - len);
                vec2 distort = uv * (1.0 + 0.1 * sin(time_f + len * 20.0));
                vec2 distortedCoord = distort * 0.5 + 0.5;
                
                vec3 texColor = texture(texSampler, distortedCoord).rgb;
                col = mix(texColor, vec3(1.0, 1.0, 1.0), bubble);
                
                if (side == 1) col *= 0.7;
            } else {
                vec2 uv = vec2(wallX, wallY) * 2.0 - 1.0;
                float len = length(uv);
                float time_f = ubo.params.x;
                float bubble = smoothstep(0.8, 1.0, 1.0 - len);
                vec2 distort = uv * (1.0 + 0.1 * sin(time_f + len * 20.0));
                vec2 distortedCoord = distort * 0.5 + 0.5;
                
                vec3 baseCol = getWallColor(wallType, side);
                vec3 texColor = texture(texSampler, distortedCoord).rgb;
                col = mix(baseCol * texColor, baseCol + vec3(0.2), bubble);
                
                if (side == 1) col *= 0.7;
            }
        } else if (y < drawStart) {
            float currentDist = screenH / (screenH - 2.0 * y);
            
            float floorX = posX + currentDist * rayDirX;
            float floorY = posY + currentDist * rayDirY;
            
            vec2 ceilUV = vec2(fract(floorX), fract(floorY));
            float time_f = ubo.params.x;
            vec2 uv_c = 1.0 - abs(1.0 - 2.0 * ceilUV);
            uv_c = uv_c - floor(uv_c);
            float len_c = length(uv_c);
            float time_tc = pingPong(time_f, 10.0);
            float bubble_c = smoothstep(0.8, 1.0, 1.0 - len_c);
            bubble_c = sin(bubble_c * time_tc);
            vec2 distort_c = uv_c * (1.0 + 0.1 * sin(time_f + len_c * 20.0));
            distort_c = sin(distort_c * time_tc);
            vec3 texColor_c = texture(texSampler, distort_c * 0.5 + 0.5).rgb;
            col = mix(texColor_c, vec3(0.7, 0.7, 0.9), bubble_c);
            
            col *= 1.0 / (1.0 + currentDist * currentDist * 0.02);
        } else {
            float currentDist = screenH / (2.0 * y - screenH);
            
            float floorX = posX + currentDist * rayDirX;
            float floorY = posY + currentDist * rayDirY;
            
            vec2 floorUV = vec2(fract(floorX), fract(floorY));
            float time_f = ubo.params.x;
            vec2 uv = 1.0 - abs(1.0 - 2.0 * floorUV);
            uv = uv - floor(uv);
            float len = length(uv);
            float time_t = pingPong(time_f, 10.0);
            float bubble = smoothstep(0.8, 1.0, 1.0 - len);
            bubble = sin(bubble * time_t);
            vec2 distort = uv * (1.0 + 0.1 * sin(time_f + len * 20.0));
            distort = sin(distort * time_t);
            vec3 texColor = texture(floorSampler, distort * 0.5 + 0.5).rgb;
            col = mix(texColor, vec3(1.0, 1.0, 1.0), bubble);
            
            col *= 1.0 / (1.0 + currentDist * currentDist * 0.02);
        }
    } else {
        // No wall hit - show ceiling/floor based on y position
        if (y < screenH / 2.0) {
            float currentDist = screenH / (screenH - 2.0 * y);
            float floorX = posX + currentDist * rayDirX;
            float floorY = posY + currentDist * rayDirY;
            
            vec2 ceilUV2 = vec2(fract(floorX), fract(floorY));
            float time_f2 = ubo.params.x;
            vec2 uv_c2 = 1.0 - abs(1.0 - 2.0 * ceilUV2);
            uv_c2 = uv_c2 - floor(uv_c2);
            float len_c2 = length(uv_c2);
            float time_tc2 = pingPong(time_f2, 10.0);
            float bubble_c2 = smoothstep(0.8, 1.0, 1.0 - len_c2);
            bubble_c2 = sin(bubble_c2 * time_tc2);
            vec2 distort_c2 = uv_c2 * (1.0 + 0.1 * sin(time_f2 + len_c2 * 20.0));
            distort_c2 = sin(distort_c2 * time_tc2);
            vec3 texColor_c2 = texture(texSampler, distort_c2 * 0.5 + 0.5).rgb;
            col = mix(texColor_c2, vec3(0.7, 0.7, 0.9), bubble_c2);
            
            col *= 1.0 / (1.0 + currentDist * currentDist * 0.02);
        } else {
            float currentDist = screenH / (2.0 * y - screenH);
            float floorX = posX + currentDist * rayDirX;
            float floorY = posY + currentDist * rayDirY;
            int tileX = int(floor(floorX));
            int tileY = int(floor(floorY));
            int checker = (tileX + tileY) & 1;
            vec3 floorColor1 = vec3(0.35, 0.25, 0.15);
            vec3 floorColor2 = vec3(0.5, 0.4, 0.25);
            col = (checker == 0) ? floorColor1 : floorColor2;
            col *= 1.0 / (1.0 + currentDist * currentDist * 0.02);
        }
    }
    
    outColor = vec4(col, 1.0);
}
