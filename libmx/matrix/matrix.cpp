#include"mx.hpp"
#include"argz.hpp"
#include<unordered_map>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

class Game : public obj::Object {
public:
    Game() = default;
    ~Game() override {
        releaseMatrix();
    }

    virtual void load(mx::mxWindow* win) override {
        the_font.loadFont(win->util.getFilePath("data/keifont.ttf"), 12);
    }


    virtual void draw(mx::mxWindow* win) override {
        createMatrixRain(win->renderer, the_font.wrapper().unwrap(), 640, 480);
    }

    virtual void event(mx::mxWindow* win, SDL_Event& e) override {
        
    }

private:
    mx::Font the_font;
    std::unordered_map<std::string, SDL_Texture*> char_textures;

    void releaseMatrix() {
        for(auto &i : char_textures) {
            if(i.second != nullptr) {
                SDL_DestroyTexture(i.second);
            }
        }
    }

    std::string unicodeToUTF8(int codepoint) {
        std::string utf8;
        if (codepoint <= 0x7F) {
            utf8 += static_cast<char>(codepoint);
        } else if (codepoint <= 0x7FF) {
            utf8 += static_cast<char>((codepoint >> 6) | 0xC0);
            utf8 += static_cast<char>((codepoint & 0x3F) | 0x80);
        } else if (codepoint <= 0xFFFF) {
            utf8 += static_cast<char>((codepoint >> 12) | 0xE0);
            utf8 += static_cast<char>(((codepoint >> 6) & 0x3F) | 0x80);
            utf8 += static_cast<char>((codepoint & 0x3F) | 0x80);
        } else if (codepoint <= 0x10FFFF) {
            utf8 += static_cast<char>((codepoint >> 18) | 0xF0);
            utf8 += static_cast<char>(((codepoint >> 12) & 0x3F) | 0x80);
            utf8 += static_cast<char>(((codepoint >> 6) & 0x3F) | 0x80);
            utf8 += static_cast<char>((codepoint & 0x3F) | 0x80);
        }
        return utf8;
    }

    std::vector<std::pair<int, int>> codepoint_ranges = {
        {0x3041, 0x3096},   
        {0x30A0, 0x30FF}
    };

    int getRandomCodepoint() {
        int range_index = rand() % codepoint_ranges.size();
        int start = codepoint_ranges[range_index].first;
        int end = codepoint_ranges[range_index].second;
        return start + rand() % (end - start + 1);
    }

    SDL_Color computeTrailColor(int trail_offset, int trail_length) {
        float intensity = 1.0f - (float)trail_offset / (float)trail_length;
        if (intensity < 0.0f) intensity = 0.0f;

        Uint8 alpha = static_cast<Uint8>(255 * intensity);
        if (alpha < 50) alpha = 50;

        Uint8 green = static_cast<Uint8>(255 * intensity);
        if (green < 100) green = 100; 
        SDL_Color color = {0, green, 0, alpha};
        return color;
    }

    void createMatrixRain(SDL_Renderer* renderer, TTF_Font* font, int screen_width, int screen_height) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 50);  
        SDL_Rect screenRect = {0, 0, screen_width, screen_height};
        SDL_RenderFillRect(renderer, &screenRect);

        int char_width = 0;
        int char_height = 0;
        TTF_SizeUTF8(font, "A", &char_width, &char_height); 
        int num_columns = screen_width / char_width + 1;
        int num_rows = screen_height / char_height + 1;

        static std::vector<float> fall_positions(num_columns, 0.0f);
        static std::vector<int> fall_speeds(num_columns, 0);
        static std::vector<int> trail_lengths(num_columns, 0);
        static Uint32 last_time = 0;
        float speed_multiplier = 2.0f;

        if (fall_positions[0] == 0.0f) {
            for (int col = 0; col < num_columns; ++col) {
                fall_positions[col] = static_cast<float>(rand() % num_rows);
                fall_speeds[col] = (rand() % 7 + 3) * speed_multiplier;
                trail_lengths[col] = rand() % 15 + 5; 
            }
            last_time = SDL_GetTicks();
        }

        Uint32 current_time = SDL_GetTicks();
        float delta_time = (current_time - last_time) / 1000.0f;
        last_time = current_time;

        for (int col = 0; col < num_columns; ++col) {
            fall_positions[col] += fall_speeds[col] * delta_time;
            if (fall_positions[col] >= num_rows) {
                fall_positions[col] -= num_rows;
                
                trail_lengths[col] = rand() % 15 + 5;
                fall_speeds[col] = (rand() % 7 + 3) * speed_multiplier;
            }

            for (int trail_offset = 0; trail_offset < trail_lengths[col]; ++trail_offset) {
                int row = static_cast<int>(fall_positions[col] - trail_offset + num_rows) % num_rows;

                int random_char_code = getRandomCodepoint();
                std::string random_char = unicodeToUTF8(random_char_code);

                SDL_Texture* char_texture = nullptr;
                if (char_textures.find(random_char) == char_textures.end()) {
                    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, random_char.c_str(), {255, 255, 255, 255});
                    char_texture = SDL_CreateTextureFromSurface(renderer, surface);
                    SDL_FreeSurface(surface);
                    char_textures[random_char] = char_texture;
                } else {
                    char_texture = char_textures[random_char];
                }

                SDL_Color color = computeTrailColor(trail_offset, trail_lengths[col]);
                SDL_SetTextureColorMod(char_texture, color.r, color.g, color.b);
                SDL_SetTextureAlphaMod(char_texture, color.a);

                SDL_Rect dst_rect = {col * char_width, row * char_height, char_width, char_height};
                SDL_RenderCopy(renderer, char_texture, nullptr, &dst_rect);
            }
        }
    }
};



