#ifndef PLUGIN_OUTPUT_HPP
#define PLUGIN_OUTPUT_HPP

/*
 * plugin_output.hpp — Header-only C++ convenience adapter for plugins.
 *
 * Provides a stream-like PluginOutput class so plugin authors can write:
 *
 *     PluginOutput output(out_ctx, out_fn);
 *     output << "Error: " << SDL_GetError() << std::endl;
 *
 * Everything in this header is compiled INTO the plugin, so no C++
 * types (std::string, std::ostream, etc.) ever cross the DLL boundary.
 */

#include "plugin_api.h"
#include <sstream>
#include <string>

class PluginOutput {
    void *ctx_;
    plugin_output_fn fn_;

  public:
    PluginOutput(void *ctx, plugin_output_fn fn) : ctx_(ctx), fn_(fn) {}

    /* Catch-all: convert any streamable type through a local ostringstream. */
    template <typename T>
    PluginOutput &operator<<(const T &val) {
        std::ostringstream ss;
        ss << val;
        fn_(ctx_, ss.str().c_str());
        return *this;
    }

    /* Fast path for C string literals / const char*. */
    PluginOutput &operator<<(const char *s) {
        fn_(ctx_, s);
        return *this;
    }

    /* Fast path for std::string (stays within the plugin). */
    PluginOutput &operator<<(const std::string &s) {
        fn_(ctx_, s.c_str());
        return *this;
    }

    /* Handle std::endl and other ostream manipulators. */
    PluginOutput &operator<<(std::ostream &(*manip)(std::ostream &)) {
        std::ostringstream ss;
        manip(ss);
        std::string s = ss.str();
        if (!s.empty())
            fn_(ctx_, s.c_str());
        return *this;
    }
};

#endif /* PLUGIN_OUTPUT_HPP */
