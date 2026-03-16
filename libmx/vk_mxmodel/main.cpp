#include "vk.hpp"
#include "SDL.h"
#include <filesystem>

#if defined(__APPLE__) || defined(_WIN32) || defined(__linux__)
#include "argz.hpp"
#endif

int main(int argc, char **argv) {
    try {
        namespace fs = std::filesystem;

#if defined(__APPLE__) || defined(_WIN32) || defined(__linux__)
        Argz<std::string> parser(argc, argv);
        parser.addOptionSingle('h', "Display help message")
              .addOptionSingleValue('p', "assets path")
              .addOptionDoubleValue('P', "path", "assets path")
              .addOptionSingleValue('r', "Resolution WidthxHeight")
              .addOptionDoubleValue('R', "resolution", "Resolution WidthxHeight")
              .addOptionSingleValue('m', "Model file")
              .addOptionDoubleValue('M', "model", "model file to load")
              .addOptionSingleValue('t', "texture file")
              .addOptionDoubleValue(123, "texture", "texture file")
              .addOptionSingleValue('T', "texture path")
              .addOptionSingle('f', "fullscreen")
              .addOptionDouble('F', "fullscreen", "fullscreen")
              .addOptionDoubleValue(256, "filename", "input filename (alias for -m)")
              .addOptionDoubleValue(257, "compress", "compress true, compress false");

        Argument<std::string> arg;
        int value = 0;
        int tw = 1280, th = 720;
        std::string path;
        std::string model_file;
        std::string text_file, texture_path;
        bool fullscreen = false;

        try {
            while ((value = parser.proc(arg)) != -1) {
                switch (value) {
                    case 'h':
                    case 'v':
                        parser.help(std::cout);
                        return EXIT_SUCCESS;
                    case 'p':
                    case 'P':
                        path = arg.arg_value;
                        break;
                    case 'r':
                    case 'R': {
                        auto pos = arg.arg_value.find("x");
                        if (pos == std::string::npos) {
                            std::cerr << "Error invalid resolution use WidthxHeight\n";
                            return EXIT_FAILURE;
                        }
                        tw = atoi(arg.arg_value.substr(0, pos).c_str());
                        th = atoi(arg.arg_value.substr(pos + 1).c_str());
                    } break;
                    case 'm':
                    case 'M':
                    case 256:
                        model_file = arg.arg_value;
                        break;
                    case 123:
                    case 't':
                        text_file = arg.arg_value;
                        break;
                    case 'T':
                        texture_path = arg.arg_value;
                        break;
                    case 'f':
                    case 'F':
                        fullscreen = true;
                        break;
                    case 257:
                        // compress option accepted for compatibility, not used by VK viewer
                        break;
                }
            }
        } catch (const ArgException<std::string> &e) {
            std::cerr << "mx: Argument Exception: " << e.text() << std::endl;
        }

        if (path.empty()) {
            std::cout << "mx: No path provided trying default current directory.\n";
            path = ".";
        }

        std::string dataPath = path;
        if (!fs::is_directory(dataPath) || (!fs::exists(dataPath + "/vert.spv") && fs::is_directory(dataPath + "/data")))
            dataPath = dataPath + "/data";

        mx::ModelViewer viewer("-[ MXModel Viewer ]-", tw, th, fullscreen);
        viewer.setPath(dataPath);
        if (!model_file.empty())
            viewer.setModelFile(model_file);
        else
            std::cout << ">> No model file given, will try default model in data path\n";
        if (!text_file.empty())
            viewer.setTextureFile(text_file);
        if (!texture_path.empty())
            viewer.setTexturePath(texture_path);
#else
        mx::ModelViewer viewer("-[ MXModel Viewer ]-", 1440, 1080, false);
#endif
        viewer.initVulkan();
        viewer.loop();
        viewer.cleanup();
    } catch (mx::Exception &e) {
        SDL_Log("mx: Exception: %s\n", e.text().c_str());
    }
    return EXIT_SUCCESS;
}
