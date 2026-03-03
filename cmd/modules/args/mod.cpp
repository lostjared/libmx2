#include "plugin_api.h"
#include "plugin_output.hpp"

extern "C" {
    int HeyCommand(int argc, const char** argv, void* out_ctx, plugin_output_fn out_fn) {
        PluginOutput output(out_ctx, out_fn);
        for (int index = 0; index < argc; ++index) {
            output << index << " " << (argv[index] ? argv[index] : "") << "\n";
        }
        return 0;
    }
}