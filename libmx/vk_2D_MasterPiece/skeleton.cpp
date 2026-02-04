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


enum ScreenState { SCREEN_INTRO, SCREEN_START, SCREEN_GAME, SCREEN_GAMEOVER, SCREEN_OPTIONS, SCREEN_CREDITS, SCREEN_SCORES };


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
    bool horizontal;  
    GameBlock() : x(0), y(0), rotation(0), horizontal(false) {}
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

struct Score {
    std::string name;
    int score;
    Score(std::string name_, int score_) : name(name_), score(score_) {}
    Score() : name{}, score{0} {}
};

class HighScores {
public:
    HighScores() {
        read();
    }
    ~HighScores() {
        write();
    }
    void addScore(const std::string &name, int score) {
        scores.push_back({name, score});
        sort();
        
        if (scores.size() > 10) {
            scores.resize(10);
        }
    }
    void sort() {
        std::sort(scores.begin(), scores.end(), [](const Score &one, const Score &two) {
            return one.score > two.score;  
        });
    }

    bool qualifiesForHighScore(int score) {
        if (scores.size() < 10) return true;
        return score > scores.back().score;
    }

    const std::vector<Score>& getScores() const {
        return scores;
    }

    void init() {
        scores.clear();
        for(int i = 0; i < 10; ++i) 
            scores.push_back(Score("Anonymous", (10 - i) * 100));
    }

    void read() {
        std::fstream file;
        file.open("./scores.dat", std::ios::in);
        if(!file.is_open()) {
            mx::system_out << "No scores file.\n";
            init();
            return;
        }
        std::string input;
        int count = 0;
        while(std::getline(file, input)) {
            if(count >= 10) break;
            if(input.length() > 0) {
                auto pos = input.find(":");
                if (pos != std::string::npos) {
                    std::string left = input.substr(0, pos);
                    std::string right = input.substr(pos + 1);
                    scores.push_back(Score(left, std::strtol(right.c_str(), 0, 10)));
                    count++;
                }
            }
        }
        file.close();
        sort();
    }
    void write() {
        std::fstream file;
        file.open("./scores.dat", std::ios::out);
        if(!file.is_open()) {
            mx::system_err << "Could not open score file..\n";
            return;
        }   
        for(auto &s : scores) {
            file << s.name << ":" << s.score << std::endl;
        }
        file.close();
    }
private:

    std::vector<Score> scores;
};

class MasterPieceWindow : public mx::VKWindow {
private:
    HighScores scores;
    TileMatrix matrix;
    ScreenState currentScreen = SCREEN_INTRO;
    Uint32 lastTick = 0;
    int cursorPos = 0;
    bool paused = false;
    
    
    int optionsCursor = 0;
    int difficultySetting = 1;    
    
    
    bool enteringName = false;
    std::string playerName;
    int finalScore = 0;
    mx::VKSprite* scoresBackground = nullptr;
    
