#include "vk.hpp"
#include "SDL.h"
#include <cmath>
#include <format>
#include <random>
#include <deque>
#include <unordered_map>

#if defined(__APPLE__) || defined(_WIN32) || defined(__linux__)
#include "argz.hpp"
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

float generateRandomFloat(float min, float max) {
    static std::random_device rd;
    static std::default_random_engine eng(rd());
    std::uniform_real_distribution<float> dist(min, max);
    return dist(eng);
}


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

class RaycastWindow;

class RaycastWindow : public mx::VKWindow {
public:
    float posX = 8.0f, posY = 2.0f;     
    float dirX = 0.0f, dirY = 1.0f;     
    float planeX = 0.66f, planeY = 0.0f; 
    float moveSpeed = 0.05f;
    float rotSpeed = 0.03f;
    
    
    bool keyW = false, keyS = false, keyA = false, keyD = false;
    bool keyLeft = false, keyRight = false;
    
    
    static constexpr int NUM_RAIN_COLUMNS = 100;
    static constexpr int MIN_TRAIL_LENGTH = 8;
    static constexpr int MAX_TRAIL_LENGTH = 28;
    static constexpr float MIN_FALL_SPEED = 8.0f;
    static constexpr float MAX_FALL_SPEED = 20.0f;
    static constexpr int TEXTURE_WIDTH = 800;
    static constexpr int TEXTURE_HEIGHT = 800;
    
    RaycastWindow(const std::string& path, int wx, int wy, bool full) 
        : mx::VKWindow("-[ Vulkan Raycaster ]-", wx, wy, full) {
        setPath(path);
        lastFrameTime = SDL_GetPerformanceCounter();
        
        codepointRanges = {
            {0x3041, 0x3096},  
            {0x30A0, 0x30FF},  
        };
    }
    
    virtual ~RaycastWindow() {
        cleanupMatrix();
    }
    
    void initVulkan() override {
        mx::VKWindow::initVulkan();
        initMatrix();
    }
    
    bool canMove(float newX, float newY) {
        float margin = 0.2f;
        return getMap(int(newX - margin), int(newY - margin)) == 0 &&
               getMap(int(newX + margin), int(newY - margin)) == 0 &&
               getMap(int(newX - margin), int(newY + margin)) == 0 &&
               getMap(int(newX + margin), int(newY + margin)) == 0;
    }
    
    void updatePlayer() {
        if (keyW) {
            float newX = posX + dirX * moveSpeed;
            float newY = posY + dirY * moveSpeed;
            if (canMove(newX, posY)) posX = newX;
            if (canMove(posX, newY)) posY = newY;
        }
        if (keyS) {
            float newX = posX - dirX * moveSpeed;
            float newY = posY - dirY * moveSpeed;
            if (canMove(newX, posY)) posX = newX;
            if (canMove(posX, newY)) posY = newY;
        }
        
        if (keyA) {
            float newX = posX - planeX * moveSpeed;
            float newY = posY - planeY * moveSpeed;
            if (canMove(newX, posY)) posX = newX;
            if (canMove(posX, newY)) posY = newY;
        }
        if (keyD) {
            float newX = posX + planeX * moveSpeed;
            float newY = posY + planeY * moveSpeed;
            if (canMove(newX, posY)) posX = newX;
            if (canMove(posX, newY)) posY = newY;
        }
        
        if (keyLeft) {
            float oldDirX = dirX;
            dirX = dirX * cos(rotSpeed) - dirY * sin(rotSpeed);
            dirY = oldDirX * sin(rotSpeed) + dirY * cos(rotSpeed);
            float oldPlaneX = planeX;
            planeX = planeX * cos(rotSpeed) - planeY * sin(rotSpeed);
            planeY = oldPlaneX * sin(rotSpeed) + planeY * cos(rotSpeed);
        }
        if (keyRight) {
            float oldDirX = dirX;
            dirX = dirX * cos(-rotSpeed) - dirY * sin(-rotSpeed);
            dirY = oldDirX * sin(-rotSpeed) + dirY * cos(-rotSpeed);
            float oldPlaneX = planeX;
            planeX = planeX * cos(-rotSpeed) - planeY * sin(-rotSpeed);
            planeY = oldPlaneX * sin(-rotSpeed) + planeY * cos(-rotSpeed);
        }
    }
    
