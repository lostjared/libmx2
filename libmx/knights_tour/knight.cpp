#include "mx.hpp"
#include "argz.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <ctime>
#include<functional>
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

class Tour {
public:
    Tour();
    void drawBoard(mx::mxWindow* win);
    void drawKnight(mx::mxWindow* win, mx::Texture& tex);
    void nextMove();
    void resetTour();
    int getMoves() const { return moves; }
private:
    struct Position {
        int row;
        int col;
        constexpr Position(int r = 0, int c = 0) : row(r), col(c) {}
    };
    
    void initializeBoard();
    void clearBoard();
    bool isValidMove(const Position& pos) const;
    int getDegree(const Position& pos) const;
    bool solveKnightsTour(Position pos, int moveCount);
    
    static constexpr int BOARD_SIZE = 8;
    static constexpr int TOTAL_MOVES = BOARD_SIZE * BOARD_SIZE + 1;
    static constexpr int START_X = 100;
    static constexpr int START_Y = 30;
    static constexpr int CELL_SIZE = 55;
    static constexpr int CELL_DRAW_SIZE = 50;
    static constexpr int KNIGHT_SIZE = 35;
    
    std::vector<std::vector<int>> board;
    std::vector<Position> moveSequence;
    Position knightPos;
    int moves;
    bool tourOver;
    
    static constexpr std::array<int, 8> horizontal = {2, 1, -1, -2, -2, -1, 1, 2};
    static constexpr std::array<int, 8> vertical = {-1, -2, -2, -1, 1, 2, 2, 1};
};


class KnightsTour : public obj::Object {
public:
    KnightsTour() {}
    ~KnightsTour() override {}
    
    virtual void load(mx::mxWindow *win) override {
        the_font.loadFont(win->util.getFilePath("data/font.ttf"), TEXT_SIZE);
        int w = 0, h = 0;
        knight.loadTexture(win, win->util.getFilePath("data/knight.png"), w, h, true, {255, 255, 255, 255});
        if(mx::Joystick::joysticks() > 0) {
            if(stick.open(0)) {
                mx::system_out << "Joystick opened: " << stick.name() << "\n";
            } else {
                mx::system_out << "Could not open joystick..\n";
            }
        }
    }
    
    virtual void draw(mx::mxWindow *win) override {
        tour.drawBoard(win);
        tour.drawKnight(win, knight);
        win->text.setColor({255, 255, 255, 255});
        if(tour.getMoves() < 65) {
            win->text.printText_Blended(the_font, TEXT_OFFSET_X, TEXT_OFFSET_Y, "Knights Tour - Tap Space, Press Return to Reset");
            win->text.printText_Blended(the_font, 400, TEXT_OFFSET_Y, "Moves: " + std::to_string(tour.getMoves()));
        } else {
            win->text.printText_Blended(the_font, TEXT_OFFSET_X, TEXT_OFFSET_Y, "-[ Tour Complete ]- Press Return to Reset");
        }
    }
    
