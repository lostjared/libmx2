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

const int MAP_WIDTH = 16;
const int MAP_HEIGHT = 16;

int getMap(int x, int y) {
    if (x <= 0 || x >= MAP_WIDTH-1 || y <= 0 || y >= MAP_HEIGHT-1) return 1;
    if ((x == 4 && y == 4) || (x == 4 && y == 11) ||
        (x == 11 && y == 4) || (x == 11 && y == 11)) return 2;
    
    if ((x == 7 && y == 7) || (x == 7 && y == 8) ||
        (x == 8 && y == 7) || (x == 8 && y == 8)) return 3;
    
    if (x == 4 && y == 7) return 4;
    if (x == 11 && y == 7) return 4;
    if (x == 7 && y == 4) return 5;
    if (x == 8 && y == 11) return 5;
    
    return 0;
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
    
    for (int i = 0; i < 64; i++) {
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
            
            // Simple matrix rain texture mapping (flip Y for correct rain direction)
            vec2 texUV = vec2(wallX, 1.0 - wallY);
            vec3 texColor = texture(texSampler, texUV).rgb;
            
            // Apply side shading
            if (side == 1) texColor *= 0.7;
            
            col = texColor;
        } else if (y < drawStart) {
            float currentDist = screenH / (screenH - 2.0 * y);
            
            float floorX = posX + currentDist * rayDirX;
            float floorY = posY + currentDist * rayDirY;
            
            // Map matrix rain texture to ceiling
            vec2 ceilUV = vec2(fract(floorX), fract(floorY));
            vec3 texColor = texture(texSampler, ceilUV).rgb;
            
            // Apply distance fog
            col = texColor * (1.0 / (1.0 + currentDist * currentDist * 0.02));
        } else {
            float currentDist = screenH / (2.0 * y - screenH);
            
            float floorX = posX + currentDist * rayDirX;
            float floorY = posY + currentDist * rayDirY;
            
            // Map matrix rain texture to floor
            vec2 floorUV = vec2(fract(floorX), fract(floorY));
            vec3 texColor = texture(texSampler, floorUV).rgb;
            
            // Apply distance fog
            col = texColor * (1.0 / (1.0 + currentDist * currentDist * 0.02));
        }
    } else {
        // No wall hit - show ceiling/floor based on y position
        if (y < screenH / 2.0) {
            float currentDist = screenH / (screenH - 2.0 * y);
            float floorX = posX + currentDist * rayDirX;
            float floorY = posY + currentDist * rayDirY;
            
            // Map matrix rain texture to ceiling
            vec2 ceilUV = vec2(fract(floorX), fract(floorY));
            vec3 texColor = texture(texSampler, ceilUV).rgb;
            col = texColor * (1.0 / (1.0 + currentDist * currentDist * 0.02));
        } else {
            float currentDist = screenH / (2.0 * y - screenH);
            float floorX = posX + currentDist * rayDirX;
            float floorY = posY + currentDist * rayDirY;
            
            // Map matrix rain texture to floor
            vec2 floorUV = vec2(fract(floorX), fract(floorY));
            vec3 texColor = texture(texSampler, floorUV).rgb;
            col = texColor * (1.0 / (1.0 + currentDist * currentDist * 0.02));
        }
    }
    
    outColor = vec4(col, 1.0);
}
