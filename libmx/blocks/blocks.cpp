#include"mx.hpp"
#include"argz.hpp"
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif
#include"mx.hpp"

#include <vector>
#include <string>
#include <ctime>
#include <cstdlib>

class Game : public obj::Object {
public:
    Game() : pieceX(0), pieceY(0), isGameOver(false), score(0), lastFallTime(0), fallInterval(1000) {}
    ~Game() override {}

    void load(mx::mxWindow* win) override {
        the_font.loadFont(win->util.getFilePath("data/font.ttf"), 14);
        bg.loadTexture(win, win->util.getFilePath("data/bg.png"));
        resetGame();
    }

    void update(mx::mxWindow* win) {
        if (!isGameOver) {
            Uint32 currentTime = SDL_GetTicks();
            if (currentTime - lastFallTime >= fallInterval) {
                movePieceDown();
                lastFallTime = currentTime;
            }
        }
    }

    void draw(mx::mxWindow* win) override {
        SDL_Renderer* renderer = win->renderer;

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        
        if (isGameOver) {
            TTF_Font* font = the_font.wrapper().unwrap();
            SDL_Color color = {255, 0, 0};
            SDL_Surface* surface = TTF_RenderText_Blended(font, "Game Over", color);
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            int textW = 0, textH = 0;
            SDL_QueryTexture(texture, nullptr, nullptr, &textW, &textH);

            int windowWidth = 300 * 2;
            int windowHeight = 630 * 2;

            SDL_Rect textRect = {(windowWidth - textW) / 2, (windowHeight - textH) / 2, textW, textH};
            SDL_RenderCopy(renderer, texture, nullptr, &textRect);
            SDL_DestroyTexture(texture);
            SDL_FreeSurface(surface);
            return;
        }

        SDL_RenderCopy(win->renderer, bg.wrapper().unwrap(), nullptr, nullptr);
        
        int windowWidth = 300 * 2;
        int windowHeight = 630 * 2;
        SDL_Rect gameArea = {50, 50, windowWidth - 100, windowHeight - 100};
        int blockSize = gameArea.w / cols;
        for (int row = 0; row < rows; ++row) {
            for (int col = 0; col < cols; ++col) {
                SDL_Rect rect = {gameArea.x + col * blockSize, gameArea.y + row * blockSize, blockSize, blockSize};
                if (grid[row][col] == 1) {
                    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                    SDL_RenderFillRect(renderer, &rect);
                } else {
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    SDL_RenderDrawRect(renderer, &rect);
                }
            }
        }

        for (int row = 0; row < static_cast<int>(currentPiece.size()); ++row) {
            for (int col = 0; col < static_cast<int>(currentPiece[0].size()); ++col) {
                if (currentPiece[row][col] == 1) {
                    SDL_Rect rect = {gameArea.x + (pieceX + col) * blockSize, gameArea.y + (pieceY + row) * blockSize, blockSize, blockSize};
                    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
                    SDL_RenderFillRect(renderer, &rect);
                }
            }
        }

        TTF_Font* font = the_font.wrapper().unwrap();
        SDL_Color textColor = {255,255,255,255};
        std::string scoreText = "Score: " + std::to_string(score);
        SDL_Surface* textSurface = TTF_RenderText_Blended(font, scoreText.c_str(), textColor);
        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        int textW = 0, textH = 0;
        SDL_QueryTexture(textTexture, nullptr, nullptr, &textW, &textH);
        SDL_Rect textRect = {gameArea.x + 5, gameArea.y - textH - 5, textW, textH};
        SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);
        SDL_DestroyTexture(textTexture);
        SDL_FreeSurface(textSurface);
        update(win);
    }

    
    void event(mx::mxWindow* win, SDL_Event& e) override {
        if (e.type == SDL_KEYDOWN && !isGameOver) {
            switch (e.key.keysym.sym) {
                case SDLK_DOWN:
                    movePieceDown();
                    break;
                case SDLK_LEFT:
                    movePieceLeft();
                    break;
                case SDLK_RIGHT:
                    movePieceRight();
                    break;
                case SDLK_UP:
                    rotatePiece();
                    break;
                case SDLK_SPACE:
                    dropPiece();
                    break;
                default:
                    break;
            }
        } else if(e.type == SDL_KEYDOWN && isGameOver == true && e.key.keysym.sym == SDLK_RETURN) {
            win->setObject(new Game());
            win->object->load(win);
            return;
        }
    }

