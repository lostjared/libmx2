#include "mx.hpp"
#include "argz.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <ctime>

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
    }
    
    virtual void draw(mx::mxWindow *win) override {
        tour.drawBoard(win);
        tour.drawKnight(win, knight);
        win->text.setColor({255, 255, 255, 255});
        win->text.printText_Blended(the_font, TEXT_OFFSET_X, TEXT_OFFSET_Y, "Knights Tour - Tap Space, Press Return to Reset");
        win->text.printText_Blended(the_font, 400, TEXT_OFFSET_Y, "Moves: " + std::to_string(tour.getMoves()));
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
        }
    }
private:
    mx::Font the_font;
    mx::Texture knight;
    
    static constexpr int TEXT_OFFSET_X = 15;
    static constexpr int TEXT_OFFSET_Y = 5;
    static constexpr int TEXT_SIZE = 14;
    
    Tour tour;
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
    SDL_RenderCopy(win->renderer, tex.handle(), nullptr, &dst);
}

class MainWindow : public mx::mxWindow {
public:
    static constexpr int WINDOW_WIDTH = 640;
    static constexpr int WINDOW_HEIGHT = 480;
    
    MainWindow(const std::string &path)
    : mx::mxWindow("Knights Tour", WINDOW_WIDTH, WINDOW_HEIGHT, false) {
        setPath(path);
        setObject(new KnightsTour());
        object->load(this);
    }
    
    ~MainWindow() override {}
    
    virtual void event(SDL_Event &e) override {

    }
    
    virtual void draw(SDL_Renderer *renderer) override {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        object->draw(this);
        SDL_RenderPresent(renderer);
    }
};

int main(int argc, char **argv) {
    
    Argz<std::string> parser(argc, argv);
    
    parser.addOptionSingle('h', "Display help message")
          .addOptionSingleValue('p', "assets path")
          .addOptionDoubleValue('P', "path", "assets path");
    
    // Parse arguments
    Argument<std::string> arg;
    std::string path;
    int value = 0;
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
            }
        }
    } catch (const ArgException<std::string>& e) {
        mx::system_err << e.text() << std::endl;
    }

    if(!path.empty()) {
        mx::system_err << "KnightsTour: Requires path variable to assets...\n";
        mx::system_err.flush();
        exit(EXIT_FAILURE);
    }

    try {
        MainWindow main_window(path);
        main_window.loop();
        
    } catch(const mx::Exception &e) {
        mx::system_err << "mx: " << e.text() << "\n";
    }

    return 0;
}
