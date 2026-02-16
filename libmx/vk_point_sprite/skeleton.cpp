#include "vk.hpp"
#include "SDL.h"
#include <random>
#include <cmath>
#include "argz.hpp"


struct LightOrb {
    float x, y;           
    float vx, vy;         
    float r, g, b;        
    float brightness;     
    float size;           
    float pulsePhase;     
    float pulseSpeed;     
    float hue;            
    float hueSpeed;       
    float minBrightness;  
    float maxBrightness;  
    float strobeTimer;    
    float strobeInterval; 
    float sizeSpeed;      
    
    void update(float dt, int screenW, int screenH, std::mt19937& rng) {
        x += vx * dt;
        y += vy * dt;
        if (x < 0) { x = 0; vx = -vx * 0.9f; }
        if (x > screenW) { x = static_cast<float>(screenW); vx = -vx * 0.9f; }
        if (y < 0) { y = 0; vy = -vy * 0.9f; }
        if (y > screenH) { y = static_cast<float>(screenH); vy = -vy * 0.9f; }
        pulsePhase += pulseSpeed * dt;
        if (pulsePhase > 2.0f * M_PI) pulsePhase -= 2.0f * M_PI;
        float pulseFactor = 0.5f * (1.0f + std::sin(pulsePhase));  
        brightness = minBrightness + (maxBrightness - minBrightness) * pulseFactor;
        strobeTimer += dt;
        if (strobeTimer >= strobeInterval) {
            strobeTimer = 0.0f;
            static std::uniform_int_distribution<int> hueDist(0, 359);
            hue = static_cast<float>(hueDist(rng));
            updateColorFromHue();
        }
        if(dir) {
            size += sizeSpeed * dt;
            if(size >= 128.0f) {
                size = 128.0f;
                dir = false;
            } 
        } else {
            size -= sizeSpeed * dt;
            if(size <= 64.0f) {
                size = 64.0f;
                dir = true;
            }
        }
    }
    
    bool dir;  

    void updateColorFromHue() {
        
        float h = hue / 60.0f;
        float s = 1.0f;
        float v = 1.0f;
        
        int i = static_cast<int>(h) % 6;
        float f = h - static_cast<int>(h);
        float p = v * (1.0f - s);
        float q = v * (1.0f - s * f);
        float t = v * (1.0f - s * (1.0f - f));
        
        switch (i) {
            case 0: r = v; g = t; b = p; break;
            case 1: r = q; g = v; b = p; break;
            case 2: r = p; g = v; b = t; break;
            case 3: r = p; g = q; b = v; break;
            case 4: r = t; g = p; b = v; break;
            case 5: r = v; g = p; b = q; break;
        }
    }
};

class PointSpriteWindow : public mx::VKWindow {
public:
    PointSpriteWindow(const std::string& path, int wx, int wy, bool full) 
        : mx::VKWindow("-[ Vulkan Point Sprites - Star Light ]-", wx, wy, full) {
        setPath(path);
    }
    virtual ~PointSpriteWindow() {}

    void setFileName(const std::string &filename) {
        this->filename = filename;
    }
    
    void initVulkan() override {
        mx::VKWindow::initVulkan();
        
        background = createSprite(util.getFilePath("data/universe.png"),
            util.getFilePath("data/sprite_vert.spv"), 
            util.getFilePath("data/sprite_frag.spv")
        );

        initOrbs();
        createOrbSprite();  
    }