class Intro : public obj::Object {
public:
    Intro() {}
    ~Intro() override {}

    virtual void load(mx::mxWindow *win) override {
        tex.loadTexture(win, win->util.getFilePath("data/logo.png"));
    }

    virtual void draw(mx::mxWindow *win) override {
        static Uint32 previous_time = SDL_GetTicks();
        Uint32 current_time = SDL_GetTicks();
        static int alpha = 255;
        static bool fading_out = true;
        static bool done = false;

        SDL_SetTextureAlphaMod(tex.wrapper().unwrap(), alpha);
        SDL_RenderCopy(win->renderer, tex.wrapper().unwrap(), nullptr, nullptr);

        if(done == true) {
            win->setObject(new Game());
            win->object->load(win);
            return;
        }
        
        if (current_time - previous_time >= 15) {
            previous_time = current_time;
            if (fading_out) {
                alpha -= 3;  
                if (alpha <= 0) {
                    alpha = 0;
                    fading_out = false;
                    done = true;
                }
            } else {
                alpha += 3;
                if (alpha >= 255) {
                    alpha = 255;
                    fading_out = true;
                    previous_time = SDL_GetTicks();
                }
            }
        }
    }
    
    virtual void event(mx::mxWindow *win, SDL_Event &e) override {}

private:
    mx::Texture tex;
};

class MainWindow : public mx::mxWindow {
public:
    
    MainWindow(std::string path, int tw, int th) : mx::mxWindow("Matrix", tw, th, false) {
        tex.createTexture(this, 640, 480);
      	setPath(path);
        setObject(new Intro());
		object->load(this);
    }
    
    ~MainWindow() override {

    }
    
    virtual void event(SDL_Event &e) override {}

    virtual void draw(SDL_Renderer *renderer) override {
        SDL_SetRenderTarget(renderer, tex.wrapper().unwrap());
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        // draw
        object->draw(this);
        SDL_SetRenderTarget(renderer,nullptr);
        SDL_RenderCopy(renderer, tex.wrapper().unwrap(), nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }
private:
    mx::Texture tex;
};

MainWindow *main_w = nullptr;

void eventProc() {
    main_w->proc();
}

int main(int argc, char **argv) {
#ifdef __EMSCRIPTEN__
    MainWindow main_window("/");
    main_w =&main_window;
    emscripten_set_main_loop(eventProc, 0, 1);
#else

    Argz<std::string> parser(argc, argv);    
    parser.addOptionSingle('h', "Display help message")
          .addOptionSingleValue('p', "assets path")
          .addOptionDoubleValue('P', "path", "assets path")
          .addOptionSingleValue('r',"Resolution WidthxHeight")
          .addOptionDoubleValue('R',"resolution", "Resolution WidthxHeight");
    Argument<std::string> arg;
    std::string path;
    int value = 0;
    int tw = 1440, th = 1080;
    try {
        while((value = parser.proc(arg)) != -1) {
            switch(value) {
                case 'h':
                case 'v':
                    parser.help(std::cout);
                    exit(EXIT_SUCCESS);
                    break;
                case 'p':
                case 'P':
                    path = arg.arg_value;
                break;
                case 'r':
                case 'R': {
                    auto pos = arg.arg_value.find("x");
                    if(pos == std::string::npos)  {
                        mx::system_err << "Error invalid resolution use WidthxHeight\n";
                        mx::system_err.flush();
                        exit(EXIT_FAILURE);
                    }
                    std::string left, right;
                    left = arg.arg_value.substr(0, pos);
                    right = arg.arg_value.substr(pos+1);
                    tw = atoi(left.c_str());
                    th = atoi(right.c_str());
                }
                break;

            }
        }
    } catch (const ArgException<std::string>& e) {
        mx::system_err << e.text() << "\n";
    }

    if(path.empty()) {
        mx::system_err << "Matrix: Requires path variable to assets...\n";
        mx::system_err.flush();
        exit(EXIT_FAILURE);
    }
    try {
        MainWindow main_window(path, tw, th);
        main_window.loop();
    } catch(const mx::Exception &e) {
        mx::system_err << "mx: Exception: " << e.text() << "\n";
        mx::system_err.flush();
        exit(EXIT_FAILURE);
    } 
#endif
    return 0;
}