#include"mx.hpp"
#include"argz.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class Game : public obj::Object {
public:
    Game() = default;
    ~Game() override {}

    constexpr static int tex_width = 640, tex_height = 480;
    constexpr static int num_stars = 100;

    virtual void load(mx::mxWindow *win) override {
        the_font.loadFont(win->util.getFilePath("data/font.ttf"), 14);
        bg.loadTexture(win, win->util.getFilePath("data/spacelogo.png"));
        resetGame();
    }

    void update(mx::mxWindow *win) {
        if (waiting_for_continue || is_game_over) return;
        updateExplosions();
        const Uint8 *keystate = SDL_GetKeyboardState(nullptr);
        int dx = 0; 
        int dy = 0; 
        static const int AXIS_THRESHOLD = 8000; 
        if (keystate[SDL_SCANCODE_LEFT] || stick.getAxis(SDL_CONTROLLER_AXIS_LEFTX) < -AXIS_THRESHOLD) dx -= 5;
        if (keystate[SDL_SCANCODE_RIGHT] || stick.getAxis(SDL_CONTROLLER_AXIS_LEFTX) > AXIS_THRESHOLD) dx += 5;
        if (keystate[SDL_SCANCODE_UP] || stick.getAxis(SDL_CONTROLLER_AXIS_LEFTY) < -AXIS_THRESHOLD) dy -= 5;
        if (keystate[SDL_SCANCODE_DOWN] || stick.getAxis(SDL_CONTROLLER_AXIS_LEFTY) > AXIS_THRESHOLD) dy += 5;
        player_rect.x += dx;
        player_rect.y += dy;
        if (player_rect.x < 0) player_rect.x = 0;
        if (player_rect.x > tex_width - player_rect.w) player_rect.x = tex_width - player_rect.w;
        if (player_rect.y < 0) player_rect.y = 0;
        if (player_rect.y > tex_height - player_rect.h) player_rect.y = tex_height - player_rect.h;
        if ((keystate[SDL_SCANCODE_SPACE] || stick.getButton(SDL_CONTROLLER_BUTTON_A)) && (SDL_GetTicks() - last_shot_time > 300)) {
            projectiles.push_back({player_rect.x + player_rect.w / 2 - 4, player_rect.y - 20, 8, 16});
            last_shot_time = SDL_GetTicks();
        }


        for (auto it = projectiles.begin(); it != projectiles.end();) {
            it->y -= 10;
            bool hit = false;

            for (auto eit = enemies.begin(); eit != enemies.end();) {
                if (SDL_HasIntersection(&(*it), &(*eit))) {
                    explode(eit->x + eit->w / 2, eit->y + eit->h / 2, ExplosionType::Red);
                    eit = enemies.erase(eit);
                    hit = true;
                    score += 5;
                    regular_enemies_destroyed++;
                    if (regular_enemies_destroyed % 5 == 0) {
                        circular_enemies.push_back({rand() % tex_width, -32, 0.0, 1});
                    }
                } else {
                    ++eit;
                }
            }

            for (auto cit = circular_enemies.begin(); cit != circular_enemies.end();) {
                SDL_Rect circular_rect = {cit->x - 16, cit->y - 16, 32, 32};
                if (SDL_HasIntersection(&(*it), &circular_rect)) {
                    explode(cit->x, cit->y, ExplosionType::Blue);
                    cit = circular_enemies.erase(cit);
                    hit = true;
                    score += 10;
                } else {
                    ++cit;
                }
            }

            if (hit || it->y < -it->h) it = projectiles.erase(it);
            else ++it;
        }

        for (auto it = enemies.begin(); it != enemies.end();) {
            it->y += 1.5;
            if (it->y > tex_height) {
                explode(it->x + it->w / 2, tex_height, ExplosionType::Red, true);
                it = enemies.erase(it);
            } else if (SDL_HasIntersection(&player_rect, &(*it))) {
                explode(player_rect.x + player_rect.w / 2, player_rect.y + player_rect.h / 2, ExplosionType::Red, true);
                it = enemies.erase(it);
            } else {
                ++it;
            }
        }

        for (auto cit = circular_enemies.begin(); cit != circular_enemies.end();) {
            cit->angle += 0.05;
            cit->x = (tex_width / 2) + 100 * cos(cit->angle);
            cit->y += cit->direction * 1.5;
            if (cit->y > tex_height) cit->direction = -1;
            if (cit->y < 0) cit->direction = 1;

            SDL_Rect circular_rect = {cit->x - 16, cit->y - 16, 32, 32};
            if (SDL_HasIntersection(&player_rect, &circular_rect)) {
                explode(cit->x, cit->y, ExplosionType::Red, true);
                cit = circular_enemies.erase(cit);
            } else {
                ++cit;
            }
        }

        for (auto &star : stars) {
            star.y += star.speed;
            if (star.y > tex_height) star.y = 0;
        }

        if (SDL_GetTicks() - last_spawn_time > 1000) {
            enemies.push_back({rand() % (tex_width - 32), -32, 32, 32});
            last_spawn_time = SDL_GetTicks();
        }
    }

    virtual void draw(mx::mxWindow *win) override {
        SDL_Renderer *renderer = win->renderer;

        drawExplosions(renderer);

        for (const auto &star : stars) {
            int brightness = 200 + 55 * (star.speed - 1);
            SDL_SetRenderDrawColor(renderer, brightness, brightness, brightness, 255);
            SDL_RenderDrawPoint(renderer, star.x, star.y);
        }

        if (!waiting_for_continue && !is_game_over) {
            drawGradientTriangle(renderer, player_rect.x + player_rect.w / 2, player_rect.y, player_rect.x, player_rect.y + player_rect.h, player_rect.x + player_rect.w, player_rect.y + player_rect.h);

            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            for (const auto &proj : projectiles) {
                SDL_RenderFillRect(renderer, &proj);
            }

            for (const auto &enemy : enemies) {
                drawGradientDiamond(renderer, enemy.x + enemy.w / 2, enemy.y + enemy.h / 2, enemy.w / 2, enemy.h / 2);
            }

            for (const auto &circular : circular_enemies) {
                drawGradientCircle(renderer, circular.x, circular.y, 16);
            }
        }
        win->text.setColor({255,255,255,255});
        std::string lives_text = "Lives: " + std::to_string(lives);
        win->text.printText_Blended(the_font, 10, 10, lives_text);

        std::string score_text = "Score: " + std::to_string(score);
        win->text.printText_Blended(the_font, 10, 30, score_text);

        if (waiting_for_continue) {
            SDL_RenderCopy(win->renderer, bg.wrapper().unwrap(), nullptr, nullptr);
            SDL_Color col = { static_cast<unsigned char>(rand()%255), static_cast<unsigned char>(rand()%255), static_cast<unsigned char>(rand()%255), 255};
            win->text.setColor(col);
            SDL_Rect rc = { 195, 235, 275, 25 };
            SDL_SetRenderDrawColor(win->renderer, 0, 0, 0, 255);
            SDL_RenderFillRect(win->renderer, &rc);
            win->text.printText_Blended(the_font, 200, 240, "You Died! Press ENTER to continue");
        }

        if (is_game_over) {
            SDL_RenderCopy(win->renderer, bg.wrapper().unwrap(), nullptr, nullptr);
            SDL_Color col = { static_cast<unsigned char>(rand()%255), static_cast<unsigned char>(rand()%255), static_cast<unsigned char>(rand()%255), 255};
            win->text.setColor(col);
            SDL_Rect rc = { 195, 235, 275, 25 };
            SDL_SetRenderDrawColor(win->renderer, 0, 0, 0, 255);
            SDL_RenderFillRect(win->renderer, &rc);
            win->text.printText_Blended(the_font, 200, 240, "Game Over! Press ENTER to restart");
        }

        static Uint32 previous_time = SDL_GetTicks();
        Uint32 current_time = SDL_GetTicks();
        if (current_time - previous_time >= 15) {
            previous_time = current_time;
            update(win);
        }
    }
    
    SDL_Rect player_rect = {tex_width / 2 - 16, tex_height - 40, 16, 16};  
    float ship_x = tex_width / 2.0f;  
    float ship_y = tex_height - player_rect.h - 20;
    float touch_start_x = 0.0f;  
    float touch_start_y = 0.0f;
    bool is_touch_active = false;  

    virtual void event(mx::mxWindow *win, SDL_Event &e) override {
        if(stick.connectEvent(e)) {
            mx::system_out << "Controller Event\n";
        }
        switch (e.type) {
        case SDL_FINGERDOWN:

            if(waiting_for_continue) {
                waiting_for_continue = false;
                resetRound();
                return;
            }
            if(is_game_over) {
                is_game_over = false;
                resetGame();
                return;
            }

            touch_start_x = e.tfinger.x * tex_width;
            touch_start_y = e.tfinger.y * tex_height; 
            is_touch_active = true;

            if (SDL_GetTicks() - last_shot_time > 300) {
                projectiles.push_back({
                    static_cast<int>(ship_x + player_rect.w / 2 - 4),
                    static_cast<int>(ship_y - 20),
                    8, 16
                });
                last_shot_time = SDL_GetTicks();
            }
            break;

        case SDL_FINGERMOTION:
            if (is_touch_active) {
                float touch_current_x = e.tfinger.x * tex_width;
                float touch_current_y = e.tfinger.y * tex_height;
                ship_x += (touch_current_x - touch_start_x);
                ship_y += (touch_current_y - touch_start_y);
                touch_start_x = touch_current_x;
                touch_start_y = touch_current_y;                if (ship_x < 0) ship_x = 0;
                if (ship_x > tex_width - player_rect.w) ship_x = tex_width - player_rect.w;
                if (ship_y < 0) ship_y = 0;
                if (ship_y > tex_height - player_rect.h) ship_y = tex_height - player_rect.h;
                player_rect.x = static_cast<int>(ship_x);
                player_rect.y = static_cast<int>(ship_y);
            }
            break;
        case SDL_FINGERUP:
            is_touch_active = false;
            break;
        }

        if (waiting_for_continue && ((e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_RETURN) || (e.type == SDL_JOYBUTTONDOWN && e.jbutton.button == 1))) {
            waiting_for_continue = false;
            resetRound();
        }

        if (is_game_over && e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_RETURN) {
            is_game_over = false;
            resetGame();
        }

        if( is_game_over && e.type == SDL_JOYBUTTONDOWN && e.jbutton.button == 1) {
            is_game_over = false;
            resetGame();
        }
    }

