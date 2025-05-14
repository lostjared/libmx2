#include "SDL.h"
#include "ast.hpp"
#include <map>
#include <memory>

// Globals to track resources
static SDL_Window* g_window = nullptr;
static SDL_Renderer* g_renderer = nullptr;
static bool g_running = false;

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
                    g_running = false;
                    break;
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        g_running = false;
                    }
                    output << "Key pressed: " << SDL_GetKeyName(event.key.keysym.sym) << std::endl;
                    break;
                case SDL_MOUSEMOTION:
                    output << "Mouse moved to: (" << event.motion.x << ", " << event.motion.y << ")" << std::endl;
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    output << "Mouse button " << (int)event.button.button << " pressed at: (" 
                           << event.button.x << ", " << event.button.y << ")" << std::endl;
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
}