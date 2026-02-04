#include "vk.hpp"
#include "SDL.h"
#include <random>
#include <cmath>
#include <sstream>
#include <ctime>
#include <unistd.h>
#include <climits>
#include "argz.hpp"


constexpr int STARTX = 184;      
constexpr int STARTY = 61;       
constexpr int GRID_WIDTH = 8;    
constexpr int GRID_HEIGHT = 18;  
constexpr int BLOCK_SIZE = 31;   
constexpr int BLOCK_HEIGHT = 15; 
constexpr int BLOCK_SPACING = 1; 
constexpr int NEXT_PANEL_X = 450;
constexpr int NEXT_PANEL_Y = 180;
constexpr int FLASH_DURATION = 300; 


enum {
    BLOCK_BLACK = 0, BLOCK_YELLOW, BLOCK_ORANGE, BLOCK_LTBLUE,
    BLOCK_DBLUE, BLOCK_PURPLE, BLOCK_PINK, BLOCK_GRAY,
    BLOCK_RED, BLOCK_GREEN, BLOCK_CLEAR, BLOCK_COUNT
};


enum ScreenState { SCREEN_INTRO, SCREEN_START, SCREEN_GAME, SCREEN_GAMEOVER, SCREEN_OPTIONS, SCREEN_CREDITS };


struct GameData {
    unsigned long score;
    int lines;
    int speed;
    int lineamt;
    
    GameData() { newgame(); }
    void newgame() {
        score = 0;
        lines = 0;
        speed = 20;
        lineamt = 0;
    }
    void addline() {
        lines++;
        score += 6;
        lineamt++;
        if (lineamt >= 10) {  
            lineamt = 0;
            speed = std::max(5, speed - 4);
        }
    }
};

struct BlockColor {
    int c1, c2, c3;
    void randcolor() {
        c1 = 1 + rand() % 9;
        c2 = 1 + rand() % 9;
        c3 = 1 + rand() % 9;
        if (c1 == c2 && c1 == c3) randcolor();
    }
    void shiftcolor(bool dir) {
        int t1 = c1, t2 = c2, t3 = c3;
        if (dir) { c1 = t3; c2 = t1; c3 = t2; }
        else { c1 = t2; c2 = t3; c3 = t1; }
    }
};

struct GameBlock {
    BlockColor color;
    BlockColor nextcolor;
    int x, y;
    int rotation;
    GameBlock() : x(0), y(0), rotation(0) {}
};

struct TileMatrix {
    int grid[GRID_WIDTH][GRID_HEIGHT];
    GameBlock block, nextblock;
    GameData Game;
    
    TileMatrix() { init_matrix(); }
    
    void init_matrix() {
        memset(grid, 0, sizeof(grid));
        Game.newgame();
        block.color.randcolor();
        nextblock.color.randcolor();
    }
};

class MasterPieceWindow : public mx::VKWindow {
private:
    TileMatrix matrix;
    ScreenState currentScreen = SCREEN_INTRO;
    Uint32 lastTick = 0;
    int cursorPos = 0;
    bool paused = false;
    
    
    bool flashActive = false;
    Uint32 flashStartTime = 0;
    bool flashGrid[GRID_WIDTH][GRID_HEIGHT] = {{false}};
    
    
    std::vector<mx::VKSprite*> grid_blocks;
    mx::VKSprite* gamebg = nullptr;
    mx::VKSprite* startScreen = nullptr;
    mx::VKSprite* introScreen = nullptr;
    
public:
    MasterPieceWindow(const std::string& path, int wx, int wy, bool full)
        : mx::VKWindow("-[ MasterPiece 2D - Vulkan ]-", wx, wy, full) {
        setPath(path);
        setFont("font.ttf", 34);
        srand((unsigned int)time(0));
        lastTick = SDL_GetTicks();
    }
    
    virtual ~MasterPieceWindow() {}