private:
    mx::Font the_font;
    mx::Texture bg;
    mx::Controller stick;
    std::vector<SDL_Rect> projectiles;
    std::vector<SDL_Rect> enemies;

    struct Star {
        int x, y, speed;
    };
    std::vector<Star> stars;

    struct Explosion {
        int x, y;
        int duration;
        SDL_Color color;
        bool die;
    };
    std::vector<Explosion> explosions;

    struct CircularEnemy {
        int x, y;
        double angle;
        int direction;
    };
    std::vector<CircularEnemy> circular_enemies;

    enum class ExplosionType {
        Red,
        Blue
    };

    int lives = 5;
    int score = 0;
    int regular_enemies_destroyed = 0;
    bool is_game_over = false;
    bool waiting_for_continue = false;
    bool player_die = false;
    Uint32 last_shot_time = 0;
    Uint32 last_spawn_time = 0;

    void resetGame() {
        player_rect = {tex_width / 2 - 16, tex_height - 40, 16, 16};
        projectiles.clear();
        enemies.clear();
        explosions.clear();
        circular_enemies.clear();
        stars.clear();
        for (int i = 0; i < num_stars; ++i) {
            stars.push_back({rand() % tex_width, rand() % tex_height, 1 + rand() % 3});
        }
        lives = 5;
        score = 0;
        regular_enemies_destroyed = 0;
    }

    void resetRound() {
        projectiles.clear();
        enemies.clear();
        explosions.clear();
        circular_enemies.clear();
    }

    void subtractLife() {
        lives--;
        if (lives <= 0) {
            is_game_over = true;
        } else {
            waiting_for_continue = true;
        }
        player_die = false;
    }

    void explode(int x, int y, ExplosionType type, bool d = false) {
        SDL_Color color = (type == ExplosionType::Red) ? SDL_Color{255, 0, 0, 255} : SDL_Color{0, 0, 255, 255};
        player_die = d;    
        explosions.push_back({x, y, 60, color, d});
    }

    void updateExplosions() {
        for (auto it = explosions.begin(); it != explosions.end();) {
            it->duration--;
            if (it->duration <= 0) {
                if(it->die)
                    subtractLife();
                it = explosions.erase(it);
            } else {
                ++it;
            }
        }
    }

    void drawExplosions(SDL_Renderer *renderer) {
        for (const auto &explosion : explosions) {
            SDL_SetRenderDrawColor(renderer, explosion.color.r, explosion.color.g, explosion.color.b, explosion.color.a);
            for (int i = 0; i < 20; ++i) {
                int dx = (rand() % 40) - 20;
                int dy = (rand() % 40) - 20;
                SDL_RenderDrawLine(renderer, explosion.x, explosion.y, explosion.x + dx, explosion.y + dy);
            }
        }
    }

    void drawGradientTriangle(SDL_Renderer *renderer, int x1, int y1, int x2, int y2, int x3, int y3) {
        if(player_die) return;

        for (int i = 0; i < (y3 - y1); ++i) {
            int r = 255 - (255 * i / (y3 - y1));
            int g = 255 * i / (y3 - y1);
            SDL_SetRenderDrawColor(renderer, r, g, 255, 255);
            SDL_RenderDrawLine(renderer, x1 - i, y1 + i, x1 + i, y1 + i);
        }
    }

    void drawGradientDiamond(SDL_Renderer *renderer, int cx, int cy, int half_width, int half_height) {
        for (int i = 0; i < half_height; ++i) {
            int r = 255 - (255 * i / half_height);
            int b = 255 * i / half_height;
            SDL_SetRenderDrawColor(renderer, r, 255, b, 255);
            SDL_RenderDrawLine(renderer, cx - i, cy - i, cx + i, cy - i);
        }
        for (int i = 0; i < half_height; ++i) {
            int g = 255 - (255 * i / half_height);
            int b = 255 * i / half_height;
            SDL_SetRenderDrawColor(renderer, 255, g, b, 255);
            SDL_RenderDrawLine(renderer, cx - i, cy + i, cx + i, cy + i);
        }
    }

    void drawGradientCircle(SDL_Renderer *renderer, int cx, int cy, int radius) {
        for (int r = radius; r > 0; --r) {
            int color = 255 * r / radius;
            SDL_SetRenderDrawColor(renderer, color, 0, color, 255);
            for (int angle = 0; angle < 360; ++angle) {
                double rad = angle * M_PI / 180.0;
                int x = cx + r * cos(rad);
                int y = cy + r * sin(rad);
                SDL_RenderDrawPoint(renderer, x, y);
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
        bg.loadTexture(win, win->util.getFilePath("data/spacelogo.png"));
    }

    virtual void draw(mx::mxWindow *win) override {
        static Uint32 previous_time = SDL_GetTicks();
        Uint32 current_time = SDL_GetTicks();
        static int alpha = 255;
        static bool fading_out = true;
        static bool done = false;

        SDL_RenderCopy(win->renderer, bg.wrapper().unwrap(), nullptr, nullptr);
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

    virtual void event(mx::mxWindow *win, SDL_Event &e) override {

    }
private:
    mx::Texture tex;
    mx::Texture bg;
};

class MainWindow : public mx::mxWindow {
public:
    MainWindow(std::string path, int tw, int th) : mx::mxWindow("Space", tw, th, false) {
        tex.createTexture(this, 640, 480);
      	setPath(path);
        setObject(new Intro());
		object->load(this);
    }
    
    ~MainWindow() override {}
    
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
    MainWindow main_window("/", 960, 720);
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
    int tw = 960, th = 720;
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
        mx::system_out << "mx: No path provided trying default current directory.\n";
        path = ".";
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