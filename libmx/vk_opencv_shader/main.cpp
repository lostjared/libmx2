#include"vk.hpp"
#include"SDL.h"
#include"argz.hpp"
#include<fstream>
#include<iostream>
#include<filesystem>
#include<sstream>
#include<vector>
#include<algorithm>
#include<opencv2/opencv.hpp>

class ShaderWindow : public mx::VKWindow {
    class ShaderLibrary {
    public:
        ShaderLibrary() = default;
        ~ShaderLibrary() = default;
        ShaderLibrary &operator=(const ShaderLibrary &) = delete;
        ShaderLibrary &operator=(ShaderLibrary &&) = delete;
        ShaderLibrary(const ShaderLibrary &) = delete;
        ShaderLibrary(ShaderLibrary &&) = delete;

        void load(const std::string &path, const std::string &text) {
            if(!shader_names.empty()) 
                shader_names.clear();

            std::fstream file;
            file.open(text, std::ios::in);
            if(!file.is_open()) {
                throw mx::Exception("acvk: Error loading index file: " + text);
            }
            std::string s;
            while(std::getline(file, s)) {
                if(!s.empty() ) {
                    std::string path_name = path + "/" + s;
                    if(std::filesystem::exists(path_name)) {
                        shader_names.push_back(s);
                        std::cout << "acvk: Shader Filename: " << s << "\n";
                    }
                    else {
                        std::cerr << "acvk: File: " << path_name << " does not exisit at path: " << path << "\n";
                    }
                }
            }
        }

        std::string getShaderName(int index) {
            if(index >= 0 && index < static_cast<int>(shader_names.size())) 
                return shader_names[index];
            throw mx::Exception("acvk: Shader Index out of obounds of get name");
        }

        int size() const {
            return static_cast<int>(shader_names.size());
        }

        std::vector<char> loadShader(int index) {
            if(index >= 0 && index < static_cast<int>(shader_names.size()))
                return mx::readFile(shader_names[index]);
            throw mx::Exception("acvk: Could not load shader file: " + shader_names[index]);
        }

        void release() {

        }

    protected:
        std::vector<std::string> shader_names;
    };

public:
    ShaderWindow(const std::string& path, int wx, int wy, bool full)
        : mx::VKWindow("-[ Vulkan Example / OpenCV Shader ]-", wx, wy, full) {
        setPath(path);
    }
    ShaderWindow() = delete;
    ShaderWindow(const ShaderWindow &) = delete;
    ShaderWindow(ShaderWindow &&) = delete;
    ShaderWindow &operator=(const ShaderWindow &) = delete;
    ShaderWindow &operator=(ShaderWindow &&) = delete;

    void openLibrary(const std::string path, const std::string &filename) {
        library.load(path, filename);
        shader_path = path;
    }

    void initVulkan() override {
        mx::VKWindow::initVulkan();
    }

    void swichShader(size_t index) {
        if(!sprite) return;
        shaderIndex = static_cast<int>(index);
        std::string fragPath = shader_path + "/" + library.getShaderName(shaderIndex);
        vkDeviceWaitIdle(device);
        sprite->createEmptySprite(camera_width, camera_height,
            util.path + "/data/sprite_vert.spv", fragPath);
        sprite->enableExtendedUBO();
        startTime = lastTime = SDL_GetTicks();
        frameCount = 0;
    }

    void openCamera(int index, int width, int height) {
        cap.open(index, cv::CAP_V4L2);
        if(!cap.isOpened()) {
            throw mx::Exception("Error could not open camera at index: " + std::to_string(index));
        }
        cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M','J','P','G'));
        cap.set(cv::CAP_PROP_FRAME_WIDTH, width);
        cap.set(cv::CAP_PROP_FRAME_HEIGHT, height);
        camera_width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
        camera_height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
        std::string fragPath = (library.size() > 0)
            ? shader_path + "/" + library.getShaderName(0)
            : util.path + "/data/sprite_frag.spv";
        sprite = createSprite(camera_width, camera_height, util.path + "/data/sprite_vert.spv", fragPath);
        sprite->enableExtendedUBO();
        sprite->rebuildPipeline();
        resizeWindow(camera_width, camera_height);
        startTime = lastTime = SDL_GetTicks();
        frameCount = 0;
    }

