#ifndef PLUGIN_API_H
#define PLUGIN_API_H

/*
 * plugin_api.h — Pure C interface for mxcmd extern plugins.
 *
 * All types that cross the DLL boundary are POD / C-compatible:
 *   - int argc, const char** argv   (pre-resolved argument strings)
 *   - void* output_ctx              (opaque handle to host output stream)
 *   - plugin_output_fn              (C function pointer callback for writing)
 *
 * The host pre-resolves every Argument (variables, command substitutions)
 * to plain C strings before calling the plugin, so the plugin never needs
 * access to cmd::Argument, cmd::getVar, or any other host-side C++ type.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Callback the plugin invokes to write text to the host's output stream. */
typedef void (*plugin_output_fn)(void *ctx, const char *text);

/*
 * Canonical plugin function signature.
 *
 *   argc      – number of pre-resolved argument strings
 *   argv      – array of argc null-terminated C strings
 *   out_ctx   – opaque context pointer (passed back to out_fn unchanged)
 *   out_fn    – callback to write output text
 *
 * Return 0 on success, non-zero on failure.
 */
typedef int (*plugin_func_t)(int argc, const char **argv,
                             void *out_ctx, plugin_output_fn out_fn);

#ifdef __cplusplus
}
#endif

#endif /* PLUGIN_API_H */
