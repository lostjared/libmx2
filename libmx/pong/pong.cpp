#include"mx.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

class Game : public obj::Object {
public:
    Game() = default;
    ~Game() override {}

    constexpr static int tex_width = 640, tex_height = 480;

    virtual void load(mx::mxWindow* win) override {
        bg.loadTexture(win, win->util.getFilePath("data/bg.png"));
        the_font.loadFont(win->util.getFilePath("data/font.ttf"), 14);
        player_paddle = {50, tex_height / 2 - 50, 10, 100};
        ai_paddle = {tex_width - 60, tex_height / 2 - 50, 10, 100};
        ball = {tex_width / 2 - 10, tex_height / 2 - 10, 20, 20};
        ball_vel_x = -5;
        ball_vel_y = -5;
    }

    void update(mx::mxWindow *win) {
        const Uint8* state = SDL_GetKeyboardState(NULL);
        if (state[SDL_SCANCODE_UP]) {
            player_paddle.y -= paddle_speed;
        }
        if (state[SDL_SCANCODE_DOWN]) {
            player_paddle.y += paddle_speed;
        }

        if (player_paddle.y < 0)
            player_paddle.y = 0;
        if (player_paddle.y + player_paddle.h > tex_height)
            player_paddle.y = tex_height - player_paddle.h;

        if (ball.y < ai_paddle.y + ai_paddle.h / 2) {
            ai_paddle.y -= paddle_speed;
        } else if (ball.y > ai_paddle.y + ai_paddle.h / 2) {
            ai_paddle.y += paddle_speed;
        }

        if (ai_paddle.y < 0)
            ai_paddle.y = 0;
        if (ai_paddle.y + ai_paddle.h > tex_height)
            ai_paddle.y = tex_height - ai_paddle.h;

        ball.x += ball_vel_x;
        ball.y += ball_vel_y;

        if (ball.y <= 0 || ball.y + ball.h >= tex_height) {
            ball_vel_y = -ball_vel_y;
        }

        if (SDL_HasIntersection(&ball, &player_paddle)) {
            ball_vel_x = -ball_vel_x;
            ball.x = player_paddle.x + player_paddle.w;
        } else if (SDL_HasIntersection(&ball, &ai_paddle)) {
            ball_vel_x = -ball_vel_x;
            ball.x = ai_paddle.x - ball.w;
        }

        if (ball.x <= 0 || ball.x + ball.w >= tex_width) {

            if(ball.x <= 0) {
                player2_score ++;
            } else if(ball.x + ball.w >= tex_width) {
                player1_score ++;
            }

            ball = {tex_width / 2 - 10, tex_height / 2 - 10, 20, 20};
            ball_vel_x = -ball_vel_x;

        }
    }

    virtual void draw(mx::mxWindow* win) override {
        SDL_RenderCopy(win->renderer, bg.wrapper().unwrap(), nullptr, nullptr);
        static Uint32 last_update_time = SDL_GetTicks();
        Uint32 current_time = SDL_GetTicks();
        Uint32 elapsed_time = current_time - last_update_time;
        if (elapsed_time >= 20) {
            update(win);
            last_update_time = current_time;
        }
        SDL_SetRenderDrawColor(win->renderer, 255, 255, 255, 255); 
        SDL_RenderFillRect(win->renderer, &player_paddle);
        SDL_RenderFillRect(win->renderer, &ai_paddle);
        SDL_RenderFillRect(win->renderer, &ball);

        win->text.printText_Solid(the_font, 15, 15, "Player 1: " + std::to_string(player1_score));
        win->text.printText_Solid(the_font, 15, 35, "Player 2: " + std::to_string(player2_score));
    }

    virtual void event(mx::mxWindow* win, SDL_Event& e) override {
        
    }

private:
    mx::Texture bg;
    mx::Font the_font;
    SDL_Rect player_paddle = {0};
    SDL_Rect ai_paddle = {0};
    SDL_Rect ball = {0};
    int ball_vel_x = 0;
    int ball_vel_y = 0;
    const int paddle_speed = 5;
    int player1_score = 0;
    int player2_score = 0;
};

class Intro : public obj::Object {
public:
    Intro() {}
    ~Intro() override {}

    virtual void load(mx::mxWindow *win) override {
        tex.loadTexture(win, win->util.getFilePath("data/logo.png"));
        bg.loadTexture(win, win->util.getFilePath("data/bg.png"));
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
    
    MainWindow(std::string path) : mx::mxWindow("Pong", 960, 720, false) {
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
	if(argc == 2) {
        try {
        MainWindow main_window(argv[1]);
    	main_window.loop();
        } catch(const mx::Exception &e) {
            mx::system_err << "mx: Exception: " << e.text() << "\n";
            mx::system_err.flush();
            exit(EXIT_FAILURE);
        }
	} else {
		mx::system_err << "Error requies one argument to path of font.\n";
	}
    return 0;
#endif
}