    void initVulkan() override {
        mx::VKWindow::initVulkan();
        initGfx();
    }

    void initGfx() {
        std::string vertShader = util.path + "/data/sprite_vert.spv";
        gamebg = createSprite(util.path + "/data/gamebg.png", vertShader);
        startScreen = createSprite(util.path + "/data/universe.png", vertShader);
        introScreen = createSprite(util.path + "/data/intro.png", vertShader);
        const char* blockFiles[] = {
            "block_black.png", "block_yellow.png", "block_orange.png", "block_ltblue.png",
            "block_dblue.png", "block_purple.png", "block_pink.png", "block_gray.png",
            "block_red.png", "block_green.png", "block_clear.png"
        };
        for (int i = 0; i < BLOCK_COUNT; i++) {
            grid_blocks.push_back(createSprite(util.path + "/data/" + blockFiles[i], vertShader));
        }
    }
    
    int centerX(const char* text) {
        return w/2 - (int)(strlen(text) * 17 / 2);
    }

    void proc() override {
        switch (currentScreen) {
            case SCREEN_INTRO:
                updateIntro();
                break;
            case SCREEN_START:
                updateStart();
                break;
            case SCREEN_GAME:
                updateGame();
                break;
            case SCREEN_GAMEOVER:
                updateGameOver();
                break;
            case SCREEN_OPTIONS:
                updateOptions();
                break;
            case SCREEN_CREDITS:
                updateCredits();
                break;
        }
    }
    
    void updateIntro() {
        if (introScreen) {
            introScreen->drawSpriteRect(0, 0, w, h);
        }
        const char* introText = "Press ENTER to Start";
        printText(introText, centerX(introText), h/2, {255, 255, 255, 255});
        Uint32 currentTick = SDL_GetTicks();
        if (currentTick - lastTick > 3000) {
            currentScreen = SCREEN_START;
            lastTick = currentTick;
        }
    }
    
    void updateStart() {
        if (startScreen) {
            startScreen->drawSpriteRect(0, 0, w, h);
        }
        
        
        const char* title = "MasterPiece";
        printText(title, centerX(title), 100, {255, 255, 0, 255});
        
        const char* menuItems[] = {"New Game", "Options", "Credits", "Quit"};
        for (int i = 0; i < 4; i++) {
            SDL_Color col = (i == cursorPos) ? SDL_Color{255, 255, 0, 255} : SDL_Color{255, 255, 255, 255};
            printText(menuItems[i], centerX(menuItems[i]), 200 + i * 50, col);
        }
        
        printText(">>", centerX(menuItems[cursorPos]) - 40, 200 + cursorPos * 50, {255, 255, 0, 255});
    }
    
    void updateGame() {
        if (!paused) {
            logic();
        }
        drawGame();
    }
    
    void updateGameOver() {
        if (gamebg) {
            gamebg->drawSpriteRect(0, 0, w, h);
        }
        
        const char* gameOverText = "GAME OVER!";
        printText(gameOverText, centerX(gameOverText), h/2 - 40, {255, 0, 0, 255});
        
        std::ostringstream stream;
        stream << "Final Score: " << matrix.Game.score;
        std::string scoreStr = stream.str();
        printText(scoreStr.c_str(), centerX(scoreStr.c_str()), h/2, {255, 255, 255, 255});
        
        stream.str("");
        stream << "Lines: " << matrix.Game.lines;
        std::string linesStr = stream.str();
        printText(linesStr.c_str(), centerX(linesStr.c_str()), h/2 + 40, {255, 255, 255, 255});
        
        const char* returnText = "Press ENTER for Menu or Return to Quit";
        printText(returnText, centerX(returnText), h/2 + 100, {255, 255, 0, 255});
    }
    