    void proc() override {
        Uint64 currentTime = SDL_GetPerformanceCounter();
        float deltaTime = (currentTime - lastFrameTime) / (double)SDL_GetPerformanceFrequency();
        lastFrameTime = currentTime;
        if (deltaTime > 0.1f) deltaTime = 0.1f;
        currentDeltaTime = deltaTime;
        
        updatePlayer();
        
        
        updateMatrixRain(deltaTime);
        
        raycastPlayer.posX = posX;
        raycastPlayer.posY = posY;
        raycastPlayer.dirX = dirX;
        raycastPlayer.dirY = dirY;
        raycastPlayer.planeX = planeX;
        raycastPlayer.planeY = planeY;
        printText("WASD: Move  Arrow Keys: Turn  ESC: Quit", 10, 10, {255, 255, 255, 255});        
        std::string posStr = std::format("Pos: {}, {}", posX, posY);
        printText(posStr, 10, 40, {200, 200, 200, 255});
    }
    
    void event(SDL_Event& e) override {
        if (e.type == SDL_QUIT) {
            quit();
        }
        else if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_ESCAPE: quit(); break;
                case SDLK_w: keyW = true; break;
                case SDLK_s: keyS = true; break;
                case SDLK_a: keyA = true; break;
                case SDLK_d: keyD = true; break;
                case SDLK_LEFT: keyLeft = true; break;
                case SDLK_RIGHT: keyRight = true; break;
            }
        }
        else if (e.type == SDL_KEYUP) {
            switch (e.key.keysym.sym) {
                case SDLK_w: keyW = false; break;
                case SDLK_s: keyS = false; break;
                case SDLK_a: keyA = false; break;
                case SDLK_d: keyD = false; break;
                case SDLK_LEFT: keyLeft = false; break;
                case SDLK_RIGHT: keyRight = false; break;
            }
        }
    }