    void initOrbs() {
        std::uniform_real_distribution<float> posXDist(50.0f, static_cast<float>(w - 50));
        std::uniform_real_distribution<float> posYDist(50.0f, static_cast<float>(h - 50));
        std::uniform_real_distribution<float> velDist(-150.0f, 150.0f);
        std::uniform_real_distribution<float> sizeDist(20.0f, 60.0f);
        std::uniform_real_distribution<float> phaseDist(0.0f, 2.0f * M_PI);
        std::uniform_real_distribution<float> pulseSpeedDist(0.5f, 3.0f);
        std::uniform_real_distribution<float> minBrightDist(0.2f, 0.5f);   
        std::uniform_real_distribution<float> maxBrightDist(0.8f, 1.5f);
        std::uniform_int_distribution<int> hueDist(0, 359);
        std::uniform_real_distribution<float> strobeIntervalDist(1.0f, 3.0f);  
        std::uniform_real_distribution<float> strobeTimerDist(0.0f, 3.0f);     
        orbs.resize(NUM_ORBS);
        for (size_t i = 0; i < orbs.size(); ++i) {
            auto& orb = orbs[i];
            orb.x = posXDist(gen);
            orb.y = posYDist(gen);
            orb.vx = velDist(gen);
            orb.vy = velDist(gen);
            orb.size = sizeDist(gen);
            orb.hue = static_cast<float>(hueDist(gen));
            orb.hueSpeed = 0.0f;  
            orb.pulsePhase = phaseDist(gen);
            orb.pulseSpeed = pulseSpeedDist(gen);
            orb.minBrightness = minBrightDist(gen);
            orb.maxBrightness = maxBrightDist(gen);
            orb.strobeInterval = strobeIntervalDist(gen);
            orb.strobeTimer = strobeTimerDist(gen);
            orb.sizeSpeed = 60.0f;
            orb.dir = std::uniform_int_distribution<int>(0, 1)(gen) == 0;
            float pulseFactor = 0.5f * (1.0f + std::sin(orb.pulsePhase));
            orb.brightness = orb.minBrightness + (orb.maxBrightness - orb.minBrightness) * pulseFactor;
            orb.updateColorFromHue();
        }
        
        lastTime = SDL_GetTicks();
    }

    void createOrbSprite() {
        orbSprite = createSprite(util.path + "/data/mstar.png",
            util.getFilePath("data/sprite_vert.spv"), 
            util.getFilePath("data/sprite_fragment_frag.spv")
        );
        orbSprite->enableInstancing(
            1024, 
            util.getFilePath("data/sprite_instance_vertex_vert.spv"),
            util.getFilePath("data/sprite_instance_fragment_frag.spv")
        );
    }

    void proc() override {
        Uint32 currentTime = SDL_GetTicks();
        float dt = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;
        if (dt > 0.05f) dt = 0.05f;
        if(background) {
            background->setShaderParams(1.0f, 1.0f, 1.0f, 1.0f);
            background->drawSpriteRect(0, 0, getWidth(), getHeight());
        }
        for (auto& orb : orbs) {
            orb.update(dt, w, h, gen);
        }
        float currentTimeSec = static_cast<float>(currentTime) / 1000.0f;
        for (const auto& orb : orbs) {
            if (orbSprite) {
                orbSprite->setShaderParams(orb.r, orb.g, orb.b, currentTimeSec);
                float scale = orb.size / 64.0f;  
                orbSprite->drawSprite(
                    static_cast<int>(orb.x - orb.size / 2), 
                    static_cast<int>(orb.y - orb.size / 2),
                    scale, scale
                );
            }
        }
        
        if(hudEnabled) {
            printText("-[ Glowing Light Orbs - Point Sprites Demo ]-", 20, 20, {255, 255, 255, 255});
            std::string orbInfo = "Orbs: " + std::to_string(orbs.size()) + " | FPS: " + std::to_string(static_cast<int>(1.0f / std::max(dt, 0.001f)));
            printText(orbInfo, 20, 50, {200, 200, 200, 255});
            printText("Press SPACE to add orbs | Press C to clear | ESC to quit", 20, 80, {150, 150, 150, 255});
        }
    }

    bool hudEnabled = true;
    