    void updateOptions() {
        if (startScreen) {
            startScreen->drawSpriteRect(0, 0, w, h);
        }
        const char* optTitle = "OPTIONS";
        const char* speed = "Speed: Normal";
        const char* sound = "Sound: On";
        const char* optReturn = "Press Return to return";
        printText(optTitle, centerX(optTitle), 100, {255, 255, 0, 255});
        printText(speed, centerX(speed), 180, {255, 255, 255, 255});
        printText(sound, centerX(sound), 220, {255, 255, 255, 255});
        printText(optReturn, centerX(optReturn), 320, {255, 255, 0, 255});
    }
    
    void updateCredits() {
        if (startScreen) {
            startScreen->drawSpriteRect(0, 0, w, h);
        }
        const char* credTitle = "CREDITS";
        const char* cred1 = "MasterPiece Clone";
        const char* cred2 = "Vulkan Engine by libmx2";
        const char* credReturn = "Press Return to return";
        printText(credTitle, centerX(credTitle), 100, {255, 255, 0, 255});
        printText(cred1, centerX(cred1), 180, {255, 255, 255, 255});
        printText(cred2, centerX(cred2), 220, {255, 255, 255, 255});
        printText(credReturn, centerX(credReturn), 320, {255, 255, 0, 255});
    }
    
    void drawGame() {
        float scaleX = (float)w / 640.0f;
        float scaleY = (float)h / 480.0f;
        
        if (gamebg) {
            gamebg->drawSpriteRect(0, 0, w, h);
        }
        
        drawmatrix(scaleX, scaleY);
        drawblock(scaleX, scaleY);
        drawnext(scaleX, scaleY);
        
        std::ostringstream stream;
        stream << "Score: " << matrix.Game.score;
        printText(stream.str().c_str(), (int)(200 * scaleX), (int)(60 * scaleY)-10, {255, 255, 255, 255});
        
        stream.str("");
        stream << "Lines: " << matrix.Game.lines;
        printText(stream.str().c_str(), (int)(310 * scaleX), (int)(60 * scaleY)-10, {255, 255, 255, 255});
        
        if (paused) {
            printText("PAUSED - Press P to Continue", w/2 - 120, h/2, {255, 255, 0, 255});
        }
    }
    
    void drawmatrix(float scaleX, float scaleY) {
        Uint32 now = SDL_GetTicks();
        bool showFlash = flashActive && ((now - flashStartTime) / 50 % 2 == 0);
        
        for (int i = 0; i < GRID_WIDTH; i++) {
            for (int j = 0; j < GRID_HEIGHT; j++) {
                if (matrix.grid[i][j] > 0 && matrix.grid[i][j] < BLOCK_COUNT) {
                    int x = STARTX + i * (BLOCK_SIZE + BLOCK_SPACING);
                    int y = STARTY + j * (BLOCK_HEIGHT + BLOCK_SPACING) + 10;
                    
                    
                    if (flashActive && flashGrid[i][j] && !showFlash) {
                        continue;
                    }
                    
                    if (grid_blocks[matrix.grid[i][j]]) {
                        grid_blocks[matrix.grid[i][j]]->drawSpriteRect(
                            (int)(x * scaleX), (int)(y * scaleY) + 10, 
                            (int)(BLOCK_SIZE * scaleX), (int)(BLOCK_HEIGHT * scaleY)
                        );
                    }
                }
            }
        }
    }
    
