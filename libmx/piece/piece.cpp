#include"mx.hpp"
#include"argz.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif


class Game : public obj::Object {
public:
    Game();
    ~Game() override;

    constexpr static int tex_width = 640, tex_height = 480;

    void load(mx::mxWindow* win) override;
    void draw(mx::mxWindow* win) override;
    void event(mx::mxWindow* win, SDL_Event& e) override;

private:
    int cur_screen = 0;
    SDL_Texture* draw_tex = nullptr;
    SDL_Texture* logo = nullptr;
    SDL_Texture* start = nullptr;
    SDL_Texture* cursor = nullptr;
    SDL_Texture* gamebg = nullptr;
    SDL_Texture* blocks[12] = { nullptr };
    SDL_Texture* alien = nullptr;
    Uint32 startTime = 0;
    Uint32 waitTime = 3000;
    int cursor_pos = 0;
    Uint32 startTime2 = 0;
    Uint32 waitTime2 = 1000;
    bool show_cursor = false;
    mx::Font the_font;
    class TileMatrix {
    public:
        struct GameData {
            unsigned long score;
            int lines;
            int speed;
            int lineamt;
            int linenum = 0;

            bool game_is_over = false;

            void gameover() {
                game_is_over = true;
            }

            bool isGameOver() const {
                return game_is_over;
            }

            GameData() {
                newgame();
            }

            void newgame() {
                score = 0;
                lines = 0;
                speed = 1500;
                lineamt = 0;
                game_is_over = false;
            }

            void addline() {
                lines++;
                score += 6;
                lineamt++;

                if (lineamt >= linenum) {
                    lineamt = 0;
                    speed -= 101;
                    if (speed < 100) {
                        speed = 100;
                    }
                }
            }
        };

        struct GameBlock {
            struct Color {
                int c1, c2, c3;

                void randcolor() {
                    c1 = rand() % 10;
                    c2 = rand() % 10;
                    c3 = rand() % 10;
                    if (c1 == 0) { c1++; }
                    if (c2 == 0) { c2++; }
                    if (c3 == 0) { c3++; }

                    if (c1 == c2 && c1 == c3) {
                        randcolor();
                        return;
                    }
                }

                void copycolor(const Color* c) {
                    c1 = c->c1;
                    c2 = c->c2;
                    c3 = c->c3;
                }

                void shiftcolor(bool dir) {
                    int ic1 = c1, ic2 = c2, ic3 = c3;

                    if (dir) {
                        c1 = ic3;
                        c2 = ic1;
                        c3 = ic2;
                    }
                    else {
                        c1 = ic2;
                        c2 = ic3;
                        c3 = ic1;
                    }
                }
            };

            Color color;
            Color nextcolor;
            int x, y;

            GameBlock() {
                color.randcolor();
                nextcolor.randcolor();
                x = 3;
                y = 0;
            }

            void NextBlock() {
                color.copycolor(&nextcolor);
                nextcolor.randcolor();
                x = 3;
                y = 0;
            }

            void MoveDown() {
                if (y + 2 < 16) {
                    y++;
                }
            }

            void MoveLeft() {
                if (x > 0) {
                    x--;
                }
            }

            void MoveRight() {
                if (x < 7) {
                    x++;
                }
            }
        };

        GameData Game;
        GameBlock block;
        int Tiles[17][8];  // 17 rows, 8 columns

        TileMatrix() {
            cleartiles();
        }

        void cleartiles() {
            for (int i = 0; i < 17; i++) {
                for (int j = 0; j < 8; j++) {
                    Tiles[i][j] = 0;
                }
            }
        }

        void init_matrix() {
            cleartiles();
            Game.newgame();
            block.NextBlock();
        }

        void setblock() {
            if (block.y <= 0) {
                Game.gameover();
            }

            if (block.y < 15) {  
                Tiles[block.y][block.x] = block.color.c1;
                Tiles[block.y + 1][block.x] = block.color.c2;
                Tiles[block.y + 2][block.x] = block.color.c3;
                block.NextBlock();
            }
        }

        void ProccessBlocks() {
            for (int z = 0; z < 8; z++) {
                for (int i = 0; i < 16; i++) {
                    if (Tiles[i][z] != 0 && Tiles[i + 1][z] == 0) {
                        Tiles[i + 1][z] = Tiles[i][z];
                        Tiles[i][z] = 0;
                    }
                }
            }
        }

        void clearBlocks() {
            for (int z = 0; z < 8; z++) {
                for (int i = 0; i < 17; i++) {
                    if (Tiles[i][z] == -1) {
                        Tiles[i][z] = 0;
                    }
                }
            }
        }
    };

