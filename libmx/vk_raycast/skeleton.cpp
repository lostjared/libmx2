#include "vk.hpp"
#include "SDL.h"
#include <cmath>
#include <format>

#if defined(__APPLE__) || defined(_WIN32) || defined(__linux__)
#include "argz.hpp"
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


const int MAP_WIDTH  = 32;
const int MAP_HEIGHT = 32;

static const int worldMap[MAP_WIDTH * MAP_HEIGHT] = {
    
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    
    1,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    
    1,0,2,0,0,2,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,2,0,0,0,2,0,1,
    
    1,0,0,0,0,0,0,0,0,0,2,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,2,0,0,0,0,2,0,0,0,1,
    
    1,0,2,0,0,2,0,1,0,0,2,0,0,2,0,1,0,0,0,0,0,0,0,0,0,2,0,0,0,2,0,1,
    
    1,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    
    1,1,1,1,0,1,1,1,1,1,1,0,1,1,1,1,1,0,1,1,1,1,1,1,1,1,0,1,1,1,1,1,
    
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,1,
    
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,1,
    
    1,0,0,0,2,0,0,0,2,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,2,0,0,0,1,
    
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    
    1,0,0,0,2,0,0,0,2,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,2,0,0,0,1,
    
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,1,
    
    1,1,1,1,1,1,0,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,0,1,1,1,
    
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    
    1,0,0,0,2,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,2,0,0,0,0,0,1,
    
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    
    1,0,0,0,2,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,2,0,0,0,0,0,1,
    
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    
    1,1,1,0,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,1,1,1,
    
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    
    1,0,0,0,2,0,0,0,0,2,0,0,0,2,0,0,0,0,0,0,2,0,0,0,0,0,2,0,0,0,0,1,
    
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    
    1,0,0,0,2,0,0,0,0,2,0,0,0,2,0,0,0,3,0,0,2,0,0,0,0,0,2,0,0,0,0,1,
    
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
};

int getMap(int x, int y) {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) return 1;
    return worldMap[y * MAP_WIDTH + x];
}

class RaycastWindow : public mx::VKWindow {
public:
    float posX = 3.5f, posY = 1.5f;     
    float dirX = 0.0f, dirY = 1.0f;     
    float planeX = 0.66f, planeY = 0.0f; 
    float moveSpeed = 3.5f;   
    float rotSpeed  = 2.5f;   
    
    
    Uint64 lastFrameTime = 0;
    float deltaTime = 0.0f;

    bool keyW = false, keyS = false, keyA = false, keyD = false;
    bool keyLeft = false, keyRight = false;

    
    SDL_GameController *controller = nullptr;
    static constexpr float DEAD_ZONE = 8000.0f;
    float leftStickX = 0.0f, leftStickY = 0.0f;   
    float rightStickX = 0.0f, rightStickY = 0.0f;  
    
    RaycastWindow(const std::string& path, int wx, int wy, bool full) 
        : mx::VKWindow("-[ Vulkan Raycaster ]-", wx, wy, full) {
        setPath(path);
    }
    
    virtual ~RaycastWindow() {
        if (controller) {
            SDL_GameControllerClose(controller);
            controller = nullptr;
        }
    }
    
    void initVulkan() override {
        mx::VKWindow::initVulkan();
        lastFrameTime = SDL_GetPerformanceCounter();
        
        for (int i = 0; i < SDL_NumJoysticks(); ++i) {
            if (SDL_IsGameController(i)) {
                controller = SDL_GameControllerOpen(i);
                if (controller) {
                    std::cout << ">> [Controller] Opened: " << SDL_GameControllerName(controller) << std::endl;
                    break;
                }
            }
        }
        if (!controller) {
            std::cout << ">> [Controller] No game controller found, using keyboard only." << std::endl;
        }
    }
    
    bool canMove(float newX, float newY) {
        float margin = 0.2f;
        return getMap(int(newX - margin), int(newY - margin)) == 0 &&
               getMap(int(newX + margin), int(newY - margin)) == 0 &&
               getMap(int(newX - margin), int(newY + margin)) == 0 &&
               getMap(int(newX + margin), int(newY + margin)) == 0;
    }

    void rotate(float angle) {
        float oldDirX = dirX;
        dirX = dirX * cos(angle) - dirY * sin(angle);
        dirY = oldDirX * sin(angle) + dirY * cos(angle);
        float oldPlaneX = planeX;
        planeX = planeX * cos(angle) - planeY * sin(angle);
        planeY = oldPlaneX * sin(angle) + planeY * cos(angle);
    }
    