    virtual void event(mx::mxWindow *win, SDL_Event &e) override {
        switch (e.type) {
            case SDL_KEYDOWN:
                switch (e.key.keysym.sym) {
                    case SDLK_SPACE:
                        tour.nextMove();
                        break;
                    case SDLK_RETURN:
                        tour.resetTour();
                        break;
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                switch(e.button.button) {
                    case SDL_BUTTON_LEFT:
                        tour.nextMove();
                    break;
                    case SDL_BUTTON_RIGHT:
                        tour.resetTour();
                    break;
                }
            break;
            case SDL_JOYBUTTONDOWN:
                switch(e.jbutton.button) {
                    case 1:
                        tour.nextMove();
                    break;
                    case 2:
                        tour.resetTour();
                    break;
                }
            break;
        }
    }
private:
    mx::Font the_font;
    mx::Texture knight;
    mx::Joystick stick;
    
    static constexpr int TEXT_OFFSET_X = 15;
    static constexpr int TEXT_OFFSET_Y = 5;
    static constexpr int TEXT_SIZE = 14;
    
    Tour tour;
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
            win->setObject(new KnightsTour());
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
};

Tour::Tour() : moves(1), tourOver(false) {
    srand(static_cast<unsigned int>(time(0)));
    initializeBoard();
    resetTour();
}

void Tour::initializeBoard() {
    board.resize(BOARD_SIZE, std::vector<int>(BOARD_SIZE, 0));
}

void Tour::clearBoard() {
    for (auto& row : board) {
        std::fill(row.begin(), row.end(), 0);
    }
}

bool Tour::isValidMove(const Position& pos) const {
    return pos.row >= 0 && pos.row < BOARD_SIZE &&
    pos.col >= 0 && pos.col < BOARD_SIZE &&
    board[pos.row][pos.col] == 0;
}

int Tour::getDegree(const Position& pos) const {
    int count = 0;
    for (int i = 0; i < 8; ++i) {
        int newRow = pos.row + vertical[i];
        int newCol = pos.col + horizontal[i];
        if (isValidMove(Position(newRow, newCol))) {
            ++count;
        }
    }
    return count;
}

bool Tour::solveKnightsTour(Position pos, int moveCount) {
    if (moveCount == TOTAL_MOVES) {
        return true;
    }
    
    std::vector<std::pair<int, Position>> nextMoves;
    
    for (int i = 0; i < 8; ++i) {
        Position newPos(pos.row + vertical[i], pos.col + horizontal[i]);
        if (isValidMove(newPos)) {
            int degree = getDegree(newPos);
            nextMoves.emplace_back(degree, newPos);
        }
    }
    
    std::sort(nextMoves.begin(), nextMoves.end(),
              [](const std::pair<int, Position>& a, const std::pair<int, Position>& b) {
        return a.first < b.first;
    });
    
    for (const auto& [degree, nextPos] : nextMoves) {
        board[nextPos.row][nextPos.col] = moveCount;
        moveSequence.push_back(nextPos);
        
        if (solveKnightsTour(nextPos, moveCount + 1)) {
            return true;
        } else {
            board[nextPos.row][nextPos.col] = 0;
            moveSequence.pop_back();
        }
    }
    
    return false;
}

void Tour::resetTour() {
    clearBoard();
    knightPos = Position(rand() % BOARD_SIZE, rand() % BOARD_SIZE);
    board[knightPos.row][knightPos.col] = 1;
    moveSequence.clear();
    moveSequence.push_back(knightPos);
    solveKnightsTour(knightPos, 2);
    moves = 1;
    tourOver = false;
}

void Tour::nextMove() {
    if (moves == TOTAL_MOVES) {
        return;
    }
    
    if (!moveSequence.empty()) {
        Position nextPos = moveSequence[moves - 1];
        board[knightPos.row][knightPos.col] = -1;
        knightPos = nextPos;
        board[knightPos.row][knightPos.col] = moves;
        ++moves;
        
        if (moves == TOTAL_MOVES) {
            tourOver = true;
        }
    }
}

void Tour::drawBoard(mx::mxWindow* win) {
    SDL_SetRenderDrawColor(win->renderer, 0, 0, 0, 255);
    SDL_RenderClear(win->renderer);
    for (int i = 0; i < BOARD_SIZE; ++i) {
        for (int j = 0; j < BOARD_SIZE; ++j) {
            SDL_Rect rect = {START_X + j * CELL_SIZE, START_Y + i * CELL_SIZE, CELL_DRAW_SIZE, CELL_DRAW_SIZE};
            if (board[i][j] == -1) {
                SDL_SetRenderDrawColor(win->renderer, 0, 0, 0, 255);
            } else if ((i + j) % 2 == 0) {
                SDL_SetRenderDrawColor(win->renderer, 255, 255, 255, 255);
            } else {
                SDL_SetRenderDrawColor(win->renderer, 255, 0, 0, 255);
            }
            SDL_RenderFillRect(win->renderer, &rect);
        }
    }
}

void Tour::drawKnight(mx::mxWindow* win, mx::Texture& tex) {
    SDL_Rect dst = {START_X + knightPos.col * CELL_SIZE + 5, START_Y + knightPos.row * CELL_SIZE + 5, KNIGHT_SIZE, KNIGHT_SIZE};
    SDL_RenderCopy(win->renderer, tex.wrapper().unwrap(), nullptr, &dst);
}

class MainWindow : public mx::mxWindow {
public:
    mx::Texture bg_tex;

    MainWindow(const std::string &path, int tex_w, int tex_h)
    : mx::mxWindow("Knights Tour", tex_w, tex_h, false) {
        bg_tex.createTexture(this, 640, 480);
        setPath(path);
        setObject(new Intro());
        object->load(this);
    }
    
    ~MainWindow() override {}
    
    virtual void event(SDL_Event &e) override {

    }
    
    virtual void draw(SDL_Renderer *renderer) override {
        SDL_SetRenderTarget(renderer,bg_tex.wrapper().unwrap());
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        object->draw(this);
        SDL_SetRenderTarget(renderer, nullptr);
        SDL_RenderCopy(renderer, bg_tex.wrapper().unwrap(), nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }
};

MainWindow *main_win = nullptr;

void eventProc() {
    if(main_win) {
        main_win->proc();
    }
}

int main(int argc, char **argv) {
    
    Argz<std::string> parser(argc, argv);
    
    parser.addOptionSingle('h', "Display help message")
          .addOptionSingleValue('p', "assets path")
          .addOptionDoubleValue('P', "path", "assets path")
          .addOptionSingleValue('r',"Resolution WidthxHeight")
          .addOptionDoubleValue('R',"resolution", "Resolution WidthxHeight");
    
    // Parse arguments
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
        mx::system_err << e.text() << std::endl;
    }
#ifndef __EMSCRIPTEN__
    if(path.empty()) {
        mx::system_err << "KnightsTour: Requires path variable to assets...\n";
        mx::system_err.flush();
        exit(EXIT_FAILURE);
    }
#endif
    try {
        MainWindow main_window(path, tw, th);
#ifdef __EMSCRIPTEN__
        main_win =  &main_window;
        emscripten_set_main_loop(eventProc, 0, 1);
#else
        main_window.loop();
#endif
    } catch(const mx::Exception &e) {
        mx::system_err << "mx: " << e.text() << "\n";
    }

    return 0;
}
