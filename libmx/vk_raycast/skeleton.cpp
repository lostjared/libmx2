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

// Map for collision detection
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

class RaycastWindow : public mx::VKWindow {
public:
    float posX = 8.0f, posY = 2.0f;     
    float dirX = 0.0f, dirY = 1.0f;     
    float planeX = 0.66f, planeY = 0.0f; 
    float moveSpeed = 0.01f;
    float rotSpeed = 0.01f;
    
    
    bool keyW = false, keyS = false, keyA = false, keyD = false;
    bool keyLeft = false, keyRight = false;
    
    RaycastWindow(const std::string& path, int wx, int wy, bool full) 
        : mx::VKWindow("-[ Vulkan Raycaster ]-", wx, wy, full) {
        setPath(path);
    }
    
    virtual ~RaycastWindow() {}
    
    void initVulkan() override {
        mx::VKWindow::initVulkan();
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
        updatePlayer();
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