    void updatePlayer() {
        
        Uint64 now = SDL_GetPerformanceCounter();
        deltaTime = static_cast<float>(now - lastFrameTime) / static_cast<float>(SDL_GetPerformanceFrequency());
        lastFrameTime = now;
        
        if (deltaTime > 0.1f) deltaTime = 0.1f;

        float ms = moveSpeed * deltaTime;
        float rs = rotSpeed  * deltaTime;
        
        if (keyW) {
            float newX = posX + dirX * ms;
            float newY = posY + dirY * ms;
            if (canMove(newX, posY)) posX = newX;
            if (canMove(posX, newY)) posY = newY;
        }
        if (keyS) {
            float newX = posX - dirX * ms;
            float newY = posY - dirY * ms;
            if (canMove(newX, posY)) posX = newX;
            if (canMove(posX, newY)) posY = newY;
        }
        if (keyA) {
            float newX = posX - planeX * ms;
            float newY = posY - planeY * ms;
            if (canMove(newX, posY)) posX = newX;
            if (canMove(posX, newY)) posY = newY;
        }
        if (keyD) {
            float newX = posX + planeX * ms;
            float newY = posY + planeY * ms;
            if (canMove(newX, posY)) posX = newX;
            if (canMove(posX, newY)) posY = newY;
        }
        if (keyLeft) rotate(rs);
        if (keyRight) rotate(-rs);

        
        if (std::abs(leftStickY) > 0.01f) {
            float spd = leftStickY * ms;
            float newX = posX - dirX * spd;
            float newY = posY - dirY * spd;
            if (canMove(newX, posY)) posX = newX;
            if (canMove(posX, newY)) posY = newY;
        }
        if (std::abs(leftStickX) > 0.01f) {
            float spd = leftStickX * ms;
            float newX = posX + planeX * spd;
            float newY = posY + planeY * spd;
            if (canMove(newX, posY)) posX = newX;
            if (canMove(posX, newY)) posY = newY;
        }

        
        if (std::abs(rightStickX) > 0.01f) {
            rotate(-rightStickX * rs);
        }
    }
    
    void proc() override {
        updatePlayer();
        raycastPlayer.posX = posX;
        raycastPlayer.posY = posY;
        raycastPlayer.dirX = dirX;
        raycastPlayer.dirY = dirY;
        raycastPlayer.planeX = planeX;
        raycastPlayer.planeY = planeY;
        printText("WASD/Left Stick: Move  Arrows/Right Stick: Turn  ESC: Quit", 10, 10, {255, 255, 255, 255});        
        std::string posStr = std::format("Pos: {:.1f}, {:.1f}", posX, posY);
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
        else if (e.type == SDL_CONTROLLERAXISMOTION) {
            float val = static_cast<float>(e.caxis.value);
            
            if (std::abs(val) < DEAD_ZONE) val = 0.0f;
            else val = val / 32767.0f;

            switch (e.caxis.axis) {
                case SDL_CONTROLLER_AXIS_LEFTX:  leftStickX  = val; break;
                case SDL_CONTROLLER_AXIS_LEFTY:  leftStickY  = val; break;
                case SDL_CONTROLLER_AXIS_RIGHTX: rightStickX = val; break;
                case SDL_CONTROLLER_AXIS_RIGHTY: rightStickY = val; break;
            }
        }
        else if (e.type == SDL_CONTROLLERBUTTONDOWN) {
            if (e.cbutton.button == SDL_CONTROLLER_BUTTON_START ||
                e.cbutton.button == SDL_CONTROLLER_BUTTON_BACK) {
                quit();
            }
        }
        else if (e.type == SDL_CONTROLLERDEVICEADDED) {
            if (!controller) {
                controller = SDL_GameControllerOpen(e.cdevice.which);
                if (controller)
                    std::cout << ">> [Controller] Connected: " << SDL_GameControllerName(controller) << std::endl;
            }
        }
        else if (e.type == SDL_CONTROLLERDEVICEREMOVED) {
            if (controller && e.cdevice.which == SDL_JoystickInstanceID(
                    SDL_GameControllerGetJoystick(controller))) {
                SDL_GameControllerClose(controller);
                controller = nullptr;
                leftStickX = leftStickY = rightStickX = rightStickY = 0.0f;
                std::cout << ">> [Controller] Disconnected." << std::endl;
            }
        }
    }

private:
};

int main(int argc, char **argv) {
#if defined(__APPLE__) || defined(_WIN32) || defined(_WIN64) || defined(__linux__)
#ifndef __ANDROID__
    Arguments args = proc_args(argc, argv);
    try {
        RaycastWindow window(args.path, args.width, args.height, args.fullscreen);
        window.initVulkan();
        window.loop();
        window.cleanup();
    } catch (mx::Exception &e) {
        SDL_Log("mx: Exception: %s\n", e.text().c_str());
    }
#endif
#elif defined(__ANDROID__)
    try {
        RaycastWindow window("", 960, 720, false);
        window.initVulkan();
        window.loop();
        window.cleanup();
    } catch (mx::Exception &e) {
        SDL_Log("mx: Exception: %s\n", e.text().c_str());
    }
#endif
    return EXIT_SUCCESS;
}