    void openFile(const std::string &filename) {
        this->filename = filename;
        if(!cap.open(filename)) {
            throw mx::Exception("Error could not open file: " + filename);
        }
        camera_width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
        camera_height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
        std::string fragPath = (library.size() > 0)
            ? shader_path + "/" + library.getShaderName(0)
            : util.path + "/data/sprite_frag.spv";
        sprite = createSprite(camera_width, camera_height, util.path + "/data/sprite_vert.spv", fragPath);
        sprite->enableExtendedUBO();
        sprite->rebuildPipeline();
        resizeWindow(camera_width, camera_height);
        startTime = lastTime = SDL_GetTicks();
        frameCount = 0;
    }

    void proc() override {
        if (!sprite) {
            return;
        }
        cv::Mat frame;
        if(!cap.read(frame)) {
            std::cout << "acvk: video loop\n";
            if(!filename.empty()) {
                cap.set(cv::CAP_PROP_POS_FRAMES, 0);
                if(!cap.read(frame)) {
                    quit();
                    return;
                }
            } else {
                quit();
                return;
            }
        }

        Uint32 now = SDL_GetTicks();
        float iTime = (now - startTime) / 1000.0f;
        float iTimeDelta = (now - lastTime) / 1000.0f;
        lastTime = now;
        frameCount++;
        float iFrameRate = (iTimeDelta > 0.0f) ? (1.0f / iTimeDelta) : 60.0f;
        int mx = 0, my = 0;
        Uint32 buttons = SDL_GetMouseState(&mx, &my);
        float mousePressed = (buttons & SDL_BUTTON_LMASK) ? 1.0f : 0.0f;
        sprite->setShaderParams(1.0f, 1.0f, 1.0f, iTime);
        sprite->setMouseState(static_cast<float>(mx), static_cast<float>(my), mousePressed, 0.0f);
        sprite->setUniform0(1.0f, 1.0f, 0.0f, 0.0f);  // amp=1, uamp=1
        sprite->setUniform1(iTimeDelta, 0.0f, 0.0f, iFrameRate);
        sprite->setUniform2(static_cast<float>(frameCount), 0.0f, 0.0f, 0.0f);
        cv::Mat rgba;
        cv::cvtColor(frame, rgba, cv::COLOR_BGR2RGBA);
        sprite->updateTexture(rgba.data, rgba.cols, rgba.rows, rgba.step);
        sprite->drawSpriteRect(0, 0, getWidth(), getHeight());
        if(library.size() > 0)
            printText("Shader: " + library.getShaderName(shaderIndex), 15, 15, {255, 255, 255, 255});
    }
    void event(SDL_Event& e) override {
        if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)) {
            quit();
        }
        if (e.type == SDL_KEYDOWN) {
            if(library.size() == 0) return;
            if (e.key.keysym.sym == SDLK_RIGHT) {
                shaderIndex = (shaderIndex + 1) % library.size();
                swichShader(shaderIndex);
            } else if (e.key.keysym.sym == SDLK_LEFT) {
                shaderIndex = (shaderIndex - 1 + library.size()) % library.size();
                swichShader(shaderIndex);
            }
        }
    }

private:
    mx::VKSprite* sprite = nullptr;
    ShaderLibrary library;
    size_t camera_width = 0;
    size_t camera_height = 0;
    cv::VideoCapture cap;
    std::string filename;
    std::string shader_path;
    int shaderIndex = 0;
    Uint32 startTime = 0;
    Uint32 lastTime = 0;
    int frameCount = 0;
};

int main(int argc, char **argv) {
    Arguments args = proc_args(argc, argv);
    try {
        ShaderWindow window(args.path, args.width, args.height, args.fullscreen);
        std::string shader_path = std::filesystem::path(args.filename).parent_path().string();
        if(!args.shaderPath.empty()) 
            shader_path = args.shaderPath; 
        std::cout << "acvk: Setting shader path: " << shader_path << "\n";
        window.openLibrary(shader_path, args.filename);
        window.initVulkan();
        if(args.texture.empty() || args.texture == "camera")
            window.openCamera(0, args.width, args.height);
        else
            window.openFile(args.texture);
        window.loop();
        window.cleanup();
    } catch (mx::Exception &e) {
        SDL_Log("mx: Exception: %s\n", e.text().c_str());
        return EXIT_FAILURE;
    }
    catch(const std::exception &e) {
        SDL_Log("Exception: %s\n", e.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
