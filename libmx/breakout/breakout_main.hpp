#ifndef GAME_H
#define GAME_H

#include "breakout_game.hpp"
#include "mx.hpp"
#include <vector>
#include <string>

class Breakout {
public:
    Breakout() : mode(2), endScore(0), score(0) {
        setup();
    }

    void setWindow(mx::mxWindow *win) {
        background.loadTexture(win, win->util.getFilePath("data/bg.png"));
        startImg.loadTexture(win, win->util.getFilePath("data/img/start.png"));
        gameoverImg.loadTexture(win, win->util.getFilePath("data/img/gameover.png"));

        std::vector<std::string> imgFiles = {
            "data/img/block_ltblue.png",
            "data/img/block_dblue.png",
            "data/img/block_pink.png",
            "data/img/block_yellow.png",
            "data/img/block_red.png",
            "data/img/block_red.png"
        };

        for(size_t i = 0; i < imgFiles.size(); ++i){
            imgTex[i].loadTexture(win, win->util.getFilePath(imgFiles[i]));
        }
        the_font.loadFont(win->util.getFilePath("data/font.ttf"),14);
    }

    void draw(mx::mxWindow *win) {
        SDL_SetRenderDrawColor(win->renderer, 0, 0, 0, 255);
        
        win->text.setColor({255,255,255,255});

        if (mode == 2) {
            SDL_RenderCopy(win->renderer, startImg.wrapper().unwrap(), nullptr, nullptr);
            win->text.printText_Solid(the_font, 25, 25, "Press Enter to Play");

        } else if (mode == 0) {
            SDL_RenderCopy(win->renderer, background.wrapper().unwrap(), nullptr, nullptr);
            grid.draw(win->renderer, imgTex);
            paddle.draw(win->renderer);
            ball.draw(win->renderer);
            std::string scoreText = "Score: " + std::to_string(ball.score) + " Lives: " + std::to_string(ball.lives);
            win->text.printText_Solid(the_font, 15, 275, scoreText);
            updateLogic();
        } else if (mode == 1) {
            SDL_RenderCopy(win->renderer, gameoverImg.wrapper().unwrap(), nullptr, nullptr);
            std::string gameOverText = "Game Over - [ Score: " + std::to_string(endScore) + " ] Press Return";
            win->text.printText_Solid(the_font, 100, 100, gameOverText);
        }
    }

    void handleEvent(const SDL_Event& e) {
        if (e.type == SDL_KEYDOWN || e.type == SDL_JOYBUTTONDOWN) {
            if (mode == 1 && (e.key.keysym.sym == SDLK_RETURN || e.jbutton.button == 1)) {
                setup();
            } else if (mode == 2 && (e.key.keysym.sym == SDLK_RETURN || e.jbutton.button == 2)) {
                setup();
                mode = 0;
                ball.score = 0;
                ball.lives = 4;
            }
        }
    }

    void processInput() {
        if (mode == 0) {
            const Uint8* keyStates = SDL_GetKeyboardState(nullptr);
            if (keyStates[SDL_SCANCODE_LEFT] || stick.getHat(0) & SDL_HAT_LEFT) paddle.moveLeft();
            if (keyStates[SDL_SCANCODE_RIGHT] || stick.getHat(0) & SDL_HAT_RIGHT) paddle.moveRight();
        }
    }

    void updateLogic() {
        if (mode == 0) {
            static Uint32 previous_time = SDL_GetTicks();
            Uint32 current_time = SDL_GetTicks();
            if (current_time - previous_time >= 15) {
                processInput();
                previous_time = current_time;
            
                ball.update(paddle, grid);
                if (ball.lives <= 0) {
                    endScore = ball.score;
                    mode = 1;
                    ball.lives = 4;
                    ball.score = 0;
                } else if (grid.isEmpty()) {
                    grid.reset();
                }
            }
        }
    }

    ~Breakout() {
    }

private:
    Paddle paddle;
    Grid grid{ 1280 / 32, (720 / 4) / 16 };
    Ball ball;
    mx::Joystick stick;

    mx::Texture background, startImg , gameoverImg;
    mx::Font the_font;
    mx::Texture imgTex[7];

    int mode;
    int endScore;
    int score;
    
    void setup() {
        grid.reset();
        ball.reset(true);
        score = 0;
        mode = 2;
        endScore = 0;
    }
};

#endif 