    TileMatrix matrix;
    void newGame();
    void resetGame();
    void setScreen(int scr);

    void load_gfx(mx::mxWindow* win);
    void release_gfx();
    void draw_intro(mx::mxWindow* win);
    void draw_start(mx::mxWindow* win);
    void draw_game(mx::mxWindow* win);
    void draw_credits(mx::mxWindow* win);
    void draw_options(mx::mxWindow* win);
    void blockProc();
};

Game::Game() {
    
}

Game::~Game() {
    if (draw_tex != nullptr)
        SDL_DestroyTexture(draw_tex);
    release_gfx();
}

void Game::load(mx::mxWindow* win) {
    SDL_Renderer* ren = win->renderer;
    load_gfx(win);
    the_font.loadFont(win->util.getFilePath("mp_dat/font.ttf"), 14);
    draw_tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, tex_width, tex_height);
    if (!draw_tex) {
        std::cerr << "Could not create texture.\n";
        exit(EXIT_FAILURE);
    }
    resetGame();
}

void Game::load_gfx(mx::mxWindow* win) {
    int w, h;
    logo = win->util.loadTexture(win->renderer, "mp_dat/intro.png");
    start = win->util.loadTexture(win->renderer, "mp_dat/start.png");
    SDL_Color color = { 255, 0, 255 };
    cursor = win->util.loadTexture(win->renderer, "mp_dat/cursor.png", w, h, true, color);
    gamebg = win->util.loadTexture(win->renderer, "mp_dat/gamebg.png");
    const char* grid_block_paths[12] = {
        "mp_dat/block_black.png",
        "mp_dat/block_yellow.png",
        "mp_dat/block_orange.png",
        "mp_dat/block_ltblue.png",
        "mp_dat/block_dblue.png",
        "mp_dat/block_purple.png",
        "mp_dat/block_pink.png",
        "mp_dat/block_gray.png",
        "mp_dat/block_red.png",
        "mp_dat/block_green.png",
        "mp_dat/block_clear.png",
        nullptr
    };
    for (int i = 0; grid_block_paths[i] != nullptr; ++i) {
        blocks[i] = win->util.loadTexture(win->renderer, grid_block_paths[i]);
    }
    alien = win->util.loadTexture(win->renderer, "mp_dat/mp.alien.png");
}

void Game::release_gfx() {
    if (logo != nullptr)
        SDL_DestroyTexture(logo);
    if (start != nullptr)
        SDL_DestroyTexture(start);
    if (cursor != nullptr)
        SDL_DestroyTexture(cursor);
    if (gamebg != nullptr)
        SDL_DestroyTexture(gamebg);

    for (int i = 0; i < 11; ++i) {
        if (blocks[i] != nullptr)
            SDL_DestroyTexture(blocks[i]);
    }
    if (alien != nullptr) {
        SDL_DestroyTexture(alien);
    }
}

void Game::resetGame() {
    startTime = 0;
    waitTime = 3000;
    cur_screen = 0;
    cursor_pos = 0;
}

void Game::newGame() {
    cur_screen = 2;
    matrix.init_matrix();
    waitTime2 = 1000;
}

void Game::setScreen(int scr) {
    cur_screen = scr;
}

void Game::draw(mx::mxWindow* win) {
    SDL_Renderer* ren = win->renderer;
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
   
    switch (cur_screen) {
    case 0:
        draw_intro(win);
        break;
    case 1:
        draw_start(win);
        break;
    case 2:
        draw_game(win);
        break;
    case 4:
        draw_credits(win);
        break;
    case 3:
        draw_options(win);
        break;
    }
}

