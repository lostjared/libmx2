#include "plugin_api.h"
#include "plugin_output.hpp"

extern "C" {
    int HelloWorldCommand(int argc, const char** argv, void* out_ctx, plugin_output_fn out_fn) {
        PluginOutput output(out_ctx, out_fn);
        output << "Hello, World!\n";
        return 0;
    }
}