    int matchBonus = 0;
    
    
    int lastFontSize = 0;
    
    
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
        updateFontSize();
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
        std::string fragShader = util.path + "/data/sprite_kaleidoscope.spv";
        gamebg = createSprite(util.path + "/data/gamebg.png", util.path + "/data/sprite_vert_bubble.spv", fragShader);
        startScreen = createSprite(util.path + "/data/universe.png", util.path + "/data/sprite_vert_bubble.spv",util.path + "/data/sprite_electric.spv");
        introScreen = createSprite(util.path + "/data/intro.png", util.path + "/data/sprite_vert_bubble.spv", util.path + "/data/sprite_bubble.spv");
        scoresBackground = createSprite(util.path + "/data/universe.png", util.path + "/data/sprite_vert_bubble.spv", util.path + "/data/sprite_warp.spv");
        const char* blockFiles[] = {
            "block_black.png", "block_yellow.png", "block_orange.png", "block_ltblue.png",
            "block_dblue.png", "block_purple.png", "block_pink.png", "block_gray.png",
            "block_red.png", "block_green.png", "block_clear.png"
        };
        for (int i = 0; i < BLOCK_COUNT; i++) {
            grid_blocks.push_back(createSprite(util.path + "/data/" + blockFiles[i], vertShader, util.path + "/data/sprite_frag.spv"));
        }
    }
    
    void updateFontSize() {
        int fontSize = (int)(24.0f * ((float)h / 480.0f));
        fontSize = std::max(14, std::min(48, fontSize));  
        if (fontSize != lastFontSize) {
            setFont("font.ttf", fontSize-4);
            lastFontSize = fontSize;
        }
    }
    
    int getCharWidth() {
        return (int)(lastFontSize * 0.5f);
    }
    
    int getMenuSpacing() {
        return (int)(40.0f * ((float)h / 480.0f));
    }
    
    int scaleY(int baseY) {
        return (int)(baseY * ((float)h / 480.0f));
    }
    
    int centerX(const char* text) {
        return w/2 - (int)(strlen(text) * getCharWidth() / 2);
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
            case SCREEN_SCORES:
                updateScores();
                break;
        }
    }
    
    void updateIntro() {
        if (introScreen) {
            float time_f = static_cast<float>(SDL_GetTicks()) / 1000.0f;
            introScreen->setShaderParams(time_f, 0.0f, 0.0f, 0.0f);
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
            float time_f = static_cast<float>(SDL_GetTicks()) / 1000.0f;
            startScreen->setShaderParams(time_f, 0.0f, 0.0f, 0.0f);
            startScreen->drawSpriteRect(0, 0, w, h);
        }
        int titleY = scaleY(100);
        int menuStartY = scaleY(180);
        int spacing = getMenuSpacing();
        const char* title = "Vulkan MasterPiece";
        printText(title, centerX(title), titleY, {255, 255, 0, 255});
        const char* menuItems[] = {"New Game", "High Scores", "Options", "Credits", "Quit"};
        for (int i = 0; i < 5; i++) {
            SDL_Color col = (i == cursorPos) ? SDL_Color{255, 255, 0, 255} : SDL_Color{255, 255, 255, 255};
            printText(menuItems[i], centerX(menuItems[i]), menuStartY + i * spacing, col);
        }
        printText(">>", centerX(menuItems[cursorPos]) - scaleY(30), menuStartY + cursorPos * spacing, {255, 255, 0, 255});
    }
    
    void updateGame() {
        if (!paused) {
            logic();
        }
        drawGame();
    }
    
    void updateGameOver() {
        if (gamebg) {
            float time_f = static_cast<float>(SDL_GetTicks()) / 1000.0f;
            gamebg->setShaderParams(time_f, 0.0f, 0.0f, 0.0f);
            gamebg->drawSpriteRect(0, 0, w, h);
        }
        
        const char* gameOverText = "GAME OVER!";
        printText(gameOverText, centerX(gameOverText), h/2 - 80, {255, 0, 0, 255});
        
        std::ostringstream stream;
        stream << "Final Score: " << matrix.Game.score;
        std::string scoreStr = stream.str();
        printText(scoreStr.c_str(), centerX(scoreStr.c_str()), h/2 - 20, {255, 255, 255, 255});
        
        stream.str("");
        stream << "Tabs: " << matrix.Game.lines;
        std::string linesStr = stream.str();
        printText(linesStr.c_str(), centerX(linesStr.c_str()), h/2 + 40, {255, 255, 255, 255});
        
        const char* returnText = "Press ENTER to view High Scores";
        printText(returnText, centerX(returnText), h/2 + 120, {255, 255, 0, 255});
    }
    
    void goToScoresScreen() {
        finalScore = matrix.Game.score;
        playerName = "";
        enteringName = scores.qualifiesForHighScore(finalScore);
        currentScreen = SCREEN_SCORES;
    }
    
    void updateOptions() {
        if (startScreen) {
            float time_f = static_cast<float>(SDL_GetTicks()) / 1000.0f;
            startScreen->setShaderParams(time_f, 0.0f, 0.0f, 0.0f);
            startScreen->drawSpriteRect(0, 0, w, h);
        }
        
        int titleY = scaleY(100);
        int optStartY = scaleY(180);
        int spacing = getMenuSpacing();
        
        const char* optTitle = "OPTIONS";
        printText(optTitle, centerX(optTitle), titleY, {255, 255, 0, 255});
        
        
        const char* difficultyLabels[] = {"Easy", "Normal", "Hard"};
        std::string diffText = std::string("< Difficulty: ") + difficultyLabels[difficultySetting] + " >";
        SDL_Color diffColor = (optionsCursor == 0) ? SDL_Color{255, 255, 0, 255} : SDL_Color{255, 255, 255, 255};
        printText(diffText.c_str(), centerX(diffText.c_str()), optStartY, diffColor);
        if (optionsCursor == 0) {
            printText(">>", centerX(diffText.c_str()) - scaleY(30), optStartY, {255, 255, 0, 255});
        }
        
        
        const char* backText = "Back";
        SDL_Color backColor = (optionsCursor == 1) ? SDL_Color{255, 255, 0, 255} : SDL_Color{255, 255, 255, 255};
        printText(backText, centerX(backText), optStartY + spacing, backColor);
        if (optionsCursor == 1) {
            printText(">>", centerX(backText) - scaleY(30), optStartY + spacing, {255, 255, 0, 255});
        }
        
        const char* instructions = "UP/DOWN: Select  LEFT/RIGHT: Change  ENTER: Confirm";
        printText(instructions, centerX(instructions)-100, scaleY(350), {200, 200, 200, 255});
    }
    
    void updateCredits() {
        if (startScreen) {
            float time_f = static_cast<float>(SDL_GetTicks()) / 1000.0f;
            startScreen->setShaderParams(time_f, 0.0f, 0.0f, 0.0f);
            startScreen->drawSpriteRect(0, 0, w, h);
        }
        
        int titleY = scaleY(100);
        int contentY = scaleY(180);
        int spacing = getMenuSpacing();
        
        const char* credTitle = "CREDITS";
        const char* cred1 = "MasterPiece Clone";
        const char* cred2 = "MX2 Engine by Jared Bruni";
        const char* credReturn = "Press Return to return";
        printText(credTitle, centerX(credTitle), titleY, {255, 255, 0, 255});
        printText(cred1, centerX(cred1), contentY, {255, 255, 255, 255});
        printText(cred2, centerX(cred2), contentY + spacing, {255, 255, 255, 255});
        printText(credReturn, centerX(credReturn), scaleY(320), {255, 255, 0, 255});
    }
    
    void updateScores() {
        
        if (scoresBackground) {
            float time_f = static_cast<float>(SDL_GetTicks()) / 1000.0f;
            scoresBackground->setShaderParams(time_f, 0.0f, 0.0f, 0.0f);
            scoresBackground->drawSpriteRect(0, 0, w, h);
        }
        
        int titleY = scaleY(40);
        int listStartY = scaleY(90);
        int lineHeight = scaleY(32);
        
        
        const char* title = "HIGH SCORES";
        printText(title, centerX(title), titleY, {255, 255, 0, 255});
        
        
        const auto& scoreList = scores.getScores();
        for (size_t i = 0; i < scoreList.size() && i < 10; ++i) {
            std::ostringstream stream;
            stream << (i + 1) << ". " << scoreList[i].name << "  " << scoreList[i].score;
            std::string scoreStr = stream.str();
            
            SDL_Color color = {255, 255, 255, 255};
            
            if (enteringName && finalScore > 0) {
                
                int rank = 0;
                for (size_t j = 0; j < scoreList.size(); ++j) {
                    if (finalScore > scoreList[j].score) {
                        rank = j;
                        break;
                    }
                    rank = j + 1;
                }
                if ((int)i == rank) {
                    color = {0, 255, 0, 255}; 
                }
            }
            
            printText(scoreStr.c_str(), scaleY(100), listStartY + (int)i * lineHeight, color);
        }
        
        
        if (enteringName) {
            int entryY = scaleY(420);
            
            std::ostringstream stream;
            stream << "Your Score: " << finalScore;
            std::string yourScore = stream.str();
            printText(yourScore.c_str(), centerX(yourScore.c_str()), entryY - lineHeight * 2, {255, 255, 0, 255});
            
            const char* prompt = "Enter your name:";
            printText(prompt, centerX(prompt), entryY - lineHeight, {255, 255, 255, 255});
            
            
            std::string displayName = playerName + "_";
            printText(displayName.c_str(), centerX(displayName.c_str()), entryY, {0, 255, 255, 255});
            
            const char* instructions = "Type name, ENTER to confirm, BACKSPACE to delete";
            printText(instructions, centerX(instructions), entryY + lineHeight, {200, 200, 200, 255});
        } else {
            const char* returnText = "Press ENTER to return to menu";
            printText(returnText, centerX(returnText), scaleY(440), {255, 255, 0, 255});
        }
    }
    
    void drawGame() {
        float scaleX = (float)w / 640.0f;
        float scaleY = (float)h / 480.0f;
        if (gamebg) {
            float time_f = static_cast<float>(SDL_GetTicks()) / 1000.0f;
            gamebg->setShaderParams(time_f, 0.0f, 0.0f, 0.0f);
            gamebg->drawSpriteRect(0, 0, w, h);
        }
        drawmatrix(scaleX, scaleY);
        drawblock(scaleX, scaleY);
        drawnext(scaleX, scaleY);
        
        std::ostringstream stream;
        stream << "Score: " << matrix.Game.score;
        printText(stream.str().c_str(), (int)(200 * scaleX), (int)(60 * scaleY)-10, {255, 255, 255, 255});
        
        stream.str("");
        stream << "Tabs: " << matrix.Game.lines;
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
        int c1 = matrix.block.color.c1;
        int c2 = matrix.block.color.c2;
        int c3 = matrix.block.color.c3;

        if (matrix.block.horizontal) {
            
            int screenY = STARTY + matrix.block.y * (BLOCK_HEIGHT + BLOCK_SPACING) + 10;
            for (int i = 0; i < 3; i++) {
                int color = (i == 0) ? c1 : (i == 1) ? c2 : c3;
                int bx = STARTX + (matrix.block.x + i) * (BLOCK_SIZE + BLOCK_SPACING);
                if (matrix.block.x + i >= 0 && matrix.block.x + i < GRID_WIDTH &&
                    color > 0 && color < BLOCK_COUNT && grid_blocks[color]) {
                    grid_blocks[color]->drawSpriteRect(
                        (int)(bx * scaleX), (int)(screenY * scaleY),
                        (int)(BLOCK_SIZE * scaleX), (int)(BLOCK_HEIGHT * scaleY)
                    );
                }
            }
        } else {
            
            int bx = STARTX + matrix.block.x * (BLOCK_SIZE + BLOCK_SPACING);
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
                applyGravity();
                
                
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
        
        if (matrix.block.horizontal) {
            
            if (by + 1 >= GRID_HEIGHT) return false;
            for (int i = 0; i < 3; i++) {
                if (bx + i >= 0 && bx + i < GRID_WIDTH && matrix.grid[bx + i][by + 1] > 0) return false;
            }
        } else {
            
            if (by + 3 >= GRID_HEIGHT) return false;
            if (matrix.grid[bx][by + 3] > 0) return false;
        }
        
        return true;
    }
    
    bool canMoveLeft() {
        if (matrix.block.x <= 0) return false;
        int bx = matrix.block.x - 1;
        int by = matrix.block.y;
        
        if (matrix.block.horizontal) {
            
            if (by >= 0 && by < GRID_HEIGHT && matrix.grid[bx][by] > 0) return false;
        } else {
            
            if (by >= 0 && by < GRID_HEIGHT && matrix.grid[bx][by] > 0) return false;
            if (by + 1 >= 0 && by + 1 < GRID_HEIGHT && matrix.grid[bx][by + 1] > 0) return false;
            if (by + 2 >= 0 && by + 2 < GRID_HEIGHT && matrix.grid[bx][by + 2] > 0) return false;
        }
        
        return true;
    }
    
    bool canMoveRight() {
        int by = matrix.block.y;
        
        if (matrix.block.horizontal) {
            
            if (matrix.block.x + 3 >= GRID_WIDTH) return false;
            int bx = matrix.block.x + 3;
            if (by >= 0 && by < GRID_HEIGHT && matrix.grid[bx][by] > 0) return false;
        } else {
            
            if (matrix.block.x >= GRID_WIDTH - 1) return false;
            int bx = matrix.block.x + 1;
            if (by >= 0 && by < GRID_HEIGHT && matrix.grid[bx][by] > 0) return false;
            if (by + 1 >= 0 && by + 1 < GRID_HEIGHT && matrix.grid[bx][by + 1] > 0) return false;
            if (by + 2 >= 0 && by + 2 < GRID_HEIGHT && matrix.grid[bx][by + 2] > 0) return false;
        }
        
        return true;
    }
    
    void placeBlock() {
        int bx = matrix.block.x;
        int by = matrix.block.y;
        
        if (matrix.block.horizontal) {
            
            if (by >= 0 && by < GRID_HEIGHT) {
                if (bx >= 0 && bx < GRID_WIDTH) matrix.grid[bx][by] = matrix.block.color.c1;
                if (bx + 1 >= 0 && bx + 1 < GRID_WIDTH) matrix.grid[bx + 1][by] = matrix.block.color.c2;
                if (bx + 2 >= 0 && bx + 2 < GRID_WIDTH) matrix.grid[bx + 2][by] = matrix.block.color.c3;
            }
        } else {
            
            if (by >= 0 && by < GRID_HEIGHT)
                matrix.grid[bx][by] = matrix.block.color.c1;
            if (by + 1 >= 0 && by + 1 < GRID_HEIGHT)
                matrix.grid[bx][by + 1] = matrix.block.color.c2;
            if (by + 2 >= 0 && by + 2 < GRID_HEIGHT)
                matrix.grid[bx][by + 2] = matrix.block.color.c3;
        }
    }
    
    bool findMatches() {
        
        memset(flashGrid, false, sizeof(flashGrid));
        bool foundMatch = false;
        matchBonus = 0;  
        
        
        for (int j = 0; j < GRID_HEIGHT; j++) {
            for (int i = 0; i < GRID_WIDTH; i++) {
                if (matrix.grid[i][j] > 0) {
                    int color = matrix.grid[i][j];
                    int count = 1;
                    
                    while (i + count < GRID_WIDTH && matrix.grid[i + count][j] == color) {
                        count++;
                    }
                    if (count >= 3) {
                        for (int k = 0; k < count; k++) {
                            flashGrid[i + k][j] = true;
                        }
                        foundMatch = true;
                        
                        if (count == 4) matchBonus += 25;
                        else if (count >= 5) matchBonus += 50;
                    }
                }
            }
        }
        
        
        for (int i = 0; i < GRID_WIDTH; i++) {
            for (int j = 0; j < GRID_HEIGHT; j++) {
                if (matrix.grid[i][j] > 0) {
                    int color = matrix.grid[i][j];
                    int count = 1;
                    
                    while (j + count < GRID_HEIGHT && matrix.grid[i][j + count] == color) {
                        count++;
                    }
                    if (count >= 3) {
                        for (int k = 0; k < count; k++) {
                            flashGrid[i][j + k] = true;
                        }
                        foundMatch = true;
                        
                        if (count == 4) matchBonus += 25;
                        else if (count >= 5) matchBonus += 50;
                    }
                }
            }
        }
        
        
        for (int i = 0; i < GRID_WIDTH; i++) {
            for (int j = 0; j < GRID_HEIGHT; j++) {
                if (matrix.grid[i][j] > 0) {
                    int color = matrix.grid[i][j];
                    int count = 1;
                    while (i + count < GRID_WIDTH && j + count < GRID_HEIGHT && 
                           matrix.grid[i + count][j + count] == color) {
                        count++;
                    }
                    if (count >= 3) {
                        for (int k = 0; k < count; k++) {
                            flashGrid[i + k][j + k] = true;
                        }
                        foundMatch = true;
                        
                        if (count == 4) matchBonus += 35;
                        else if (count >= 5) matchBonus += 75;
                    }
                }
            }
        }
        
        
        for (int i = 0; i < GRID_WIDTH; i++) {
            for (int j = 0; j < GRID_HEIGHT; j++) {
                if (matrix.grid[i][j] > 0) {
                    int color = matrix.grid[i][j];
                    int count = 1;
                    while (i - count >= 0 && j + count < GRID_HEIGHT && 
                           matrix.grid[i - count][j + count] == color) {
                        count++;
                    }
                    if (count >= 3) {
                        for (int k = 0; k < count; k++) {
                            flashGrid[i - k][j + k] = true;
                        }
                        foundMatch = true;
                        
                        if (count == 4) matchBonus += 35;
                        else if (count >= 5) matchBonus += 75;
                    }
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
        
        matrix.Game.score += matchBonus;
        matchBonus = 0;
        
        memset(flashGrid, false, sizeof(flashGrid));
        
        
        spawnNewBlock();
        
        
        if (checkGameOver()) {
            currentScreen = SCREEN_GAMEOVER;
        }
    }
    
    void applyGravity() {
        bool moved;
        do {
            moved = false;
            for (int i = 0; i < GRID_WIDTH; i++) {
                for (int j = GRID_HEIGHT - 2; j >= 0; j--) {
                    if (matrix.grid[i][j] > 0 && matrix.grid[i][j + 1] == 0) {
                        
                        matrix.grid[i][j + 1] = matrix.grid[i][j];
                        matrix.grid[i][j] = 0;
                        moved = true;
                    }
                }
            }
        } while (moved);  
    }
    
    void spawnNewBlock() {
        matrix.block.color = matrix.nextblock.color;
        matrix.nextblock.color.randcolor();
        matrix.block.x = GRID_WIDTH / 2;
        matrix.block.y = 1;
        matrix.block.horizontal = false;
    }
    
    bool checkGameOver() {
        
        for (int i = 0; i < GRID_WIDTH; i++) {
            if (matrix.grid[i][1] > 0) {
                return true;
            }
        }
        
        for (int i = 0; i < GRID_WIDTH; i++) {
            for (int j = 0; j < GRID_HEIGHT; j++) {
                if (matrix.grid[i][j] == 0) {
                    return false;
                }
            }
        }
        
        return true;
    }
    
    void applyDifficulty() {
        
        int baseSpeeds[] = {30, 20, 10};
        matrix.Game.speed = baseSpeeds[difficultySetting];
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
    
    bool canRotate() {
        int bx = matrix.block.x;
        int by = matrix.block.y;
        
        if (matrix.block.horizontal) {
            
            
            if (by + 2 >= GRID_HEIGHT) return false;
            
            if (by + 1 < GRID_HEIGHT && matrix.grid[bx][by + 1] > 0) return false;
            if (by + 2 < GRID_HEIGHT && matrix.grid[bx][by + 2] > 0) return false;
        } else {
            
            
            if (bx + 2 >= GRID_WIDTH) return false;
            
            if (bx + 1 < GRID_WIDTH && by >= 0 && by < GRID_HEIGHT && matrix.grid[bx + 1][by] > 0) return false;
            if (bx + 2 < GRID_WIDTH && by >= 0 && by < GRID_HEIGHT && matrix.grid[bx + 2][by] > 0) return false;
        }
        return true;
    }
    
    void rotateBlock() {
        if (canRotate()) {
            matrix.block.horizontal = !matrix.block.horizontal;
        }
    }
    
    void event(SDL_Event& e) override {
        if(currentScreen == SCREEN_START && e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
            quit();
            return;
        }
        if (e.type == SDL_KEYDOWN) {
            gameKeypress(e.key.keysym.sym);
        } else if (e.type == SDL_TEXTINPUT) {
            handleTextInput(e.text.text);
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
                    cursorPos = (cursorPos - 1 + 5) % 5;
                } else if (key == SDLK_DOWN) {
                    cursorPos = (cursorPos + 1) % 5;
                } else if (key == SDLK_RETURN) {
                    if (cursorPos == 0) {  
                        matrix.init_matrix();
                        applyDifficulty();
                        matrix.block.x = GRID_WIDTH / 2;
                        matrix.block.y = 1;
                        matrix.block.horizontal = false;
                        currentScreen = SCREEN_GAME;
                    } else if (cursorPos == 1) {  
                        finalScore = 0;
                        enteringName = false;
                        currentScreen = SCREEN_SCORES;
                    } else if (cursorPos == 2) {  
                        currentScreen = SCREEN_OPTIONS;
                    } else if (cursorPos == 3) {  
                        currentScreen = SCREEN_CREDITS;
                    } else if (cursorPos == 4) {  
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
                    rotateBlock();
                } else if (key == SDLK_z) {
                    matrix.block.color.shiftcolor(false);
                } else if (key == SDLK_p) {
                    paused = !paused;
                } else if (key == SDLK_ESCAPE) {
                    currentScreen = SCREEN_START;
                }
                break;
            case SCREEN_GAMEOVER:
                if (key == SDLK_RETURN) {
                    goToScoresScreen();
                } else if (key == SDLK_ESCAPE) {
                    currentScreen = SCREEN_START;
                }
                break;
            case SCREEN_OPTIONS:
                if (key == SDLK_UP) {
                    optionsCursor = (optionsCursor - 1 + 2) % 2;
                } else if (key == SDLK_DOWN) {
                    optionsCursor = (optionsCursor + 1) % 2;
                } else if (key == SDLK_LEFT) {
                    if (optionsCursor == 0) {
                        difficultySetting = (difficultySetting - 1 + 3) % 3;
                    }
                } else if (key == SDLK_RIGHT) {
                    if (optionsCursor == 0) {
                        difficultySetting = (difficultySetting + 1) % 3;
                    }
                } else if (key == SDLK_RETURN) {
                    if (optionsCursor == 1) {
                        currentScreen = SCREEN_START;
                    }
                }
                break;
            case SCREEN_CREDITS:
                if (key == SDLK_RETURN) {
                    currentScreen = SCREEN_START;
                }
                break;
            case SCREEN_SCORES:
                if (enteringName) {
                    if (key == SDLK_RETURN && !playerName.empty()) {
                        
                        scores.addScore(playerName, finalScore);
                        enteringName = false;
                    } else if (key == SDLK_BACKSPACE && !playerName.empty()) {
                        playerName.pop_back();
                    } else if (key == SDLK_ESCAPE) {
                        
                        enteringName = false;
                    }
                } else {
                    if (key == SDLK_RETURN || key == SDLK_ESCAPE) {
                        currentScreen = SCREEN_START;
                    }
                }
                break;
        }
    }
    
    void handleTextInput(const char* text) {
        if (currentScreen == SCREEN_SCORES && enteringName) {
            
            if (playerName.length() < 15) {
                playerName += text;
            }
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
