#include "SDL.h"
#include "ast.hpp"
#include <map>
#include <unordered_map>
#include <memory>

static SDL_Window* g_window = nullptr;
static SDL_Renderer* g_renderer = nullptr;
static bool g_running = false;
std::unordered_map<std::string, SDL_Surface *> surfaces;
class Raii {
public:
     ~Raii() {
        for(auto &s : surfaces) {
            SDL_FreeSurface(s.second);
        }
        surfaces.clear();
     }
};
static Raii raii;

extern "C" {
    int sdl_init(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream& output) {
        if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK) < 0) {
            output << "SDL_Init Error: " << SDL_GetError() << std::endl;
            return 1;
        }
        return 0;
    }
    
    int sdl_create_window(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream& output) {
        std::string title = "SDL Window";
        int width = 800;
        int height = 600;
        int x = SDL_WINDOWPOS_CENTERED;
        int y = SDL_WINDOWPOS_CENTERED;
        Uint32 flags = SDL_WINDOW_SHOWN;
        if (args.size() > 0) title = cmd::getVar(args[0]);
        if (args.size() > 1) width = std::stoi(cmd::getVar(args[1]));
        if (args.size() > 2) height = std::stoi(cmd::getVar(args[2]));
        if (args.size() > 3) x = std::stoi(cmd::getVar(args[3]));
        if (args.size() > 4) y = std::stoi(cmd::getVar(args[4]));
        if (args.size() > 5) {
            std::string flag_str = cmd::getVar(args[5]);
            if (flag_str == "fullscreen") flags |= SDL_WINDOW_FULLSCREEN;
            if (flag_str == "resizable") flags |= SDL_WINDOW_RESIZABLE;
            if (flag_str == "borderless") flags |= SDL_WINDOW_BORDERLESS;
        }    
        g_window = SDL_CreateWindow(
            title.c_str(),
            x, y,
            width, height,
            flags
        );
        
        if (g_window == nullptr) {
            output << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
            return 1;
        }
        return 0;
    }
    
    int sdl_loadsurface(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream& output) {
        if (g_window == nullptr) {
            output << "Window must be created first" << std::endl;
            throw cmd::AstFailure("Window must be created first");
        }
        if (args.size() < 1) {
            output << "Usage: loadsurface <filename>" << std::endl;
            throw cmd::AstFailure("Too many arguments for load surface");
            return 1;
        }
        std::string filename = cmd::getVar(args[0]);
        SDL_Surface* surface = SDL_LoadBMP(filename.c_str());
        if (surface == nullptr) {
            output << "SDL_LoadBMP Error: " << SDL_GetError() << std::endl;
            throw cmd::AstFailure("Error loading bitmap: " + std::string(SDL_GetError()));
            return 1;
        }
        static int offset = 0;
        std::string s = std::to_string(offset);
        surfaces[s] = surface;
        output << offset++;
        return 0;
    }

    int sdl_copysurface(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream& output) {
        if (g_renderer == nullptr) {
            output << "Renderer must be created first" << std::endl;
            throw cmd::AstFailure("Renderer must be created first");
            return 1;
        }
        if (args.size() < 3) {
            output << "Usage: copysurface <surface_id> <x> <y>" << std::endl;
            throw cmd::AstFailure("Not enough arguments for copysurface");
            return 1;
        }
        std::string surface_id = cmd::getVar(args[0]);
        int x = std::stoi(cmd::getVar(args[1]));
        int y = std::stoi(cmd::getVar(args[2]));
        auto it = surfaces.find(surface_id);
        if (it == surfaces.end()) {
            output << "Surface not found: " << surface_id << std::endl;
            throw cmd::AstFailure("Surface not found");
            return 1;
        }
        SDL_Surface* surface = it->second;
        int w = surface->w;
        int h = surface->h;
        if(args.size() >= 5) {
            w = std::stoi(cmd::getVar(args[3]));
            h = std::stoi(cmd::getVar(args[4]));
        }
        SDL_Texture* texture = SDL_CreateTextureFromSurface(g_renderer, surface);
        if (texture == nullptr) {
            output << "Failed to create texture: " << SDL_GetError() << std::endl;
            throw cmd::AstFailure("Failed to create texture");
            return 1;
        }
        SDL_Rect dst_rect = {x, y, w, h};
        SDL_RenderCopy(g_renderer, texture, nullptr, &dst_rect);
        SDL_DestroyTexture(texture);
        return 0;
    }

    int sdl_freesurface(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream& output) {
        if (g_window == nullptr) {
            output << "Window must be created first" << std::endl;
            throw cmd::AstFailure("Window must be created first");
            return 1;
        }
        if (args.size() < 1) {
            output << "Usage: freesurface id" << std::endl;
            throw cmd::AstFailure("Usage: freesurface id");
            return 1;
        }
        std::string num = cmd::getVar(args[0]);
        auto pos = surfaces.find(num);
        if(pos != surfaces.end()) {
            SDL_FreeSurface(pos->second);
            surfaces.erase(pos);
        } else {
            output << "Error: Surface not found..\n";
            throw cmd::AstFailure("Surface not found");
        }
        return 0;
    }
    
    int sdl_create_renderer(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream& output) {
        if (g_window == nullptr) {
            output << "Window must be created first" << std::endl;
            return 1;
        }
        
        int index = -1;  
        Uint32 flags = SDL_RENDERER_ACCELERATED;
        
        if (args.size() > 0) index = std::stoi(cmd::getVar(args[0]));
        if (args.size() > 1) {
            std::string flag_str = cmd::getVar(args[1]);
            if (flag_str == "software") flags = SDL_RENDERER_SOFTWARE;
            if (flag_str == "vsync") flags |= SDL_RENDERER_PRESENTVSYNC;
        }
        
        g_renderer = SDL_CreateRenderer(g_window, index, flags);
        
        if (g_renderer == nullptr) {
            output << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
            return 1;
        }
        return 0;
    }
    
    
    int sdl_set_color(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream& output) {
        if (g_renderer == nullptr) {
            output << "Renderer must be created first" << std::endl;
            return 1;
        }
        
        Uint8 r = 0, g = 0, b = 0, a = 255;
        
        if (args.size() > 0) r = static_cast<Uint8>(std::stoi(cmd::getVar(args[0])));
        if (args.size() > 1) g = static_cast<Uint8>(std::stoi(cmd::getVar(args[1])));
        if (args.size() > 2) b = static_cast<Uint8>(std::stoi(cmd::getVar(args[2])));
        if (args.size() > 3) a = static_cast<Uint8>(std::stoi(cmd::getVar(args[3])));
        
        if (SDL_SetRenderDrawColor(g_renderer, r, g, b, a) < 0) {
            output << "SDL_SetRenderDrawColor Error: " << SDL_GetError() << std::endl;
            return 1;
        }
        return 0;
    }
    
    int sdl_clear(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream& output) {
        if (g_renderer == nullptr) {
            output << "Renderer must be created first" << std::endl;
            return 1;
        }
        
        if (SDL_RenderClear(g_renderer) < 0) {
            output << "SDL_RenderClear Error: " << SDL_GetError() << std::endl;
            return 1;
        }
        return 0;
    }
    
    int sdl_draw_rect(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream& output) {
        if (g_renderer == nullptr) {
            output << "Renderer must be created first" << std::endl;
            return 1;
        }
        
        SDL_Rect rect = {0, 0, 100, 100};
        
        if (args.size() > 0) rect.x = std::stoi(cmd::getVar(args[0]));
        if (args.size() > 1) rect.y = std::stoi(cmd::getVar(args[1]));
        if (args.size() > 2) rect.w = std::stoi(cmd::getVar(args[2]));
        if (args.size() > 3) rect.h = std::stoi(cmd::getVar(args[3]));
        
        if (SDL_RenderFillRect(g_renderer, &rect) < 0) {
            output << "SDL_RenderFillRect Error: " << SDL_GetError() << std::endl;
            return 1;
        }
        return 0;
    }
    
    int sdl_draw_line(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream& output) {
        if (g_renderer == nullptr) {
            output << "Renderer must be created first" << std::endl;
            return 1;
        }
        
        int x1 = 0, y1 = 0, x2 = 100, y2 = 100;
        
        if (args.size() > 0) x1 = std::stoi(cmd::getVar(args[0]));
        if (args.size() > 1) y1 = std::stoi(cmd::getVar(args[1]));
        if (args.size() > 2) x2 = std::stoi(cmd::getVar(args[2]));
        if (args.size() > 3) y2 = std::stoi(cmd::getVar(args[3]));
        
        if (SDL_RenderDrawLine(g_renderer, x1, y1, x2, y2) < 0) {
            output << "SDL_RenderDrawLine Error: " << SDL_GetError() << std::endl;
            return 1;
        }
        return 0;
    }
    
    int sdl_present(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream& output) {
        if (g_renderer == nullptr) {
            output << "Renderer must be created first" << std::endl;
            return 1;
        }
        
        SDL_RenderPresent(g_renderer);
        return 0;
    }
    
    int sdl_process_events(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream& output) {
        static SDL_Event event;
        
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    exit(0);
                    break;
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        exit(0);
                    }
                    output << "Key pressed: " << SDL_GetKeyName(event.key.keysym.sym) << std::endl;
                    break;
            }
        }
        return 0;
    }
    
    int sdl_main_loop(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream& output) {
        g_running = true;
        int target_fps = 60;
        
        if (args.size() > 0) {
            target_fps = std::stoi(cmd::getVar(args[0]));
        }
        
        const int frame_delay = 1000 / target_fps;
        Uint32 frame_start;
        int frame_time;
        
        while (g_running) {
            frame_start = SDL_GetTicks();
            
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    g_running = false;
                }
            }
            
            frame_time = SDL_GetTicks() - frame_start;
            if (frame_delay > frame_time) {
                SDL_Delay(frame_delay - frame_time);
            }
        }
        
        return 0;
    }
    
    int sdl_destroy(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream& output) {
        if (g_renderer) {
            SDL_DestroyRenderer(g_renderer);
            g_renderer = nullptr;
        }
        if (g_window) {
            SDL_DestroyWindow(g_window);
            g_window = nullptr;
        }
        return 0;
    }
    
    int sdl_quit(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream& output) {
        sdl_destroy(args, input, output);
        SDL_Quit();
        return 0;
    }

     int sdl_delay(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream& output) {
        int ms = 1000; 
        if (args.size() > 0) {
            ms = std::stoi(cmd::getVar(args[0]));
        }
        if (ms < 0) {
            output << "Delay cannot be negative" << std::endl;
            return 1;
        }
        SDL_Delay(static_cast<Uint32>(ms));
        return 0;
    }
}