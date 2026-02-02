#include"mx.hpp"
#include"argz.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

class Game : public obj::Object {
public:
    Game() = default;
    ~Game() override {}

    constexpr static int canvasWidth = 800, canvasHeight = 600;
    constexpr static int bucketWidth = 80, bucketHeight = 20;
    constexpr static int bomberWidth = 60, bomberHeight = 40;
    constexpr static int bombRadius = 10, bombSpeed = 4;

    virtual void load(mx::mxWindow* win) override {
        bg.loadTexture(win, win->util.getFilePath("data/bg.png"));
        font.loadFont(win->util.getFilePath("data/font.ttf"), 24);
        win->text.setColor({255, 255, 255, 255});
        resetGame();
        lastUpdateTime = SDL_GetTicks();
    }

    void update(mx::mxWindow* win) {
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - lastUpdateTime >= 15) {
            lastUpdateTime = currentTime;

            if (gameOver) return;

            bomberX += bomberSpeed;
            if (bomberX + bomberWidth > canvasWidth || bomberX < 0) {
                bomberSpeed *= -1;
            }

            if (rand() % 100 < 2) {
                dropBomb();
            }

            moveBombs();
            checkCatch();
            handleKeys();
        }
    }

    virtual void draw(mx::mxWindow* win) override {
        SDL_SetRenderDrawColor(win->renderer, 0, 0, 0, 255);
        SDL_RenderCopy(win->renderer, bg.wrapper().unwrap(), nullptr, nullptr);
        drawBomber(win);
        drawBombs(win);
        drawBucket(win);
        drawScore(win);
        update(win);
        if (gameOver) {
            drawGameOver(win);
        }     
    }

    virtual void event(mx::mxWindow* win, SDL_Event& e) override {
        if (e.type == SDL_MOUSEMOTION) {
            bucketX = e.motion.x - bucketWidth / 2;
        } else if (e.type == SDL_MOUSEBUTTONDOWN && gameOver) {
            clickCount++;
            if (clickCount == 2) {
                resetGame();
                gameOver = false;
                clickCount = 0;
            }
        }
    }

private:
    mx::Texture bg;
    mx::Font font;

    Uint32 lastUpdateTime;
    int bucketX = (canvasWidth - bucketWidth) / 2;
    const int bucketY = canvasHeight - bucketHeight - 10;
    int bomberX = 0, bomberY = 10, bomberSpeed = 3;
    std::vector<std::pair<int, int>> bombs;
    int score = 0, lives = 5;
    bool gameOver = false;
    int clickCount = 0;

    void drawBucket(mx::mxWindow* win) {
        SDL_Rect bucket = {bucketX, bucketY, bucketWidth, bucketHeight};
        SDL_SetRenderDrawColor(win->renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(win->renderer, &bucket);
    }

    void drawBomber(mx::mxWindow* win) {
        SDL_Rect bomber = {bomberX, bomberY, bomberWidth, bomberHeight};
        SDL_SetRenderDrawColor(win->renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(win->renderer, &bomber);
    }

    void dropBomb() {
        bombs.emplace_back(bomberX + bomberWidth / 2, bomberY + bomberHeight);
    }

    void drawBombs(mx::mxWindow* win) {
        SDL_SetRenderDrawColor(win->renderer, 255, 255, 0, 255);
        for (const auto& bomb : bombs) {
            SDL_Rect bombRect = {bomb.first - bombRadius, bomb.second - bombRadius, bombRadius * 2, bombRadius * 2};
            SDL_RenderFillRect(win->renderer, &bombRect);
        }
    }

    void moveBombs() {
        for (auto& bomb : bombs) {
            bomb.second += bombSpeed;
        }
    }

    void checkCatch() {
        auto it = bombs.begin();
        while (it != bombs.end()) {
            if (it->second + bombRadius >= bucketY &&
                it->first >= bucketX && it->first <= bucketX + bucketWidth) {
                it = bombs.erase(it);
                score++;
            } else if (it->second > canvasHeight) {
                bombs.clear();
                lives--;
                if (lives > 0) {
                    resetRound();
                } else {
                    gameOver = true;
                }
                return;
            } else {
                ++it;
            }
        }
    }

    void drawScore(mx::mxWindow* win) {
        win->text.printText_Solid(font, 10, 10, "Score: " + std::to_string(score));
        win->text.printText_Solid(font, canvasWidth - 200, 10, "Lives: " + std::to_string(lives));
    }

    void drawGameOver(mx::mxWindow* win) {
        win->text.setColor({255,255,255,255});
        win->text.printText_Blended(font, canvasWidth / 2 - 100, canvasHeight / 2 - 20, "Game Over!");
        win->text.printText_Blended(font, canvasWidth / 2 - 150, canvasHeight / 2 + 20, "Double Click to Restart");
        win->text.setColor({255, 255, 255, 255});
    }

    void resetRound() {
        bomberX = 0;
        bomberSpeed = 3;
        bombs.clear();
        bucketX = (canvasWidth - bucketWidth) / 2;
    }

    void resetGame() {
        resetRound();
        score = 0;
        lives = 5;
    }

    void handleKeys(int moveSpeed = 10) {
         const Uint8* keyboardState = SDL_GetKeyboardState(NULL);
        if (keyboardState[SDL_SCANCODE_LEFT]) {
            bucketX -= moveSpeed;
            if (bucketX < 0) bucketX = 0; 
        }
        if (keyboardState[SDL_SCANCODE_RIGHT]) {
            bucketX += moveSpeed;
            if (bucketX + bucketWidth > canvasWidth) bucketX = canvasWidth - bucketWidth;
        }
    }
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
    MainWindow(std::string path, int tw, int th) : mx::mxWindow("Pong", tw, th, false) {
        tex.createTexture(this, 800, 600);
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
    MainWindow main_window("/", 800, 600);
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
    int tw = 800, th = 600;
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