private:
    static const int rows = 21;
    static const int cols = 10;

    int pieceX;
    int pieceY;
    std::vector<std::vector<int>> grid;
    std::vector<std::vector<int>> currentPiece;
    std::vector<std::vector<int>> nextPiece;
    bool isGameOver;
    int score;

    Uint32 lastFallTime = 0;
    Uint32 fallInterval = 0; 

    void resetGame() {
        srand(static_cast<unsigned>(time(nullptr)));
        grid.clear();
        grid.resize(rows, std::vector<int>(cols, 0));
        currentPiece = getRandomPiece();
        nextPiece = getRandomPiece();
        pieceX = cols / 2 - static_cast<int>(currentPiece[0].size()) / 2;
        pieceY = 0;
        isGameOver = false;
        score = 0;
        lastFallTime = SDL_GetTicks();
        fallInterval = 1000;
    }

    std::vector<std::vector<int>> getRandomPiece() {
        const std::vector<std::vector<std::vector<int>>> pieces = {
            {{1, 1, 1, 1}},                
            {{1, 1}, {1, 1}},              
            {{0, 1, 0}, {1, 1, 1}},        
            {{1, 1, 0}, {0, 1, 1}},        
            {{0, 1, 1}, {1, 1, 0}},        
            {{1, 1, 1}, {1, 0, 0}},        
            {{1, 1, 1}, {0, 0, 1}}         
        };
        int index = rand() % static_cast<int>(pieces.size());
        return pieces[index];
    }

    std::vector<std::vector<int>> rotate(const std::vector<std::vector<int>>& piece) {
        int rows = static_cast<int>(piece.size());
        int cols = static_cast<int>(piece[0].size());
        std::vector<std::vector<int>> rotatedPiece(cols, std::vector<int>(rows, 0));

        for (int row = 0; row < rows; ++row) {
            for (int col = 0; col < cols; ++col) {
                rotatedPiece[col][rows - 1 - row] = piece[row][col];
            }
        }

        return rotatedPiece;
    }

    bool checkCollision(int newX, int newY, const std::vector<std::vector<int>>& piece) {
        for (int row = 0; row < static_cast<int>(piece.size()); ++row) {
            for (int col = 0; col < static_cast<int>(piece[0].size()); ++col) {
                if (piece[row][col] == 1) {
                    int gridX = newX + col;
                    int gridY = newY + row;
                    if (gridX < 0 || gridX >= cols || gridY >= rows || (gridY >= 0 && grid[gridY][gridX] != 0)) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    void lockPiece() {
        for (int row = 0; row < static_cast<int>(currentPiece.size()); ++row) {
            for (int col = 0; col < static_cast<int>(currentPiece[0].size()); ++col) {
                if (currentPiece[row][col] == 1) {
                    grid[pieceY + row][pieceX + col] = 1;
                }
            }
        }
        clearLines();
        currentPiece = nextPiece;
        nextPiece = getRandomPiece();
        pieceX = cols / 2 - static_cast<int>(currentPiece[0].size()) / 2;
        pieceY = 0;
        if (checkCollision(pieceX, pieceY, currentPiece)) {
            gameOver();
        }
    }

    void clearLines() {
        for (int row = rows - 1; row >= 0; --row) {
            bool isFull = true;
            for (int col = 0; col < cols; ++col) {
                if (grid[row][col] == 0) {
                    isFull = false;
                    break;
                }
            }
            if (isFull) {
                grid.erase(grid.begin() + row);
                grid.insert(grid.begin(), std::vector<int>(cols, 0));
                row++;
                score += 100;
                if (fallInterval > 100) {
                    fallInterval -= 10;
                }
            }
        }
    }

    void gameOver() {
        isGameOver = true;
    }

    void movePieceDown() {
        if (!checkCollision(pieceX, pieceY + 1, currentPiece)) {
            pieceY++;
        } else {
            lockPiece();
        }
    }

    void movePieceLeft() {
        if (!checkCollision(pieceX - 1, pieceY, currentPiece)) {
            pieceX--;
        }
    }

    void movePieceRight() {
        if (!checkCollision(pieceX + 1, pieceY, currentPiece)) {
            pieceX++;
        }
    }

    void rotatePiece() {
        auto rotatedPiece = rotate(currentPiece);
        if (!checkCollision(pieceX, pieceY, rotatedPiece)) {
            currentPiece = rotatedPiece;
        }
    }

    void dropPiece() {
        while (!checkCollision(pieceX, pieceY + 1, currentPiece)) {
            pieceY++;
        }
        lockPiece();
    }

    mx::Font the_font;
    mx::Texture bg;
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
    
    MainWindow(std::string path, int tw, int th) : mx::mxWindow("Blocks", tw, th, false) {
        tex.createTexture(this, 300 * 2, 630 * 2);
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
    MainWindow main_window("/", 300 * 2 , 630 * 2);
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
    int tw = 300 * 2, th = 630 * 2;
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