    void drawblock(float scaleX, float scaleY) {
        int bx = STARTX + matrix.block.x * (BLOCK_SIZE + BLOCK_SPACING);

        int c1 = matrix.block.color.c1;
        int c2 = matrix.block.color.c2;
        int c3 = matrix.block.color.c3;

        
        int y0 = matrix.block.y;
        int y1 = matrix.block.y + 1;
        int y2 = matrix.block.y + 2;
        
        if (y0 >= 0 && c1 > 0 && c1 < BLOCK_COUNT && grid_blocks[c1]) {
            int screenY = STARTY + y0 * (BLOCK_HEIGHT + BLOCK_SPACING) + 10;
            grid_blocks[c1]->drawSpriteRect(
                (int)(bx * scaleX), (int)(screenY * scaleY),
                (int)(BLOCK_SIZE * scaleX), (int)(BLOCK_HEIGHT * scaleY)
            );
        }
        if (y1 >= 0 && c2 > 0 && c2 < BLOCK_COUNT && grid_blocks[c2]) {
            int screenY = STARTY + y1 * (BLOCK_HEIGHT + BLOCK_SPACING) + 10; 
            grid_blocks[c2]->drawSpriteRect(
                (int)(bx * scaleX), (int)(screenY * scaleY),
                (int)(BLOCK_SIZE * scaleX), (int)(BLOCK_HEIGHT * scaleY)
            );
        }
        if (y2 >= 0 && c3 > 0 && c3 < BLOCK_COUNT && grid_blocks[c3]) {
            int screenY = STARTY + y2 * (BLOCK_HEIGHT + BLOCK_SPACING) + 10;
            grid_blocks[c3]->drawSpriteRect(
                (int)(bx * scaleX), (int)(screenY * scaleY),
                (int)(BLOCK_SIZE * scaleX), (int)(BLOCK_HEIGHT * scaleY)
            );
        }
    }
    
    void drawnext(float scaleX, float scaleY) {
        
        
        int bx = NEXT_PANEL_X + 70;
        int by = NEXT_PANEL_Y + 15;
        
        int c1 = matrix.nextblock.color.c1;
        int c2 = matrix.nextblock.color.c2;
        int c3 = matrix.nextblock.color.c3;
        
        if (c1 > 0 && c1 < BLOCK_COUNT && grid_blocks[c1]) {
            grid_blocks[c1]->drawSpriteRect(
                (int)(bx * scaleX), (int)(by * scaleY),
                (int)(BLOCK_SIZE * scaleX), (int)(BLOCK_HEIGHT * scaleY)
            );
        }
        if (c2 > 0 && c2 < BLOCK_COUNT && grid_blocks[c2]) {
            grid_blocks[c2]->drawSpriteRect(
                (int)(bx * scaleX), (int)((by + BLOCK_HEIGHT + BLOCK_SPACING) * scaleY),
                (int)(BLOCK_SIZE * scaleX), (int)(BLOCK_HEIGHT * scaleY)
            );
        }
        if (c3 > 0 && c3 < BLOCK_COUNT && grid_blocks[c3]) {
            grid_blocks[c3]->drawSpriteRect(
                (int)(bx * scaleX), (int)((by + (BLOCK_HEIGHT + BLOCK_SPACING) * 2) * scaleY),
                (int)(BLOCK_SIZE * scaleX), (int)(BLOCK_HEIGHT * scaleY)
            );
        }
    }
    
    void logic() {
        static Uint32 lastMove = 0;
        Uint32 now = SDL_GetTicks();
        
        
        if (flashActive) {
            if (now - flashStartTime >= FLASH_DURATION) {
                
                clearFlashedBlocks();
                flashActive = false;
                applyGravity();
                
                if (findMatches()) {
                    startFlash();
                }
            }
            return; 
        }
        
        if (now - lastMove > (Uint32)(matrix.Game.speed * 50)) {
            lastMove = now;
            
            
            if (canMoveDown()) {
                matrix.block.y++;
            } else {
                
                placeBlock();
                
                
                if (findMatches()) {
                    startFlash();
                } else {
                    
                    spawnNewBlock();
                    
                    
                    if (checkGameOver()) {
                        currentScreen = SCREEN_GAMEOVER;
                    }
                }
            }
        }
    }
    
    bool canMoveDown() {
        int bx = matrix.block.x;
        int by = matrix.block.y;
        
        
        if (by + 3 >= GRID_HEIGHT) return false;
        
        
        if (matrix.grid[bx][by + 3] > 0) return false;
        
        return true;
    }
    
