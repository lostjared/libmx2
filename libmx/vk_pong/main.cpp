#include "vk.hpp"
#include"SDL.h"
#include <format>

#if defined(__APPLE__) || defined(_WIN32) || defined(__linux__)
#include "argz.hpp"
#endif

class MainWindow : public mx::VKWindow {
public:
    Uint64 lastFrameTime = 0;
    
    MainWindow(const std::string& path, int wx, int wy, bool full) : mx::VKWindow("-[ VK Stars 3D ]-", wx, wy, full) {
        setPath(path);
        cameraZ = 5.0f; 
        cameraX = 0.0f;
        cameraY = 0.0f;
        cameraYaw = -90.0f;
        cameraPitch = 0.0f;
        cameraSpeed = 5.0f;
        lastFrameTime = SDL_GetPerformanceCounter();
    }
    virtual ~MainWindow() {
    }
    virtual void event(SDL_Event& e) override {
        const float mouseSensitivity = 0.1f;
        
        if(e.type == SDL_QUIT) {
            quit();
            return;
        }

        if(e.type == SDL_KEYDOWN) {
            switch(e.key.keysym.sym) {
                case SDLK_SPACE:
                    if (currentPolygonMode == VK_POLYGON_MODE_FILL) {
                        currentPolygonMode = VK_POLYGON_MODE_LINE;
                    } else {
                        currentPolygonMode = VK_POLYGON_MODE_FILL;
                    }
                    break;
                case SDLK_RETURN:
                    cameraZ = 5.0f;
                    cameraX = 0.0f;
                    cameraY = 0.0f;
                    cameraYaw = -90.0f;
                    cameraPitch = 0.0f;
                    cameraSpeed = 5.0f;
                    break;
                case SDLK_ESCAPE:
                    quit();
                    break;
                case SDLK_1:
                    lightPollution = 0.0f;
                    atmosphericTwinkle = 0.3f;
                    break;
                case SDLK_2:
                    lightPollution = 0.3f;
                    atmosphericTwinkle = 0.7f;
                    break;
                case SDLK_3:
                    lightPollution = 0.7f;
                    atmosphericTwinkle = 1.0f;
                    break;
                case SDLK_z:
                    cameraSpeed += 1.0f;
                    break;
                case SDLK_x:
                    cameraSpeed -= 1.0f;
                    if (cameraSpeed < 0.5f) cameraSpeed = 0.5f;
                    break;
            }
        }
        
        
        if (e.type == SDL_MOUSEMOTION && (e.motion.state & SDL_BUTTON_LMASK)) {
            cameraYaw += e.motion.xrel * mouseSensitivity;
            cameraPitch -= e.motion.yrel * mouseSensitivity;
            
            if (cameraPitch > 89.0f) cameraPitch = 89.0f;
            if (cameraPitch < -89.0f) cameraPitch = -89.0f;
        }
    }
    virtual void proc() override {
        Uint64 currentTime = SDL_GetPerformanceCounter();
        float deltaTime = (currentTime - lastFrameTime) / (double)SDL_GetPerformanceFrequency();
        lastFrameTime = currentTime;
        
        if (deltaTime > 0.1f) deltaTime = 0.1f;
        
        const Uint8* keyState = SDL_GetKeyboardState(nullptr);
        
        glm::vec3 front;
        front.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
        front.y = sin(glm::radians(cameraPitch));
        front.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
        front = glm::normalize(front);
        
        glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
        glm::vec3 up(0.0f, 1.0f, 0.0f);
        
        float moveSpeed = cameraSpeed * deltaTime;
        
        if (keyState[SDL_SCANCODE_W] || keyState[SDL_SCANCODE_UP]) {
            cameraX += front.x * moveSpeed;
            cameraY += front.y * moveSpeed;
            cameraZ += front.z * moveSpeed;
        }
        if (keyState[SDL_SCANCODE_S] || keyState[SDL_SCANCODE_DOWN]) {
            cameraX -= front.x * moveSpeed;
            cameraY -= front.y * moveSpeed;
            cameraZ -= front.z * moveSpeed;
        }
        if (keyState[SDL_SCANCODE_A] || keyState[SDL_SCANCODE_LEFT]) {
            cameraX -= right.x * moveSpeed;
            cameraZ -= right.z * moveSpeed;
        }
        if (keyState[SDL_SCANCODE_D] || keyState[SDL_SCANCODE_RIGHT]) {
            cameraX += right.x * moveSpeed;
            cameraZ += right.z * moveSpeed;
        }

        if (keyState[SDL_SCANCODE_Q]) {
            cameraY += moveSpeed;
        }
        if (keyState[SDL_SCANCODE_E]) {
            cameraY -= moveSpeed;
        }
        
        SDL_Color white {255, 255, 255, 255};
        SDL_Color green {0, 255, 0, 255};
        SDL_Color yellow {255, 255, 0, 255};
        
        static uint64_t frameCount = 0;
        static Uint64 fpsLastTime = SDL_GetPerformanceCounter();
        static double fps = 0.0;
        static int fpsUpdateCounter = 0;
        
        ++frameCount;
        ++fpsUpdateCounter;
        
        Uint64 fpsCurrentTime = SDL_GetPerformanceCounter();
        double elapsed = (fpsCurrentTime - fpsLastTime) / (double)SDL_GetPerformanceFrequency();
        
        if (fpsUpdateCounter >= 10) {
            fps = fpsUpdateCounter / elapsed;
            fpsLastTime = fpsCurrentTime;
            fpsUpdateCounter = 0;
        }
        std::string fpsText = std::format("FPS: {:.1f}", fps);
        std::string polygonMode = (currentPolygonMode == VK_POLYGON_MODE_LINE) ? "WIREFRAME" : "SOLID";
        printText("Vulkan Stars 3D", 50, 50, white);
        printText("WASD/Arrows: Move | Q/E: Up/Down | Mouse Drag: Look", 50, 100, green);
        printText("1/2/3: Light pollution | Z/X: Speed | SPACE: Toggle Polygon Mode", 50, 150, green);
        printText(fpsText, 50, 200, yellow);
        printText(std::format("Speed: {:.1f} | Pos: ({:.1f}, {:.1f}, {:.1f}) | Mode: {}", 
                              cameraSpeed, cameraX, cameraY, cameraZ, polygonMode), 50, 250, white);
    }
private:

};
int main(int argc, char **argv) {
    #if defined(__APPLE__) || defined(_WIN32) || defined(_WIN64) || defined(__linux__)
#ifndef __ANDROID__
        Arguments args = proc_args(argc, argv);
        try {
            MainWindow window(args.path, args.width, args.height, args.fullscreen);
            window.initVulkan();
            window.initStarfield(60000);
            window.loop();   
            window.cleanup();
        } catch (mx::Exception &e) {
            SDL_Log("mx: Exception: %s\n", e.text().c_str());
        }
#endif
    #elif defined(__ANDROID__)
        try {
            MainWindow window("", 960, 720, false);
            window.initVulkan();
            window.initStarfield(60000);
            window.loop();   
            window.cleanup();
        } catch (mx::Exception &e) {
            SDL_Log("mx: Exception: %s\n", e.text().c_str());
        }
    #endif
        return EXIT_SUCCESS;
}