void Game::event(mx::mxWindow* win, SDL_Event& e) {
    SDL_Rect ir = { 0, 0, win->width, win->height };
    static int x1 = 0, y1x = 0, x2 = 0, y2 = 0;
    static const int drag_threshold = 15;
    if (cur_screen == 2 && e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        x1 = e.button.x - ir.x;
        y1x = e.button.y - ir.y;
    }
    else if (cur_screen == 2 && e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
        if (!(e.button.x >= ir.x && e.button.x <= ir.x + ir.w && e.button.y >= ir.y && e.button.y <= ir.y + ir.h)) {
            return;
        }
        x2 = e.button.x - ir.x;
        y2 = e.button.y - ir.y;
        if (abs(y1x - y2) > abs(x1 - x2)) {
            if (y1x < y2 && abs(y1x - y2) > drag_threshold) {
                x1 = x2;
                y1x = y2;
                matrix.block.MoveDown();
            }
            else if (y1x > y2 && abs(y2 - y1x) > drag_threshold) {
                matrix.block.color.shiftcolor(true);
                x1 = x2;
                y1x = y2;
            }
        }
        else {
            if (x1 < x2 && abs(x1 - x2) > drag_threshold) {
                x1 = x2;
                y1x = y2;
                matrix.block.MoveRight();
            }
            else if (x1 > x2 && abs(x2 - x1) > drag_threshold) {
                matrix.block.MoveLeft();
                x1 = x2;
                y1x = y2;
            }
        }
    }

    switch (e.type) {
    case SDL_MOUSEMOTION: {
        if (cur_screen == 1) {
            SDL_Rect startMeRect;
            int rectWidth = static_cast<int>(ir.w * 0.2);
            int rectHeight = static_cast<int>(ir.h * 0.1);
            startMeRect.x = (ir.w / 2) - (rectWidth / 2);
            startMeRect.y = (ir.h / 2) - (rectHeight / 2);
            startMeRect.w = rectWidth;
            startMeRect.h = rectHeight;
            SDL_Point p = { e.motion.x - ir.x, e.motion.y - ir.y };
            if (SDL_PointInRect(&p, &startMeRect)) {
                show_cursor = true;
            }
            else {
                show_cursor = false;
            }
        }
        break;
    }
    case SDL_MOUSEBUTTONDOWN: {
        if (e.button.button == SDL_BUTTON_LEFT) {
            if (e.button.x >= ir.x && e.button.x <= ir.x + ir.w && e.button.y >= ir.y && e.button.y <= ir.y + ir.h) {
                if (cur_screen == 1) {
                    SDL_Rect startMeRect;
                    int rectWidth = static_cast<int>(ir.w * 0.2);
                    int rectHeight = static_cast<int>(ir.h * 0.1);
                    startMeRect.x = (ir.w / 2) - (rectWidth / 2);
                    startMeRect.y = (ir.h / 2) - (rectHeight / 2);
                    startMeRect.w = rectWidth;
                    startMeRect.h = rectHeight;

                    SDL_Point p = { e.button.x - ir.x, e.button.y - ir.y };
                    if (SDL_PointInRect(&p, &startMeRect)) {
                        newGame();
                    }
                }
            }
        }
        break;
    }
    case SDL_KEYDOWN:
        switch (e.key.keysym.sym) {
        case SDLK_UP:
            if (cur_screen == 2) {
                matrix.block.color.shiftcolor(true);
            }
            break;
        case SDLK_DOWN:
            if (cur_screen == 2) {
                matrix.block.MoveDown();
            }
            break;
        case SDLK_LEFT:
            if (cur_screen == 2) {
                matrix.block.MoveLeft();
            }
            break;
        case SDLK_RIGHT:
            if (cur_screen == 2) {
                matrix.block.MoveRight();
            }
            break;
        case SDLK_SPACE:
            if (cur_screen == 4 || cur_screen == 3) {
                cur_screen = 1;
            }
            break;
        case SDLK_RETURN:
            if (cur_screen == 1) {
                switch (cursor_pos) {
                case 0:
                    newGame();
                    break;
                }
            }
            break;
        }
        break;
    }
}

void Game::draw_intro(mx::mxWindow* win) {
    SDL_Renderer* ren = win->renderer;
    SDL_Rect rc = { 0, 0, tex_width, tex_height };

    if (startTime == 0) {
        startTime = SDL_GetTicks();
    }

    Uint32 elapsedTime = SDL_GetTicks() - startTime;
    if (elapsedTime > waitTime) {
        elapsedTime = waitTime;
    }

    Uint8 alphaLogo = 255 - (255 * elapsedTime / waitTime);
    Uint8 alphaStart = 255 * elapsedTime / waitTime;

    SDL_SetTextureAlphaMod(logo, alphaLogo);
    SDL_RenderCopy(ren, logo, nullptr, &rc);

    SDL_SetTextureAlphaMod(start, alphaStart);
    SDL_RenderCopy(ren, start, nullptr, &rc);

    if (elapsedTime >= waitTime) {
        cur_screen = 1;
        startTime = 0;
    }
}

void Game::draw_start(mx::mxWindow* win) {
    SDL_Renderer* ren = win->renderer;
    SDL_Rect rc = { 0, 0, tex_width, tex_height };
    SDL_RenderCopy(ren, start, nullptr, &rc);
}