    bool canMoveLeft() {
        if (matrix.block.x <= 0) return false;
        int bx = matrix.block.x - 1;
        int by = matrix.block.y;
        
        
        if (by >= 0 && by < GRID_HEIGHT && matrix.grid[bx][by] > 0) return false;
        if (by + 1 >= 0 && by + 1 < GRID_HEIGHT && matrix.grid[bx][by + 1] > 0) return false;
        if (by + 2 >= 0 && by + 2 < GRID_HEIGHT && matrix.grid[bx][by + 2] > 0) return false;
        
        return true;
    }
    
    bool canMoveRight() {
        if (matrix.block.x >= GRID_WIDTH - 1) return false;
        int bx = matrix.block.x + 1;
        int by = matrix.block.y;
        
        
        if (by >= 0 && by < GRID_HEIGHT && matrix.grid[bx][by] > 0) return false;
        if (by + 1 >= 0 && by + 1 < GRID_HEIGHT && matrix.grid[bx][by + 1] > 0) return false;
        if (by + 2 >= 0 && by + 2 < GRID_HEIGHT && matrix.grid[bx][by + 2] > 0) return false;
        
        return true;
    }
    
    void placeBlock() {
        int bx = matrix.block.x;
        int by = matrix.block.y;
        
        if (by >= 0 && by < GRID_HEIGHT)
            matrix.grid[bx][by] = matrix.block.color.c1;
        if (by + 1 >= 0 && by + 1 < GRID_HEIGHT)
            matrix.grid[bx][by + 1] = matrix.block.color.c2;
        if (by + 2 >= 0 && by + 2 < GRID_HEIGHT)
            matrix.grid[bx][by + 2] = matrix.block.color.c3;
    }
    
    bool findMatches() {
        
        memset(flashGrid, false, sizeof(flashGrid));
        bool foundMatch = false;
        
        
        for (int j = 0; j < GRID_HEIGHT; j++) {
            for (int i = 0; i < GRID_WIDTH - 2; i++) {
                if (matrix.grid[i][j] > 0 && 
                    matrix.grid[i][j] == matrix.grid[i+1][j] && 
                    matrix.grid[i][j] == matrix.grid[i+2][j]) {
                    
                    flashGrid[i][j] = true;
                    flashGrid[i+1][j] = true;
                    flashGrid[i+2][j] = true;
                    foundMatch = true;
                }
            }
        }
        
        
        for (int i = 0; i < GRID_WIDTH; i++) {
            for (int j = 0; j < GRID_HEIGHT - 2; j++) {
                if (matrix.grid[i][j] > 0 && 
                    matrix.grid[i][j] == matrix.grid[i][j+1] && 
                    matrix.grid[i][j] == matrix.grid[i][j+2]) {
                    
                    flashGrid[i][j] = true;
                    flashGrid[i][j+1] = true;
                    flashGrid[i][j+2] = true;
                    foundMatch = true;
                }
            }
        }
        
        return foundMatch;
    }
    
    void startFlash() {
        flashActive = true;
        flashStartTime = SDL_GetTicks();
    }
    
    void clearFlashedBlocks() {
        for (int i = 0; i < GRID_WIDTH; i++) {
            for (int j = 0; j < GRID_HEIGHT; j++) {
                if (flashGrid[i][j]) {
                    matrix.grid[i][j] = 0;
                    matrix.Game.addline();
                }
            }
        }
        memset(flashGrid, false, sizeof(flashGrid));
        
        
        spawnNewBlock();
        
        
        if (checkGameOver()) {
            currentScreen = SCREEN_GAMEOVER;
        }
    }
    
