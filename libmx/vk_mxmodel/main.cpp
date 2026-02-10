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

        mx::ModelViewer viewer("-[ MXModel Viewer ]-", args.width, args.height, args.fullscreen);
        viewer.setPath(dataPath);
        if (!args.filename.empty())
            viewer.setModelFile(args.filename);
        else
            std::cout << ">> No --filename given, will try default model in data path\n";
        if (!args.texture.empty())
            viewer.setTextureFile(args.texture);
        viewer.initVulkan();
        viewer.loop();
        viewer.cleanup();
    } catch (mx::Exception &e) {
        SDL_Log("mx: Exception: %s\n", e.text().c_str());
    }
    return EXIT_SUCCESS;
}
