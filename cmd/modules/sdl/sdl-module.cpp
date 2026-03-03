#include "SDL.h"
#include "SDL_ttf.h"
#include "plugin_api.h"
#include "plugin_output.hpp"
#include <map>
#include <unordered_map>
#include <memory>
#include <cmath>
#include <chrono>
#include <functional>
#include <vector>
#include <string>
#include <sstream>
#include <atomic>
#include <cstdlib>

static SDL_Window* g_window = nullptr;
static SDL_Renderer* g_renderer = nullptr;
static std::atomic<bool> g_running{true};
static Uint32 g_window_id = 0;
static bool g_event_watch_registered = false;
static int g_window_width = 800;
static int g_window_height = 600;
static Uint32 g_plugin_init_flags = 0;
static bool g_ttf_initialized_by_plugin = false;
static bool g_keys[SDL_NUM_SCANCODES] = {false};
static int g_mouse_x = 0;
static int g_mouse_y = 0;
static bool g_mouse_buttons[5] = {false};
static Uint32 g_last_frame_time = 0;
static float g_delta_time = 0.0f;
static std::unordered_map<int, std::function<void(SDL_Event&)>> g_event_handlers;

std::unordered_map<std::string, SDL_Surface*> surfaces;
std::unordered_map<std::string, SDL_Texture*> textures;

class Raii {
public:
    ~Raii() {}
};
static Raii raii;

static std::unordered_map<std::string, TTF_Font*> g_font_cache;
static void cleanupFonts();

static void cleanupPluginResources(bool destroyWindow) {
    cleanupFonts();
    for (auto& pair : textures) {
        SDL_DestroyTexture(pair.second);
    }
    textures.clear();

    for (auto& pair : surfaces) {
        SDL_FreeSurface(pair.second);
    }
    surfaces.clear();

    if (g_renderer) {
        SDL_DestroyRenderer(g_renderer);
        g_renderer = nullptr;
    }

    if (g_window) {
        if (destroyWindow) {
            SDL_DestroyWindow(g_window);
            g_window = nullptr;
        } else {
            SDL_HideWindow(g_window);
        }
    }
}

static int sdl_event_watch(void* userdata, SDL_Event* event) {
    if (event == nullptr) {
        return 1;
    }

    if (event->type == SDL_KEYDOWN && event->key.windowID == g_window_id) {
        if (event->key.keysym.scancode >= 0 && event->key.keysym.scancode < SDL_NUM_SCANCODES) {
            g_keys[event->key.keysym.scancode] = true;
        }
    } else if (event->type == SDL_KEYUP && event->key.windowID == g_window_id) {
        if (event->key.keysym.scancode >= 0 && event->key.keysym.scancode < SDL_NUM_SCANCODES) {
            g_keys[event->key.keysym.scancode] = false;
        }
    }

    if (event->type == SDL_WINDOWEVENT &&
        event->window.windowID == g_window_id &&
        (event->window.event == SDL_WINDOWEVENT_CLOSE ||
         event->window.event == SDL_WINDOWEVENT_HIDDEN)) {
        g_running = false;
    }

    if (event->type == SDL_QUIT) {
        g_running = false;
    }

    if (event->type == SDL_KEYDOWN &&
        event->key.windowID == g_window_id &&
        event->key.keysym.sym == SDLK_ESCAPE &&
        event->key.repeat == 0) {
        g_running = false;
    }

    return 1;
}

static void cleanupFonts() {
    for (auto& pair : g_font_cache) {
        if (pair.second) {
            TTF_CloseFont(pair.second);
        }
    }
    g_font_cache.clear();
    if (g_ttf_initialized_by_plugin && TTF_WasInit()) {
        TTF_Quit();
        g_ttf_initialized_by_plugin = false;
    }
}

float degToRad(float degrees) {
    return degrees * (M_PI / 180.0f);
}

std::string createResourceId(const std::string& prefix) {
    static std::unordered_map<std::string, int> counters;
    int id = counters[prefix]++;
    return prefix + "_" + std::to_string(id);
}

void updateTiming() {
    Uint32 current_time = SDL_GetTicks();
    g_delta_time = (current_time - g_last_frame_time) / 1000.0f;
    g_last_frame_time = current_time;
}

bool ensureRenderer(PluginOutput& output) {
    if (g_renderer != nullptr) {
        return true;
    }
    if (g_window == nullptr) {
        output << "Window must be created first" << std::endl;
        return false;
    }
    g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_SOFTWARE);
    if (g_renderer == nullptr) {
        output << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        return false;
    }
    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);
    return true;
}