    void applyGravity() {
        for (int i = 0; i < GRID_WIDTH; i++) {
            for (int j = GRID_HEIGHT - 2; j >= 0; j--) {
                if (matrix.grid[i][j] > 0 && matrix.grid[i][j + 1] == 0) {
                    
                    int k = j;
                    while (k + 1 < GRID_HEIGHT && matrix.grid[i][k + 1] == 0) {
                        matrix.grid[i][k + 1] = matrix.grid[i][k];
                        matrix.grid[i][k] = 0;
                        k++;
                    }
                }
            }
        }
    }
    
    void spawnNewBlock() {
        matrix.block.color = matrix.nextblock.color;
        matrix.nextblock.color.randcolor();
        matrix.block.x = GRID_WIDTH / 2;
        matrix.block.y = 0;  
    }
    
    bool checkGameOver() {
        
        for (int i = 0; i < GRID_WIDTH; i++) {
            if (matrix.grid[i][0] > 0) {
                return true;
            }
        }
        return false;
    }
    
    void moveBlockLeft() {
        if (canMoveLeft()) {
            matrix.block.x--;
        }
    }
    
    void moveBlockRight() {
        if (canMoveRight()) {
            matrix.block.x++;
        }
    }
    
    void moveBlockDown() {
        if (canMoveDown()) {
            matrix.block.y++;
        }
    }
    
    void dropBlock() {
        while (canMoveDown()) {
            matrix.block.y++;
        }
    }
    
    void event(SDL_Event& e) override {
        if(e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
            quit();
            return;
        }
        if (e.type == SDL_KEYDOWN) {
            gameKeypress(e.key.keysym.sym);
        }
    }
    
    void gameKeypress(SDL_Keycode key) {
        switch (currentScreen) {
            case SCREEN_INTRO:
                if (key == SDLK_RETURN) {
                    currentScreen = SCREEN_START;
                }
                break;
            case SCREEN_START:
                if (key == SDLK_UP) {
                    cursorPos = (cursorPos - 1 + 4) % 4;
                } else if (key == SDLK_DOWN) {
                    cursorPos = (cursorPos + 1) % 4;
                } else if (key == SDLK_RETURN) {
                    if (cursorPos == 0) {  
                        matrix.init_matrix();
                        matrix.block.x = GRID_WIDTH / 2;
                        matrix.block.y = 0;  
                        currentScreen = SCREEN_GAME;
                    } else if (cursorPos == 1) {  
                        currentScreen = SCREEN_OPTIONS;
                    } else if (cursorPos == 2) {  
                        currentScreen = SCREEN_CREDITS;
                    } else if (cursorPos == 3) {  
                        quit();
                    }
                }
                break;
            case SCREEN_GAME:
                if (key == SDLK_LEFT) {
                    moveBlockLeft();
                } else if (key == SDLK_RIGHT) {
                    moveBlockRight();
                } else if (key == SDLK_DOWN) {
                    moveBlockDown();
                } else if (key == SDLK_UP) {
                    matrix.block.color.shiftcolor(true);
                } else if (key == SDLK_SPACE) {
                    dropBlock();
                } else if (key == SDLK_p) {
                    paused = !paused;
                } else if (key == SDLK_RETURN) {
                    currentScreen = SCREEN_START;
                }
                break;
            case SCREEN_GAMEOVER:
                if (key == SDLK_RETURN) {
                    currentScreen = SCREEN_START;
                } else if (key == SDLK_RETURN) {
                    quit();
                }
                break;
            case SCREEN_OPTIONS:
            case SCREEN_CREDITS:
                if (key == SDLK_RETURN) {
                    currentScreen = SCREEN_START;
                }
                break;
        }
    }
};

int main(int argc, char **argv) {
    Arguments args = proc_args(argc, argv);
    try {
        MasterPieceWindow window(args.path, args.width, args.height, args.fullscreen);
        window.initVulkan();
        window.loop();
        window.cleanup();
    } catch (mx::Exception &e) {
        SDL_Log("mx: Exception: %s\n", e.text().c_str());
    }
    return EXIT_SUCCESS;
}
