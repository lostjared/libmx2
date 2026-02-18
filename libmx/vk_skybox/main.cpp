#include "vk.hpp"
#include "SDL.h"
#include <filesystem>

#if defined(__APPLE__) || defined(_WIN32) || defined(__linux__)
#include "argz.hpp"
#endif

int main(int argc, char **argv) {
    Arguments args = proc_args(argc, argv);
    try {
        namespace fs = std::filesystem;
        std::string dataPath = args.path;
        if (!fs::is_directory(dataPath) || (!fs::exists(dataPath + "/vert.spv") && fs::is_directory(dataPath + "/data")))
            dataPath = dataPath + "/data";

        mx::SkyboxViewer viewer("-[ Skybox with Vulkan ]-", args.width, args.height, args.fullscreen, args.shaderPath);
        viewer.setPath(dataPath);
        if (!args.shaderPath.empty()) {
            viewer.setShaderPath(args.shaderPath);
        }
        if (!args.filename.empty()) {
            if (args.filename == "camera")
                viewer.openCamera(0, args.width, args.height);
            else
                viewer.openFile(args.filename);
        }
        viewer.initVulkan();
        // When video + shaders are active, swap to the first shader immediately
        // so we don't try to render with the cubemap shader on a 2D texture
        viewer.activateFirstShader();
        viewer.loop();
        viewer.cleanup();
    } catch (mx::Exception &e) {
        SDL_Log("mx: Exception: %s\n", e.text().c_str());
    }
    return EXIT_SUCCESS;
}