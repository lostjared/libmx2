#include "vk.hpp"
#include "SDL.h"
#include <cmath>
#include <format>
#include <random>
#include <deque>
#include <unordered_map>
#include <memory>
#include <opencv2/opencv.hpp>
#include "argz.hpp"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

float generateRandomFloat(float min, float max) {
    static std::random_device rd;
    static std::default_random_engine eng(rd());
    std::uniform_real_distribution<float> dist(min, max);
    return dist(eng);
}

template<typename T>
T getRand(T min, T max) {
    static std::random_device rd;
    static std::default_random_engine eng(rd());
    if constexpr (std::is_floating_point_v<T>) {
        std::uniform_real_distribution<T> dist(min, max);
        return dist(eng);
    } else {
        std::uniform_int_distribution<T> dist(min, max);
        return dist(eng);
    }
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

struct GlyphKey {
    int codepoint;
    uint32_t color; 

    bool operator==(const GlyphKey& other) const {
        return codepoint == other.codepoint && color == other.color;
    }
};

struct GlyphHasher {
    std::size_t operator()(const GlyphKey& k) const {
        return std::hash<int>{}(k.codepoint) ^ (std::hash<uint32_t>{}(k.color) << 1);
    }
};

class GlyphCache {
public:
    struct GlyphData {
        std::vector<uint32_t> pixels;
        int w, h;
    };

    GlyphCache(TTF_Font* font) : font(font) {}

    void draw(int codepoint, SDL_Color color, uint32_t* destBuffer, int destX, int destY, int destWidth, int destHeight) {
        color.a = (color.a / 10) * 10;
        uint32_t colorKey = (color.r << 24) | (color.g << 16) | (color.b << 8) | color.a;
        GlyphKey key{codepoint, colorKey};
        auto it = cache.find(key);
        if (it == cache.end()) {
            it = cache.emplace(key, renderToPixels(codepoint, color)).first;
        }

        const GlyphData& glyph = it->second;
        for (int py = 0; py < glyph.h && (destY + py) < destHeight; ++py) {
            for (int px = 0; px < glyph.w && (destX + px) < destWidth; ++px) {
                if (destX + px >= 0 && destY + py >= 0) {
                    uint32_t pixel = glyph.pixels[py * glyph.w + px];
                    if ((pixel >> 24) & 0xFF) { // Simple alpha test
                        destBuffer[(destY + py) * destWidth + (destX + px)] = pixel;
                    }
                }
            }
        }
    }

private:
    TTF_Font* font;
    std::unordered_map<GlyphKey, GlyphData, GlyphHasher> cache;

    GlyphData renderToPixels(int codepoint, SDL_Color color) {
        std::string s = unicodeToUTF8(codepoint);
        SDL_Surface* surf = TTF_RenderUTF8_Blended(font, s.c_str(), color);
        if (!surf) return {{}, 0, 0};

        SDL_Surface* rgba = SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_RGBA32, 0);
        SDL_FreeSurface(surf);

        GlyphData data;
        data.w = rgba->w;
        data.h = rgba->h;
        data.pixels.resize(data.w * data.h);
        memcpy(data.pixels.data(), rgba->pixels, data.w * data.h * 4);

        SDL_FreeSurface(rgba);
        return data;
    }

    std::string unicodeToUTF8(int cp) {
        std::string res;
        if (cp <= 0x7F) res += (char)cp;
        else if (cp <= 0x7FF) { res += (char)((cp >> 6) | 0xC0); res += (char)((cp & 0x3F) | 0x80); }
        else if (cp <= 0xFFFF) { 
            res += (char)((cp >> 12) | 0xE0); 
            res += (char)(((cp >> 6) & 0x3F) | 0x80); 
            res += (char)((cp & 0x3F) | 0x80); 
        }
        return res;
    }
};

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
    std::string filename;
    cv::VideoCapture cap;
    bool draw_glyph = false;

    void setFile(const std::string &filen) {
        filename = filen;
        if(cap.open(filename)) {
            std::cout << "cv: Opened capture device.\n";
        }
    }
    
    std::unique_ptr<GlyphCache> glyphCache;

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
        float frameMoveSpeed = moveSpeed * (currentDeltaTime * 60.0f); 
        float frameRotSpeed = rotSpeed * (currentDeltaTime * 60.0f);
        if (keyW) {
            float newX = posX + dirX * frameMoveSpeed;
            float newY = posY + dirY * frameMoveSpeed;
            if (canMove(newX, posY)) posX = newX;
            if (canMove(posX, newY)) posY = newY;
        }
        if (keyS) {
            float newX = posX - dirX * frameMoveSpeed;
            float newY = posY - dirY * frameMoveSpeed;
            if (canMove(newX, posY)) posX = newX;
            if (canMove(posX, newY)) posY = newY;
        }
        
        if (keyA) {
            float newX = posX - planeX * frameMoveSpeed;
            float newY = posY - planeY * frameMoveSpeed;
            if (canMove(newX, posY)) posX = newX;
            if (canMove(posX, newY)) posY = newY;
        }
        if (keyD) {
            float newX = posX + planeX * frameMoveSpeed;
            float newY = posY + planeY * frameMoveSpeed;
            if (canMove(newX, posY)) posX = newX;
            if (canMove(posX, newY)) posY = newY;
        }
  
        if (keyLeft) {
            float oldDirX = dirX;
            dirX = dirX * cos(frameRotSpeed) - dirY * sin(frameRotSpeed);
            dirY = oldDirX * sin(frameRotSpeed) + dirY * cos(frameRotSpeed);
            
            float oldPlaneX = planeX;
            planeX = planeX * cos(frameRotSpeed) - planeY * sin(frameRotSpeed);
            planeY = oldPlaneX * sin(frameRotSpeed) + planeY * cos(frameRotSpeed);
        }
  
        if (keyRight) {
            float oldDirX = dirX;
            dirX = dirX * cos(-frameRotSpeed) - dirY * sin(-frameRotSpeed);
            dirY = oldDirX * sin(-frameRotSpeed) + dirY * cos(-frameRotSpeed);
            
            float oldPlaneX = planeX;
            planeX = planeX * cos(-frameRotSpeed) - planeY * sin(-frameRotSpeed);
            planeY = oldPlaneX * sin(-frameRotSpeed) + planeY * cos(-frameRotSpeed);
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
                case SDLK_SPACE: 
                    draw_glyph = !draw_glyph;
                break;
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
    std::vector<int> gridCodepoints;
    TTF_Font* matrixFont = nullptr;
    std::vector<uint32_t> texturePixels;
    
    void initMatrix() {
        std::string fontPath = util.getFilePath("data/keifont.ttf");
        matrixFont = TTF_OpenFont(fontPath.c_str(), 28);
        if (!matrixFont) {
            throw mx::Exception("Failed to load matrix font: " + fontPath);
        }
        glyphCache = std::make_unique<GlyphCache>(matrixFont);
        numColumns = NUM_RAIN_COLUMNS;
        numRows = TEXTURE_HEIGHT / charHeight;
        charWidth = TEXTURE_WIDTH / numColumns;
        
        gridCodepoints.resize(numColumns * numRows);
        for (int i = 0; i < static_cast<int>(gridCodepoints.size()); ++i) {
            gridCodepoints[i] = getRandomCodepoint();
        }

        fallPositions.resize(numColumns);
        fallSpeeds.resize(numColumns);
        trailLengths.resize(numColumns);
        columnBrightness.resize(numColumns);
        isHighlightColumn.resize(numColumns);
        
        for (int i = 0; i < numColumns; ++i) {
            fallPositions[i] = static_cast<float>(getRand<int>(0, numRows));
            fallSpeeds[i] = generateRandomFloat(MIN_FALL_SPEED, MAX_FALL_SPEED);
            trailLengths[i] = getRand<int>(MIN_TRAIL_LENGTH, MAX_TRAIL_LENGTH);
            isHighlightColumn[i] = (getRand<int>(0, 14) == 0);
            columnBrightness[i] = generateRandomFloat(0.8f, 1.2f);
        }
        
        texturePixels.resize(TEXTURE_WIDTH * TEXTURE_HEIGHT);
    }
    int getRandomCodepoint() {
        static std::random_device rd;
        static std::default_random_engine eng(rd());
        std::uniform_int_distribution<size_t> rangeDist(0, codepointRanges.size() - 1);
        size_t rangeIndex = rangeDist(eng);
        auto [start, end] = codepointRanges[rangeIndex];
        std::uniform_int_distribution<int> cpDist(start, end);   
        return cpDist(eng);
    }
    
    void updateMatrixRain(float deltaTime) {
        memset(texturePixels.data(), 0, texturePixels.size() * sizeof(uint32_t)); 
        cv::Mat frame;
        if(!cap.read(frame)) {
            cap.set(cv::CAP_PROP_POS_FRAMES, 0);
            if(!cap.read(frame)) {
                quit();
                return;
            }
        }
        cv::Mat resized;
        cv::resize(frame, resized, cv::Size(TEXTURE_WIDTH, TEXTURE_HEIGHT));
        cv::cvtColor(resized, resized, cv::COLOR_BGR2RGBA);
        memcpy(texturePixels.data(), resized.data, TEXTURE_WIDTH * TEXTURE_HEIGHT * 4);
        if(draw_glyph) {
            for(int i = 0; i < 30; ++i) {
                int randIdx = getRand<int>(0, gridCodepoints.size() - 1);
                gridCodepoints[randIdx] = getRandomCodepoint();
            }

            for (int col = 0; col < numColumns; ++col) {
                fallPositions[col] -= fallSpeeds[col] * deltaTime;
                if (fallPositions[col] < -trailLengths[col]) { 
                    fallPositions[col] = (float)numRows; 
                    trailLengths[col] = getRand<int>(MIN_TRAIL_LENGTH, MAX_TRAIL_LENGTH);
                    isHighlightColumn[col] = (getRand<int>(0, 20) == 0); 
                    columnBrightness[col] = generateRandomFloat(0.7f, 1.3f);
                }
                
                for (int i = 0; i < trailLengths[col]; ++i) {
                    int row = static_cast<int>(fallPositions[col] + i + numRows) % numRows;    
                    int codepoint = gridCodepoints[row * numColumns + col];
                    SDL_Color color;
                    if (i == 0) {
                        color = {255, 255, 255, 255}; 
                    } else if (i < 3 && isHighlightColumn[col]) {
                        color = {140, 255, 140, 255};
                    } else { 
                        float intensity = 1.0f - (float)i / (float)trailLengths[col];
                        intensity = powf(intensity, 1.3f) * columnBrightness[col];
                        intensity = std::clamp(intensity, 0.0f, 1.0f);
                        uint8_t green = static_cast<uint8_t>(255 * intensity);
                        uint8_t red = static_cast<uint8_t>(30 * intensity);
                        uint8_t blue = static_cast<uint8_t>(30 * intensity);
                        uint8_t alpha = static_cast<uint8_t>(std::max(0.15f, intensity) * 255);
                        color = {red, green, blue, alpha};
                    }
                
                    glyphCache->draw(codepoint, color, texturePixels.data(), 
                                    col * charWidth, row * charHeight, 
                                    TEXTURE_WIDTH, TEXTURE_HEIGHT);
                }
            }
        }
        updateGPUTexture();
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
        window.setFile(args.filename);
        window.initVulkan();
        window.loop();
        window.cleanup();
    } catch (mx::Exception &e) {
        SDL_Log("mx: Exception: %s\n", e.text().c_str());
    }
    return EXIT_SUCCESS;
}