    void event(SDL_Event& e) override {
        if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)) {
            quit();
            return;
        }
        if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.sym == SDLK_SPACE) {
                addRandomOrbs(50);
            }
            if (e.key.keysym.sym == SDLK_c) {
                orbs.clear();
                initOrbs();
            }
            if(e.key.keysym.sym == SDLK_v) {
                hudEnabled = !hudEnabled;
            }
        }
    
        if (e.type == SDL_MOUSEBUTTONDOWN) {
            int mx, my;
            SDL_GetMouseState(&mx, &my);
            addOrbAtPosition(static_cast<float>(mx), static_cast<float>(my));
        }
    }
    
    void addRandomOrbs(int count) {
        std::uniform_real_distribution<float> posXDist(50.0f, static_cast<float>(w - 50));
        std::uniform_real_distribution<float> posYDist(50.0f, static_cast<float>(h - 50));
        std::uniform_real_distribution<float> velDist(-150.0f, 150.0f);
        std::uniform_real_distribution<float> sizeDist(20.0f, 60.0f);
        std::uniform_real_distribution<float> phaseDist(0.0f, 2.0f * M_PI);
        std::uniform_real_distribution<float> pulseSpeedDist(0.5f, 3.0f);
        std::uniform_real_distribution<float> minBrightDist(0.2f, 0.5f);
        std::uniform_real_distribution<float> maxBrightDist(0.8f, 1.5f);
        std::uniform_int_distribution<int> hueDist(0, 359);
        std::uniform_real_distribution<float> strobeIntervalDist(1.0f, 3.0f);
        std::uniform_real_distribution<float> strobeTimerDist(0.0f, 3.0f);
        
        for (int i = 0; i < count; i++) {
            LightOrb orb;
            orb.x = posXDist(gen);
            orb.y = posYDist(gen);
            orb.vx = velDist(gen);
            orb.vy = velDist(gen);
            orb.size = sizeDist(gen);
            orb.dir = std::uniform_int_distribution<int>(0, 1)(gen) == 0;
            orb.hue = static_cast<float>(hueDist(gen));
            orb.hueSpeed = 0.0f;  
            orb.pulsePhase = phaseDist(gen);
            orb.pulseSpeed = pulseSpeedDist(gen);
            orb.minBrightness = minBrightDist(gen);
            orb.maxBrightness = maxBrightDist(gen);
            orb.strobeInterval = strobeIntervalDist(gen);
            orb.strobeTimer = strobeTimerDist(gen);
            orb.sizeSpeed = 60.0f;
            float pulseFactor = 0.5f * (1.0f + std::sin(orb.pulsePhase));
            orb.brightness = orb.minBrightness + (orb.maxBrightness - orb.minBrightness) * pulseFactor;
            orb.updateColorFromHue();
            orbs.push_back(orb);
        }
    }
    
    void addOrbAtPosition(float px, float py) {
        std::uniform_real_distribution<float> velDist(-100.0f, 100.0f);
        std::uniform_real_distribution<float> sizeDist(30.0f, 70.0f);
        std::uniform_int_distribution<int> hueDist(0, 359);
        std::uniform_real_distribution<float> phaseDist(0.0f, 2.0f * M_PI);
        std::uniform_real_distribution<float> pulseSpeedDist(1.0f, 4.0f);
        std::uniform_real_distribution<float> minBrightDist(0.2f, 0.5f);
        std::uniform_real_distribution<float> maxBrightDist(0.8f, 1.5f);
        std::uniform_real_distribution<float> strobeIntervalDist(1.0f, 3.0f);
        std::uniform_real_distribution<float> strobeTimerDist(0.0f, 3.0f);
        
        LightOrb orb;
        orb.x = px;
        orb.y = py;
        orb.vx = velDist(gen);
        orb.vy = velDist(gen);
        orb.size = sizeDist(gen);
        orb.hue = static_cast<float>(hueDist(gen));
        orb.hueSpeed = 0.0f;
        orb.pulsePhase = phaseDist(gen);
        orb.pulseSpeed = pulseSpeedDist(gen);
        orb.minBrightness = minBrightDist(gen);
        orb.maxBrightness = maxBrightDist(gen);
        orb.strobeInterval = strobeIntervalDist(gen);
        orb.strobeTimer = strobeTimerDist(gen);
        orb.sizeSpeed = 60.0f;
        orb.dir = std::uniform_int_distribution<int>(0, 1)(gen) == 0;
        float pulseFactor = 0.5f * (1.0f + std::sin(orb.pulsePhase));
        orb.brightness = orb.minBrightness + (orb.maxBrightness - orb.minBrightness) * pulseFactor;
        orb.updateColorFromHue();
        orbs.push_back(orb);
    }
    
private:
    static constexpr int NUM_ORBS = 50;
    std::vector<LightOrb> orbs;
    mx::VKSprite *orbSprite = nullptr;
    mx::VKSprite *background = nullptr;
    Uint32 lastTime = 0;
    std::string filename;
    std::random_device rd;
    std::mt19937 gen{rd()};
};

int main(int argc, char **argv) {
    Arguments args = proc_args(argc, argv);
    try {
        PointSpriteWindow window(args.path, args.width, args.height, args.fullscreen);
        window.initVulkan();
        window.loop();
        window.cleanup();
    } catch (mx::Exception &e) {
        SDL_Log("mx: Exception: %s\n", e.text().c_str());
    }
    return EXIT_SUCCESS;
}