private:
    Uint64 lastFrameTime;
    float currentDeltaTime = 0.0f;
    
    
    int charWidth = 24;
    int charHeight = 32;
    int numColumns;
    int numRows;
    
    std::vector<float> fallPositions;
    std::vector<float> fallSpeeds;
    std::vector<int> trailLengths;
    std::vector<float> columnBrightness;
    std::vector<bool> isHighlightColumn;
    std::vector<std::pair<int, int>> codepointRanges;
    
    TTF_Font* matrixFont = nullptr;
    std::vector<uint32_t> texturePixels;
    
    void initMatrix() {
        
        std::string fontPath = util.getFilePath("data/keifont.ttf");
        matrixFont = TTF_OpenFont(fontPath.c_str(), 28);
        if (!matrixFont) {
            throw mx::Exception("Failed to load matrix font: " + fontPath);
        }
        
        
        numColumns = NUM_RAIN_COLUMNS;
        numRows = TEXTURE_HEIGHT / charHeight;
        charWidth = TEXTURE_WIDTH / numColumns;
        
        fallPositions.resize(numColumns);
        fallSpeeds.resize(numColumns);
        trailLengths.resize(numColumns);
        columnBrightness.resize(numColumns);
        isHighlightColumn.resize(numColumns);
        
        for (int i = 0; i < numColumns; ++i) {
            fallPositions[i] = static_cast<float>(rand() % numRows);
            fallSpeeds[i] = generateRandomFloat(MIN_FALL_SPEED, MAX_FALL_SPEED);
            trailLengths[i] = rand() % (MAX_TRAIL_LENGTH - MIN_TRAIL_LENGTH + 1) + MIN_TRAIL_LENGTH;
            isHighlightColumn[i] = (rand() % 15 == 0);
            columnBrightness[i] = generateRandomFloat(0.7f, 1.3f);
        }
        
        texturePixels.resize(TEXTURE_WIDTH * TEXTURE_HEIGHT);
    }
    
    std::string unicodeToUTF8(int codepoint) {
        std::string utf8;
        if (codepoint <= 0x7F) {
            utf8 += static_cast<char>(codepoint);
        } else if (codepoint <= 0x7FF) {
            utf8 += static_cast<char>((codepoint >> 6) | 0xC0);
            utf8 += static_cast<char>((codepoint & 0x3F) | 0x80);
        } else if (codepoint <= 0xFFFF) {
            utf8 += static_cast<char>((codepoint >> 12) | 0xE0);
            utf8 += static_cast<char>(((codepoint >> 6) & 0x3F) | 0x80);
            utf8 += static_cast<char>((codepoint & 0x3F) | 0x80);
        }
        return utf8;
    }
    
    int getRandomCodepoint() {
        int rangeIndex = rand() % codepointRanges.size();
        int start = codepointRanges[rangeIndex].first;
        int end = codepointRanges[rangeIndex].second;
        return start + rand() % (end - start + 1);
    }
    
    void updateMatrixRain(float deltaTime) {
        
        memset(texturePixels.data(), 0, texturePixels.size() * sizeof(uint32_t));
        
        
        for (int col = 0; col < numColumns; ++col) {
            
            if (rand() % 100 == 0) {
                fallSpeeds[col] = generateRandomFloat(MIN_FALL_SPEED, MAX_FALL_SPEED);
            }
            
            fallPositions[col] -= fallSpeeds[col] * deltaTime;
            
            
            if (fallPositions[col] < 0) {
                fallPositions[col] += numRows;
                trailLengths[col] = rand() % (MAX_TRAIL_LENGTH - MIN_TRAIL_LENGTH + 1) + MIN_TRAIL_LENGTH;
                isHighlightColumn[col] = (rand() % 15 == 0);
                columnBrightness[col] = generateRandomFloat(0.7f, 1.3f);
            }
            
            
            for (int i = 0; i < trailLengths[col]; ++i) {
                int row = static_cast<int>(fallPositions[col] + i + numRows) % numRows;
                
                
                int codepoint = getRandomCodepoint();
                std::string charStr = unicodeToUTF8(codepoint);
                
                
                SDL_Color color;
                if (i == 0) {
                    
                    if (isHighlightColumn[col]) {
                        color = {255, 255, 255, 255};
                    } else {
                        color = {180, 255, 180, 255};
                    }
                } else {
                    
                    float intensity = 1.0f - (float)i / (float)trailLengths[col];
                    intensity = powf(intensity, 1.5f) * columnBrightness[col];
                    if (intensity < 0.0f) intensity = 0.0f;
                    if (intensity > 1.0f) intensity = 1.0f;
                    
                    uint8_t green = static_cast<uint8_t>(255 * intensity);
                    uint8_t red = static_cast<uint8_t>(50 * intensity);
                    uint8_t alpha = static_cast<uint8_t>(255 * intensity);
                    if (alpha < 30) alpha = 30;
                    
                    color = {red, green, 0, alpha};
                }
                
                
                renderCharToTexture(charStr, col * charWidth, row * charHeight, color);
            }
        }
        
        
        updateGPUTexture();
    }
    
    void renderCharToTexture(const std::string& charStr, int x, int y, SDL_Color color) {
        if (!matrixFont) return;
        
        SDL_Surface* charSurface = TTF_RenderUTF8_Blended(matrixFont, charStr.c_str(), color);
        if (!charSurface) {
            charSurface = TTF_RenderUTF8_Blended(matrixFont, "#", color);
            if (!charSurface) return;
        }
        
        
        SDL_Surface* rgbaSurface = SDL_ConvertSurfaceFormat(charSurface, SDL_PIXELFORMAT_RGBA32, 0);
        SDL_FreeSurface(charSurface);
        if (!rgbaSurface) return;
        
        if (SDL_MUSTLOCK(rgbaSurface)) {
            SDL_LockSurface(rgbaSurface);
        }
        
        int pitchInPixels = rgbaSurface->pitch / 4;
        uint32_t* srcPixels = (uint32_t*)rgbaSurface->pixels;
        
        for (int py = 0; py < rgbaSurface->h && (y + py) < TEXTURE_HEIGHT; ++py) {
            for (int px = 0; px < rgbaSurface->w && (x + px) < TEXTURE_WIDTH; ++px) {
                if (x + px >= 0 && y + py >= 0) {
                    int srcIdx = py * pitchInPixels + px;
                    int dstIdx = (y + py) * TEXTURE_WIDTH + (x + px);
                    
                    uint32_t srcPixel = srcPixels[srcIdx];
                    uint8_t srcAlpha = (srcPixel >> 24) & 0xFF;
                    
                    if (srcAlpha > 0) {
                        texturePixels[dstIdx] = srcPixel;
                    }
                }
            }
        }
        
        if (SDL_MUSTLOCK(rgbaSurface)) {
            SDL_UnlockSurface(rgbaSurface);
        }
        SDL_FreeSurface(rgbaSurface);
    }
    
    void updateGPUTexture() {
        
        VkDeviceSize imageSize = TEXTURE_WIDTH * TEXTURE_HEIGHT * 4;
        updateTexture(texturePixels.data(), imageSize);
    }
    
    void cleanupMatrix() {
        if (matrixFont) {
            TTF_CloseFont(matrixFont);
            matrixFont = nullptr;
        }
    }
};

int main(int argc, char **argv) {
    Arguments args = proc_args(argc, argv);
    try {
        RaycastWindow window(args.path, args.width, args.height, args.fullscreen);
        window.initVulkan();
        window.loop();
        window.cleanup();
    } catch (mx::Exception &e) {
        SDL_Log("mx: Exception: %s\n", e.text().c_str());
    }
    return EXIT_SUCCESS;
}