void Game::draw_credits(mx::mxWindow* win) {
    SDL_Renderer* ren = win->renderer;
    SDL_Rect rc = { 0, 0, tex_width, tex_height };
    SDL_RenderCopy(ren, start, nullptr, &rc);
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    SDL_Rect rc2 = { 35, 92, tex_width - 35, tex_height - 130 };
    SDL_RenderFillRect(ren, &rc2);
    SDL_Color color = { 255, 255, 255, 255 };
    SDL_Color color2 = { 255, 0, 0, 255 };
    SDL_Color color3 = { 0, 0, 255, 255 };

    win->util.printText(ren, the_font.wrapper().unwrap(), 60, 110, "MasterPiece MX Edition", color);
    win->util.printText(ren, the_font.wrapper().unwrap(), 60, 135, "written by Jared Bruni", color2);
    win->util.printText(ren, the_font.wrapper().unwrap(), (tex_width / 2) - (200 / 2) + 15, 200, "[ Press Space to Return ]", color3);

    SDL_Rect rc3 = { (tex_width / 2) - (150 / 2), 225, 150, 150 };
    SDL_RenderCopy(ren, alien, nullptr, &rc3);
}

void Game::draw_options(mx::mxWindow* win) {
    SDL_Renderer* ren = win->renderer;
    SDL_Rect rc = { 0, 0, tex_width, tex_height };
    SDL_RenderCopy(ren, start, nullptr, &rc);
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    SDL_Rect rc2 = { 35, 92, tex_width - 35, tex_height - 130 };
    SDL_RenderFillRect(ren, &rc2);
    SDL_Color color = { 255, 255, 255, 255 };
    win->util.printText(ren, the_font.wrapper().unwrap(), 60, 110, "[ Options Menu - Space to Return ]", color);
}

void Game::draw_game(mx::mxWindow* win) {
    SDL_Renderer* ren = win->renderer;
    SDL_Rect rc = { 0, 0, tex_width, tex_height };
    SDL_RenderCopy(ren, gamebg, nullptr, &rc);

    const int STARTPOSX = 185;
    const int STARTPOSY = 95;
    int x = STARTPOSX, y = STARTPOSY;
    int blockWidth = 32;
    int blockHeight = 16;

    for (int i = 0; i < 17; i++) {
        for (int j = 0; j < 8; j++) {
            if (matrix.Tiles[i][j] != -1) {
                if (matrix.Tiles[i][j] != 0) {
                    SDL_Rect dstRect = { x, y, blockWidth, blockHeight };
                    SDL_RenderCopy(ren, blocks[matrix.Tiles[i][j]], nullptr, &dstRect);
                }
                x += blockWidth;
            }
            else {
                SDL_Rect dstRect = { x, y, blockWidth, blockHeight };
                SDL_RenderCopy(ren, blocks[1 + (rand() % 8)], nullptr, &dstRect);
                x += blockWidth;
            }
        }
        x = STARTPOSX;
        y += blockHeight;
    }

    int bx = 510, by = 200;

    SDL_Rect nextBlockRect1 = { bx, by, blockWidth, blockHeight };
    SDL_RenderCopy(ren, blocks[matrix.block.nextcolor.c1], nullptr, &nextBlockRect1);

    SDL_Rect nextBlockRect2 = { bx, by + blockHeight, blockWidth, blockHeight };
    SDL_RenderCopy(ren, blocks[matrix.block.nextcolor.c2], nullptr, &nextBlockRect2);

    SDL_Rect nextBlockRect3 = { bx, by + 2 * blockHeight, blockWidth, blockHeight };
    SDL_RenderCopy(ren, blocks[matrix.block.nextcolor.c3], nullptr, &nextBlockRect3);

    auto getcords = [](int r, int c, int& rx, int& ry) {
        const int STARTPOSX = 185;
        const int STARTPOSY = 95;
        rx = STARTPOSX + c * 32;
        ry = STARTPOSY + r * 16;
    };

    getcords(matrix.block.y, matrix.block.x, bx, by);
    SDL_Rect blockRect1 = { bx, by, blockWidth, blockHeight };
    SDL_RenderCopy(ren, blocks[matrix.block.color.c1], nullptr, &blockRect1);

    getcords(matrix.block.y + 1, matrix.block.x, bx, by);
    SDL_Rect blockRect2 = { bx, by, blockWidth, blockHeight };
    SDL_RenderCopy(ren, blocks[matrix.block.color.c2], nullptr, &blockRect2);

    getcords(matrix.block.y + 2, matrix.block.x, bx, by);
    SDL_Rect blockRect3 = { bx, by, blockWidth, blockHeight };
    SDL_RenderCopy(ren, blocks[matrix.block.color.c3], nullptr, &blockRect3);

    SDL_Color white{ 255,255,255,255 };
    win->util.printText(ren, the_font.wrapper().unwrap(), 200, 60, "Score: " + std::to_string(matrix.Game.score), white);
    win->util.printText(ren, the_font.wrapper().unwrap(), 310, 60, "Lines: " + std::to_string(matrix.Game.lines), white);
    blockProc();

    if (startTime2 == 0) {
        startTime2 = SDL_GetTicks();
    }

    waitTime2 = matrix.Game.speed;

    Uint32 elapsedTime = SDL_GetTicks() - startTime2;
    if (elapsedTime > waitTime2) {
        elapsedTime = waitTime2;
    }
    if (elapsedTime >= waitTime2) {
        matrix.block.MoveDown();
        startTime2 = 0;
        matrix.clearBlocks();
    }

    if (matrix.Game.isGameOver())
        cur_screen = 0;
}