extern "C" {
    
    int sdl_init(int argc, const char** argv, void* out_ctx, plugin_output_fn out_fn) {
        PluginOutput output(out_ctx, out_fn);
        Uint32 flags = SDL_INIT_VIDEO | SDL_INIT_TIMER;
        
        if (argc > 0) {
            std::string flag_str = argv[0];
            if (flag_str.find("audio") != std::string::npos) flags |= SDL_INIT_AUDIO;
            if (flag_str.find("joystick") != std::string::npos) flags |= SDL_INIT_JOYSTICK;
            if (flag_str.find("gamepad") != std::string::npos) flags |= SDL_INIT_GAMECONTROLLER;
            if (flag_str.find("haptic") != std::string::npos) flags |= SDL_INIT_HAPTIC;
        }
        
        Uint32 alreadyInitialized = SDL_WasInit(0);
        Uint32 flagsToInit = flags & ~alreadyInitialized;

        if (flagsToInit != 0) {
            if (SDL_InitSubSystem(flagsToInit) < 0) {
                output << "SDL_InitSubSystem Error: " << SDL_GetError() << std::endl;
                return 1;
            }
            g_plugin_init_flags |= flagsToInit;
        }

        g_last_frame_time = SDL_GetTicks();
        return 0;
    }
    
    int sdl_create_window(int argc, const char** argv, void* out_ctx, plugin_output_fn out_fn) {
        PluginOutput output(out_ctx, out_fn);
        std::string title = "SDL Window";
        int width = 800;
        int height = 600;
        int x = SDL_WINDOWPOS_CENTERED;
        int y = SDL_WINDOWPOS_CENTERED;
        Uint32 flags = SDL_WINDOW_SHOWN;
        
        if (argc > 0) title = argv[0];
        if (argc > 1) width = std::stoi(argv[1]);
        if (argc > 2) height = std::stoi(argv[2]);
        if (argc > 3) x = std::stoi(argv[3]);
        if (argc > 4) y = std::stoi(argv[4]);
        if (argc > 5) {
            std::string flag_str = argv[5];
            if (flag_str.find("fullscreen") != std::string::npos) flags |= SDL_WINDOW_FULLSCREEN;
            if (flag_str.find("desktop") != std::string::npos) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
            if (flag_str.find("resizable") != std::string::npos) flags |= SDL_WINDOW_RESIZABLE;
            if (flag_str.find("borderless") != std::string::npos) flags |= SDL_WINDOW_BORDERLESS;
            if (flag_str.find("opengl") != std::string::npos) flags |= SDL_WINDOW_OPENGL;
        }

        if (g_renderer != nullptr) {
            SDL_DestroyRenderer(g_renderer);
            g_renderer = nullptr;
        }

        if (g_window != nullptr) {
            SDL_DestroyWindow(g_window);
            g_window = nullptr;
            g_window_id = 0;
        }
        
        g_window = SDL_CreateWindow(
            title.c_str(),
            x, y,
            width, height,
            flags
        );
        
        g_window_width = width;
        g_window_height = height;
        
        if (g_window == nullptr) {
            output << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
            return 1;
        }

        g_window_id = SDL_GetWindowID(g_window);
        g_running = true;
        if (!g_event_watch_registered) {
            SDL_AddEventWatch(sdl_event_watch, nullptr);
            g_event_watch_registered = true;
        }
        return 0;
    }
    
    int sdl_create_renderer(int argc, const char** argv, void* out_ctx, plugin_output_fn out_fn) {
        PluginOutput output(out_ctx, out_fn);
        if (g_window == nullptr) {
            output << "Window must be created first" << std::endl;
            return 1;
        }
        
        int index = -1;  
        Uint32 flags = SDL_RENDERER_ACCELERATED;
        bool requested_software = false;
        
        if (argc > 0) index = std::stoi(argv[0]);
        if (argc > 1) {
            std::string flag_str = argv[1];
            if (flag_str.find("software") != std::string::npos) {
                flags = SDL_RENDERER_SOFTWARE;
                requested_software = true;
            }
            if (flag_str.find("vsync") != std::string::npos) flags |= SDL_RENDERER_PRESENTVSYNC;
            if (flag_str.find("target") != std::string::npos) flags |= SDL_RENDERER_TARGETTEXTURE;
        }

        if (g_renderer != nullptr) {
            SDL_DestroyRenderer(g_renderer);
            g_renderer = nullptr;
        }
        
        g_renderer = SDL_CreateRenderer(g_window, index, flags);

        if (g_renderer == nullptr && !requested_software) {
            Uint32 fallback_flags = SDL_RENDERER_SOFTWARE;
            if (flags & SDL_RENDERER_PRESENTVSYNC) fallback_flags |= SDL_RENDERER_PRESENTVSYNC;
            if (flags & SDL_RENDERER_TARGETTEXTURE) fallback_flags |= SDL_RENDERER_TARGETTEXTURE;
            g_renderer = SDL_CreateRenderer(g_window, index, fallback_flags);
        }
        
        if (g_renderer == nullptr) {
            output << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
            return 1;
        }
        
        SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);
        return 0;
    }
    
    
    int sdl_loadsurface(int argc, const char** argv, void* out_ctx, plugin_output_fn out_fn) {
        PluginOutput output(out_ctx, out_fn);
        if (g_window == nullptr) {
            output << "Window must be created first" << std::endl;
            return 1;
        }
        
        if (argc < 1) {
            output << "Usage: sdl_loadsurface <filename>" << std::endl;
            return 1;
        }
        
        std::string filename = argv[0];
        SDL_Surface* surface = SDL_LoadBMP(filename.c_str());
        
        if (surface == nullptr) {
            output << "SDL_LoadBMP Error: " << SDL_GetError() << std::endl;
            return 1;
        }
        if (argc >= 4) {
            int r = std::stoi(argv[1]);
            int g = std::stoi(argv[2]);
            int b = std::stoi(argv[3]);    
            Uint32 colorkey = SDL_MapRGB(surface->format, r, g, b);
            SDL_SetColorKey(surface, SDL_TRUE, colorkey);
        }
        
        std::string id = createResourceId("surface");
        surfaces[id] = surface;
        output << id;
        return 0;
    }
    
    int sdl_create_texture(int argc, const char** argv, void* out_ctx, plugin_output_fn out_fn) {
        PluginOutput output(out_ctx, out_fn);
        if (g_renderer == nullptr) {
            output << "Renderer must be created first" << std::endl;
            return 1;
        }
        
        if (argc < 1) {
            output << "Usage: sdl_create_texture <surface_id> [blend_mode]" << std::endl;
            return 1;
        }
        
        std::string surface_id = argv[0];
        auto it = surfaces.find(surface_id);
        
        if (it == surfaces.end()) {
            output << "Surface not found: " << surface_id << std::endl;
            return 1;
        }
        
        SDL_Texture* texture = SDL_CreateTextureFromSurface(g_renderer, it->second);
        
        if (texture == nullptr) {
            output << "Failed to create texture: " << SDL_GetError() << std::endl;
            return 1;
        }
        
        
        if (argc > 1) {
            std::string blend_mode = argv[1];
            SDL_BlendMode mode = SDL_BLENDMODE_NONE;
            
            if (blend_mode == "blend") mode = SDL_BLENDMODE_BLEND;
            else if (blend_mode == "add") mode = SDL_BLENDMODE_ADD;
            else if (blend_mode == "mod") mode = SDL_BLENDMODE_MOD;
            
            SDL_SetTextureBlendMode(texture, mode);
        }
        
        std::string id = createResourceId("texture");
        textures[id] = texture;
        output << id;
        return 0;
    }
    
    int sdl_freesurface(int argc, const char** argv, void* out_ctx, plugin_output_fn out_fn) {
        PluginOutput output(out_ctx, out_fn);
        if (argc < 1) {
            output << "Usage: sdl_freesurface <surface_id>" << std::endl;
            return 1;
        }
        
        std::string id = argv[0];
        auto it = surfaces.find(id);
        
        if (it == surfaces.end()) {
            output << "Surface not found: " << id << std::endl;
            return 0; 
        }
        
        SDL_FreeSurface(it->second);
        surfaces.erase(it);
        return 0;
    }
    
    int sdl_freetexture(int argc, const char** argv, void* out_ctx, plugin_output_fn out_fn) {
        PluginOutput output(out_ctx, out_fn);
        if (argc < 1) {
            output << "Usage: sdl_freetexture <texture_id>" << std::endl;
            return 1;
        }
        
        std::string id = argv[0];
        auto it = textures.find(id);
        
        if (it == textures.end()) {
            output << "Texture not found: " << id << std::endl;
            return 0; 
        }
        
        SDL_DestroyTexture(it->second);
        textures.erase(it);
        return 0;
    }
    
    
    int sdl_set_color(int argc, const char** argv, void* out_ctx, plugin_output_fn out_fn) {
        PluginOutput output(out_ctx, out_fn);
        if (!ensureRenderer(output)) {
            return 1;
        }
        Uint8 r = 0, g = 0, b = 0, a = 255;
        if (argc > 0) r = static_cast<Uint8>(std::stoi(argv[0]));
        if (argc > 1) g = static_cast<Uint8>(std::stoi(argv[1]));
        if (argc > 2) b = static_cast<Uint8>(std::stoi(argv[2]));
        if (argc > 3) a = static_cast<Uint8>(std::stoi(argv[3]));
        
        if (SDL_SetRenderDrawColor(g_renderer, r, g, b, a) < 0) {
            output << "SDL_SetRenderDrawColor Error: " << SDL_GetError() << std::endl;
            return 1;
        }
        return 0;
    }
    
    int sdl_clear(int argc, const char** argv, void* out_ctx, plugin_output_fn out_fn) {
        PluginOutput output(out_ctx, out_fn);
        if (!ensureRenderer(output)) {
            return 1;
        }
        
        if (SDL_RenderClear(g_renderer) < 0) {
            output << "SDL_RenderClear Error: " << SDL_GetError() << std::endl;
            return 1;
        }
        return 0;
    }
    
    int sdl_draw_rect(int argc, const char** argv, void* out_ctx, plugin_output_fn out_fn) {
        PluginOutput output(out_ctx, out_fn);
        if (g_renderer == nullptr) {
            output << "Renderer must be created first" << std::endl;
            return 1;
        }
        
        SDL_Rect rect = {0, 0, 100, 100};
        bool filled = true;
        
        if (argc > 0) rect.x = std::stoi(argv[0]);
        if (argc > 1) rect.y = std::stoi(argv[1]);
        if (argc > 2) rect.w = std::stoi(argv[2]);
        if (argc > 3) rect.h = std::stoi(argv[3]);
        if (argc > 4) filled = std::string(argv[4]) != "0";
        
        int result;
        if (filled) {
            result = SDL_RenderFillRect(g_renderer, &rect);
        } else {
            result = SDL_RenderDrawRect(g_renderer, &rect);
        }
        
        if (result < 0) {
            output << "SDL_RenderRect Error: " << SDL_GetError() << std::endl;
            return 1;
        }
        return 0;
    }
    
    int sdl_draw_line(int argc, const char** argv, void* out_ctx, plugin_output_fn out_fn) {
        PluginOutput output(out_ctx, out_fn);
        if (g_renderer == nullptr) {
            output << "Renderer must be created first" << std::endl;
            return 1;
        }
        
        int x1 = 0, y1 = 0, x2 = 100, y2 = 100;
        
        if (argc > 0) x1 = std::stoi(argv[0]);
        if (argc > 1) y1 = std::stoi(argv[1]);
        if (argc > 2) x2 = std::stoi(argv[2]);
        if (argc > 3) y2 = std::stoi(argv[3]);
        
        if (SDL_RenderDrawLine(g_renderer, x1, y1, x2, y2) < 0) {
            output << "SDL_RenderDrawLine Error: " << SDL_GetError() << std::endl;
            return 1;
        }
        return 0;
    }
    
    int sdl_draw_circle(int argc, const char** argv, void* out_ctx, plugin_output_fn out_fn) {
        PluginOutput output(out_ctx, out_fn);
        if (g_renderer == nullptr) {
            output << "Renderer must be created first" << std::endl;
            return 1;
        }
        
        int x = g_window_width / 2;
        int y = g_window_height / 2;
        int radius = 50;
        bool filled = false;
        
        if (argc > 0) x = std::stoi(argv[0]);
        if (argc > 1) y = std::stoi(argv[1]);
        if (argc > 2) radius = std::stoi(argv[2]);
        if (argc > 3) filled = std::string(argv[3]) != "0";
        
        if (filled) {
            for (int dy = -radius; dy <= radius; dy++) {
                int dx = sqrt(radius * radius - dy * dy);
                SDL_RenderDrawLine(g_renderer, x - dx, y + dy, x + dx, y + dy);
            }
        } else {
            int offsetx = 0;
            int offsety = radius;
            int d = radius - 1;
            
            while (offsety >= offsetx) {
                SDL_RenderDrawPoint(g_renderer, x + offsetx, y + offsety);
                SDL_RenderDrawPoint(g_renderer, x + offsety, y + offsetx);
                SDL_RenderDrawPoint(g_renderer, x - offsetx, y + offsety);
                SDL_RenderDrawPoint(g_renderer, x - offsety, y + offsetx);
                SDL_RenderDrawPoint(g_renderer, x + offsetx, y - offsety);
                SDL_RenderDrawPoint(g_renderer, x + offsety, y - offsetx);
                SDL_RenderDrawPoint(g_renderer, x - offsetx, y - offsety);
                SDL_RenderDrawPoint(g_renderer, x - offsety, y - offsetx);
                
                if (d >= 2 * offsetx) {
                    d -= 2 * offsetx + 1;
                    offsetx += 1;
                } else if (d < 2 * (radius - offsety)) {
                    d += 2 * offsety - 1;
                    offsety -= 1;
                } else {
                    d += 2 * (offsety - offsetx - 1);
                    offsety -= 1;
                    offsetx += 1;
                }
            }
        }
        
        return 0;
    }

    
    int sdl_color_string(int argc, const char** argv, void* out_ctx, plugin_output_fn out_fn) {
        PluginOutput output(out_ctx, out_fn);
        if(argc == 0) {
            output << "Usage: color_string \"r g b a\"" << std::endl;
            return 1;
        }
        int r = 0, g = 0, b = 0, a = 255;
        if (argc > 0) {
            std::string value = argv[0];
            auto tokenize = [](const std::string& str) -> std::vector<std::string> {
                std::vector<std::string> tokens;
                std::istringstream iss(str);
                std::string token;
                while (iss >> token) {
                    tokens.push_back(token);
                }
                return tokens;
            };
            std::vector<std::string> num = tokenize(value);
            if(num.empty()) {
                output << "Error: invalid color value..\n";
                return 1;
            }
            if(num.size() > 0)
                r = std::stoi(num[0]);
            if(num.size() > 1)
                g = std::stoi(num[1]);
            if(num.size() > 2)
                b = std::stoi(num[2]);
            if(num.size() > 3)
                a = std::stoi(num[3]);
        }
        if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255 || a < 0 || a > 255) {
            output << "Invalid color values. Must be between 0 and 255." << std::endl;
            return 1;
        }
        SDL_SetRenderDrawColor(g_renderer, r, g, b, a);
        return 0;
    }
    
    int sdl_draw_text(int argc, const char** argv, void* out_ctx, plugin_output_fn out_fn) {       
        PluginOutput output(out_ctx, out_fn);
        
        if (g_renderer == nullptr) {
            output << "Renderer must be created first" << std::endl;
            return 1;
        }
        
        if (!TTF_WasInit()) {
            if (TTF_Init() < 0) {
                output << "TTF_Init Error: " << TTF_GetError() << std::endl;
                return 1;
            }
            g_ttf_initialized_by_plugin = true;
        }
        
        if (argc < 4) {
            output << "Usage: sdl_draw_text <text> <x> <y> <font_path> [size=24] [r=255] [g=255] [b=255] [a=255]" << std::endl;
            return 1;
        }
        
        std::string text = argv[0];
        int x = std::stoi(argv[1]);
        int y = std::stoi(argv[2]);
        std::string font_path = argv[3];
        
        int font_size = 24;  
        if (argc > 4) {
            font_size = std::stoi(argv[4]);
        }
        
        SDL_Color color = {255, 255, 255, 255};
        if (argc > 5) color.r = static_cast<Uint8>(std::stoi(argv[5]));
        if (argc > 6) color.g = static_cast<Uint8>(std::stoi(argv[6]));
        if (argc > 7) color.b = static_cast<Uint8>(std::stoi(argv[7]));
        if (argc > 8) color.a = static_cast<Uint8>(std::stoi(argv[8]));
        std::string font_key = font_path + "_" + std::to_string(font_size);
        TTF_Font* font = nullptr;
        auto it = g_font_cache.find(font_key);
        
        if (it == g_font_cache.end()) {
            font = TTF_OpenFont(font_path.c_str(), font_size);
            if (font == nullptr) {
                output << "TTF_OpenFont Error: " << TTF_GetError() << " (Path: " << font_path << ")" << std::endl;
                return 1;
            }
            g_font_cache[font_key] = font;
        } else {
            font = it->second;
        }
        
            SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), color);
        if (surface == nullptr) {
            output << "TTF_RenderText Error: " << TTF_GetError() << std::endl;
            return 1;
        }
        SDL_Texture* texture = SDL_CreateTextureFromSurface(g_renderer, surface);
        if (texture == nullptr) {
            SDL_FreeSurface(surface);
            output << "SDL_CreateTextureFromSurface Error: " << SDL_GetError() << std::endl;
            return 1;
        }
        SDL_Rect dst_rect = {x, y, surface->w, surface->h};
        SDL_RenderCopy(g_renderer, texture, nullptr, &dst_rect);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
        return 0;
    }
    

    int sdl_draw_triangle(int argc, const char** argv, void* out_ctx, plugin_output_fn out_fn) {
        PluginOutput output(out_ctx, out_fn);
        if (g_renderer == nullptr) {
            output << "Renderer must be created first" << std::endl;
            return 1;
        }
        
        int x1 = g_window_width / 2;
        int y1 = g_window_height / 2 - 50;
        int x2 = g_window_width / 2 - 50;
        int y2 = g_window_height / 2 + 50;
        int x3 = g_window_width / 2 + 50;
        int y3 = g_window_height / 2 + 50;
        bool filled = false;
        
        if (argc > 0) x1 = std::stoi(argv[0]);
        if (argc > 1) y1 = std::stoi(argv[1]);
        if (argc > 2) x2 = std::stoi(argv[2]);
        if (argc > 3) y2 = std::stoi(argv[3]);
        if (argc > 4) x3 = std::stoi(argv[4]);
        if (argc > 5) y3 = std::stoi(argv[5]);
        if (argc > 6) filled = std::string(argv[6]) != "0";
        
        SDL_RenderDrawLine(g_renderer, x1, y1, x2, y2);
        SDL_RenderDrawLine(g_renderer, x2, y2, x3, y3);
        SDL_RenderDrawLine(g_renderer, x3, y3, x1, y1);
        
        if (filled) {
            int minY = std::min(std::min(y1, y2), y3);
            int maxY = std::max(std::max(y1, y2), y3);
            
            for (int y = minY; y <= maxY; y++) {
                std::vector<int> intersections;
                
                if ((y1 <= y && y2 > y) || (y2 <= y && y1 > y)) {
                    intersections.push_back(x1 + (y - y1) * (x2 - x1) / (y2 - y1));
                }
                if ((y2 <= y && y3 > y) || (y3 <= y && y2 > y)) {
                    intersections.push_back(x2 + (y - y2) * (x3 - x2) / (y3 - y2));
                }
                if ((y3 <= y && y1 > y) || (y1 <= y && y3 > y)) {
                    intersections.push_back(x3 + (y - y3) * (x1 - x3) / (y1 - y3));
                }
                
                if (intersections.size() >= 2) {
                    if (intersections[0] > intersections[1]) {
                        std::swap(intersections[0], intersections[1]);
                    }
                    
                    SDL_RenderDrawLine(g_renderer, intersections[0], y, intersections[1], y);
                }
            }
        }
        
        return 0;
    }

    
    int sdl_copysurface(int argc, const char** argv, void* out_ctx, plugin_output_fn out_fn) {
        PluginOutput output(out_ctx, out_fn);
        if (g_renderer == nullptr) {
            output << "Renderer must be created first" << std::endl;
            return 1;
        }
        if (argc < 3) {
            output << "Usage: copysurface <surface_id> <x> <y>" << std::endl;
            return 1;
        }
        std::string surface_id = argv[0];
        int x = std::stoi(argv[1]);
        int y = std::stoi(argv[2]);
        auto it = surfaces.find(surface_id);
        if (it == surfaces.end()) {
            output << "Surface not found: " << surface_id << std::endl;
            return 1;
        }
        SDL_Surface* surface = it->second;
        int w = surface->w;
        int h = surface->h;
        if(argc >= 5) {
            w = std::stoi(argv[3]);
            h = std::stoi(argv[4]);
        }

        SDL_Texture* texture = SDL_CreateTextureFromSurface(g_renderer, surface);
        if (texture == nullptr) {
            output << "Failed to create texture: " << SDL_GetError() << std::endl;
            return 1;
        }
        SDL_Rect dst_rect = {x, y, w, h};
        SDL_RenderCopy(g_renderer, texture, nullptr, &dst_rect);
        SDL_DestroyTexture(texture);
        return 0;
    }
    
    int sdl_render_texture(int argc, const char** argv, void* out_ctx, plugin_output_fn out_fn) {
        PluginOutput output(out_ctx, out_fn);
        if (g_renderer == nullptr) {
            output << "Renderer must be created first" << std::endl;
            return 1;
        }
        
        if (argc < 3) {
            output << "Usage: sdl_render_texture <texture_id> <x> <y> [w h] [angle] [flip]" << std::endl;
            return 1;
        }
        
        std::string texture_id = argv[0];
        int x = std::stoi(argv[1]);
        int y = std::stoi(argv[2]);
        
        auto it = textures.find(texture_id);
        if (it == textures.end()) {
            output << "Texture not found: " << texture_id << std::endl;
            return 1;
        }
        
        SDL_Texture* texture = it->second;
        
        int w, h;
        SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);
        
        
        if (argc > 4) {
            w = std::stoi(argv[3]);
            h = std::stoi(argv[4]);
        }
        
        
        SDL_Rect dst_rect = {x, y, w, h};
        
        double angle = 0.0;
        SDL_RendererFlip flip = SDL_FLIP_NONE;
        
        if (argc > 5) {
            angle = std::stod(argv[5]);
        }
        
        if (argc > 6) {
            std::string flip_str = argv[6];
            if (flip_str == "h") flip = SDL_FLIP_HORIZONTAL;
            else if (flip_str == "v") flip = SDL_FLIP_VERTICAL;
            else if (flip_str == "hv") flip = (SDL_RendererFlip)(SDL_FLIP_HORIZONTAL | SDL_FLIP_VERTICAL);
        }
        
        if (angle != 0.0 || flip != SDL_FLIP_NONE) {
            SDL_Point center = {w/2, h/2};
            if (SDL_RenderCopyEx(g_renderer, texture, nullptr, &dst_rect, angle, &center, flip) < 0) {
                output << "SDL_RenderCopyEx Error: " << SDL_GetError() << std::endl;
                return 1;
            }
        } else {
            if (SDL_RenderCopy(g_renderer, texture, nullptr, &dst_rect) < 0) {
                output << "SDL_RenderCopy Error: " << SDL_GetError() << std::endl;
                return 1;
            }
        }
        
        return 0;
    }
    
    int sdl_present(int argc, const char** argv, void* out_ctx, plugin_output_fn out_fn) {
        PluginOutput output(out_ctx, out_fn);
        if (g_renderer == nullptr) {
            output << "Renderer must be created first" << std::endl;
            return 1;
        }
        
        SDL_RenderPresent(g_renderer);
        updateTiming();  
        return 0;
    }
    
    
    int sdl_process_events(int argc, const char** argv, void* out_ctx, plugin_output_fn out_fn) {
        PluginOutput output(out_ctx, out_fn);
        static SDL_Event event;
        bool verbose = argc > 0 && std::string(argv[0]) == "verbose";

        if (!g_running.load()) {
            output << "0";
            return 0;
        }
        
        
        int mouse_buttons;
        Uint32 mouse_state = SDL_GetMouseState(&g_mouse_x, &g_mouse_y);
        g_mouse_buttons[0] = (mouse_state & SDL_BUTTON(1)) != 0;  
        g_mouse_buttons[1] = (mouse_state & SDL_BUTTON(2)) != 0;  
        g_mouse_buttons[2] = (mouse_state & SDL_BUTTON(3)) != 0;  
        
#ifdef _APPLE__
        if (SDL_PollEvent(&event)) {
#else
	while(SDL_PollEvent(&event)) {
#endif
            if (g_window_id != 0) {
                if (event.type == SDL_WINDOWEVENT && event.window.windowID != g_window_id) continue;
                if (event.type == SDL_KEYDOWN && event.key.windowID != g_window_id) continue;
                if (event.type == SDL_KEYUP && event.key.windowID != g_window_id) continue;
                if (event.type == SDL_MOUSEMOTION && event.motion.windowID != g_window_id) continue;
                if (event.type == SDL_MOUSEBUTTONDOWN && event.button.windowID != g_window_id) continue;
                if (event.type == SDL_MOUSEBUTTONUP && event.button.windowID != g_window_id) continue;
            }

            auto it = g_event_handlers.find(event.type);
            if (it != g_event_handlers.end()) {
                it->second(event);
            }
            
            switch (event.type) {
                case SDL_QUIT:
                    g_running = false;
                    if (verbose) output << "Event: SDL_QUIT" << std::endl;
                    output << "0";
                    return 0;
                    
                case SDL_KEYDOWN:
                    g_keys[event.key.keysym.scancode] = true;
                    if (event.key.keysym.sym == SDLK_ESCAPE && event.key.repeat == 0) {
                        if (verbose) output << "Event: ESC pressed, quitting" << std::endl;
                        g_running = false;
                        output << "0";
                        return 0;
                    }
                    if (verbose) output << "Event: Key pressed: " << SDL_GetKeyName(event.key.keysym.sym) << std::endl;
                    break;
                    
                case SDL_KEYUP:
                    g_keys[event.key.keysym.scancode] = false;
                    if (verbose) output << "Event: Key released: " << SDL_GetKeyName(event.key.keysym.sym) << std::endl;
                    break;
                    
                case SDL_MOUSEMOTION:
                    if (verbose) output << "Event: Mouse moved to: (" << event.motion.x << ", " << event.motion.y << ")" << std::endl;
                    break;
                    
                case SDL_MOUSEBUTTONDOWN:
                    if (verbose) output << "Event: Mouse button " << (int)event.button.button << " pressed at: ("
                                        << event.button.x << ", " << event.button.y << ")" << std::endl;
                    break;
                    
                case SDL_MOUSEBUTTONUP:
                    if (verbose) output << "Event: Mouse button " << (int)event.button.button << " released at: ("
                                        << event.button.x << ", " << event.button.y << ")" << std::endl;
                    break;
                    
                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == g_window_id) {
                        if (verbose) output << "Event: Window close" << std::endl;
                        g_running = false;
                        output << "0";
                        return 0;
                    }
                    if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                        g_window_width = event.window.data1;
                        g_window_height = event.window.data2;
                        if (verbose) output << "Event: Window resized to: " << g_window_width << "x" << g_window_height << std::endl;
                    }
                    break;
            }
        }
        output << "1";
        return 0;
    }
    
    int sdl_is_key_pressed(int argc, const char** argv, void* out_ctx, plugin_output_fn out_fn) {
        PluginOutput output(out_ctx, out_fn);
        if (argc < 1) {
            output << "Usage: sdl_is_key_pressed <key_name>" << std::endl;
            return 1;
        }
        
        std::string key_name = argv[0];
        SDL_Scancode scancode = SDL_GetScancodeFromName(key_name.c_str());
        
        if (scancode == SDL_SCANCODE_UNKNOWN) {
            output << "Unknown key: " << key_name << std::endl;
            return 1;
        }
        
        if (g_keys[scancode]) {
            output << "1";
            return 0;
        } else {
            output << "0";
            return 0;
        }
    }
    
    int sdl_get_mouse_pos(int argc, const char** argv, void* out_ctx, plugin_output_fn out_fn) {
        PluginOutput output(out_ctx, out_fn);
        output << g_mouse_x << " " << g_mouse_y;
        return 0;
    }
    
    int sdl_is_mouse_button_pressed(int argc, const char** argv, void* out_ctx, plugin_output_fn out_fn) {
        PluginOutput output(out_ctx, out_fn);
        int button = 0;  
        
        if (argc > 0) {
            button = std::stoi(argv[0]);
        }
        
        if (button < 0 || button >= 3) {
            output << "Invalid mouse button index (0-2): " << button << std::endl;
            return 1;
        }
        
        output << (g_mouse_buttons[button] ? "1" : "0");
        return 0;
    }
    
 
    int sdl_set_window_title(int argc, const char** argv, void* out_ctx, plugin_output_fn out_fn) {
        PluginOutput output(out_ctx, out_fn);
        if (g_window == nullptr) {
            output << "Window must be created first" << std::endl;
            return 1;
        }
        
        if (argc < 1) {
            output << "Usage: sdl_set_window_title <title>" << std::endl;
            return 1;
        }
        
        std::string title = argv[0];
        SDL_SetWindowTitle(g_window, title.c_str());
        return 0;
    }
    
    int sdl_set_fullscreen(int argc, const char** argv, void* out_ctx, plugin_output_fn out_fn) {
        PluginOutput output(out_ctx, out_fn);
        if (g_window == nullptr) {
            output << "Window must be created first" << std::endl;
            return 1;
        }
        
        bool fullscreen = true;
        bool desktop = true;
        
        if (argc > 0) fullscreen = std::string(argv[0]) != "0";
        if (argc > 1) desktop = std::string(argv[1]) != "0";
        
        Uint32 flags = 0;
        if (fullscreen) {
            flags = desktop ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_WINDOW_FULLSCREEN;
        }
        
        if (SDL_SetWindowFullscreen(g_window, flags) < 0) {
            output << "SDL_SetWindowFullscreen Error: " << SDL_GetError() << std::endl;
            return 1;
        }
        
        return 0;
    }
    
    int sdl_get_window_size(int argc, const char** argv, void* out_ctx, plugin_output_fn out_fn) {
        PluginOutput output(out_ctx, out_fn);
        if (g_window == nullptr) {
            output << "Window must be created first" << std::endl;
            return 1;
        }
        
        output << g_window_width << " " << g_window_height;
        return 0;
    }

    
    int sdl_delay(int argc, const char** argv, void* out_ctx, plugin_output_fn out_fn) {
        PluginOutput output(out_ctx, out_fn);
        int ms = 16;  
        if (argc > 0) {
            ms = std::stoi(argv[0]);
        }
        
        if (ms < 0) {
            output << "Delay cannot be negative" << std::endl;
            return 1;
        }
        
        SDL_Delay(static_cast<Uint32>(ms));
        return 0;
    }
    
    int sdl_get_delta_time(int argc, const char** argv, void* out_ctx, plugin_output_fn out_fn) {
        PluginOutput output(out_ctx, out_fn);
        output << g_delta_time;
        return 0;
    }
    
    int sdl_get_fps(int argc, const char** argv, void* out_ctx, plugin_output_fn out_fn) {
        PluginOutput output(out_ctx, out_fn);
        if (g_delta_time > 0) {
            output << (1.0f / g_delta_time);
        } else {
            output << "0";  
        }
        return 0;
    }
    
    int sdl_get_ticks(int argc, const char** argv, void* out_ctx, plugin_output_fn out_fn) {
        PluginOutput output(out_ctx, out_fn);
        output << SDL_GetTicks();
        return 0;
    }
    
    int sdl_destroy(int argc, const char** argv, void* out_ctx, plugin_output_fn out_fn) {
        cleanupPluginResources(true);
        g_window_id = 0;
        g_running = false;
        if (g_event_watch_registered) {
            SDL_DelEventWatch(sdl_event_watch, nullptr);
            g_event_watch_registered = false;
        }
        return 0;
    }
    
    int sdl_quit(int argc, const char** argv, void* out_ctx, plugin_output_fn out_fn) {
        const char* cmdMode = std::getenv("MXCMD_CMD_MODE");
        if (cmdMode != nullptr && std::string(cmdMode) == "1") {
            cleanupPluginResources(true);
            g_window_id = 0;
            g_running = false;
            if (g_event_watch_registered) {
                SDL_DelEventWatch(sdl_event_watch, nullptr);
                g_event_watch_registered = false;
            }
            SDL_PumpEvents();
            return 0;
        }

        sdl_destroy(argc, argv, out_ctx, out_fn);
        SDL_PumpEvents();

        if (g_plugin_init_flags != 0) {
            SDL_QuitSubSystem(g_plugin_init_flags);
            g_plugin_init_flags = 0;
        }
        return 0;
    }
}