void Game::blockProc() {
    if (matrix.block.y + 2 >= 16) {
        matrix.setblock();
        return;
    }

    if (matrix.block.y + 3 < 17 && matrix.Tiles[matrix.block.y + 3][matrix.block.x] != 0) {
        matrix.setblock();
        return;
    }

    for (int i = 0; i < 17; i++) {
        for (int j = 0; j < 8 - 2; j++) {
            int cur_color = matrix.Tiles[i][j];
            if (cur_color != 0 && cur_color != -1) {
                if (matrix.Tiles[i][j + 1] == cur_color && matrix.Tiles[i][j + 2] == cur_color) {
                    matrix.Game.addline();
                    matrix.Tiles[i][j] = -1;
                    matrix.Tiles[i][j + 1] = -1;
                    matrix.Tiles[i][j + 2] = -1;
                    return;
                }
            }
        }
    }

    for (int z = 0; z < 8; z++) {
        for (int q = 0; q < 17 - 2; q++) {
            int cur_color = matrix.Tiles[q][z];
            if (cur_color != 0 && cur_color != -1) {
                if (matrix.Tiles[q + 1][z] == cur_color && matrix.Tiles[q + 2][z] == cur_color) {
                    matrix.Tiles[q][z] = -1;
                    matrix.Tiles[q + 1][z] = -1;
                    matrix.Tiles[q + 2][z] = -1;
                    matrix.Game.addline();
                    return;
                }
            }
        }
    }

    for (int p = 0; p < 8; p++) {
        for (int w = 0; w < 17; w++) {
            int cur_color = matrix.Tiles[w][p];

            if (cur_color != 0 && cur_color != -1) {
                if (w + 2 < 17 && p + 2 < 8) {
                    if (matrix.Tiles[w + 1][p + 1] == cur_color && matrix.Tiles[w + 2][p + 2] == cur_color) {
                        matrix.Tiles[w][p] = -1;
                        matrix.Tiles[w + 1][p + 1] = -1;
                        matrix.Tiles[w + 2][p + 2] = -1;
                        matrix.Game.addline();
                    }
                }
                if (w - 2 >= 0 && p - 2 >= 0) {
                    if (matrix.Tiles[w - 1][p - 1] == cur_color && matrix.Tiles[w - 2][p - 2] == cur_color) {
                        matrix.Tiles[w][p] = -1;
                        matrix.Tiles[w - 1][p - 1] = -1;
                        matrix.Tiles[w - 2][p - 2] = -1;
                        matrix.Game.addline();
                    }
                }
                if (w - 2 >= 0 && p + 2 < 8) {
                    if (matrix.Tiles[w - 1][p + 1] == cur_color && matrix.Tiles[w - 2][p + 2] == cur_color) {
                        matrix.Tiles[w][p] = -1;
                        matrix.Tiles[w - 1][p + 1] = -1;
                        matrix.Tiles[w - 2][p + 2] = -1;
                        matrix.Game.addline();
                    }
                }
                if (w + 2 < 17 && p - 2 >= 0) {
                    if (matrix.Tiles[w + 1][p - 1] == cur_color && matrix.Tiles[w + 2][p - 2] == cur_color) {
                        matrix.Tiles[w][p] = -1;
                        matrix.Tiles[w + 1][p - 1] = -1;
                        matrix.Tiles[w + 2][p - 2] = -1;
                        matrix.Game.addline();
                    }
                }
            }
        }
    }

    matrix.ProccessBlocks();
}

class Intro : public obj::Object {
public:
    Intro() {}
    ~Intro() override {}

    virtual void load(mx::mxWindow *win) override {
        tex.loadTexture(win, win->util.getFilePath("mp_dat/logo.png"));
        bg.loadTexture(win, win->util.getFilePath("mp_dat/bg.png"));
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
    
    MainWindow(std::string path, int tw, int th) : mx::mxWindow("MasterPiece", tw, th, false) {
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