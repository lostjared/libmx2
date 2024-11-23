#ifndef BREAKOUT_H__
#define BREAKOUT_H__
#include"mx.hpp"
#include "SDL.h"
#include <vector>
#include <random>

class GradientRenderer {
public:
    static void drawGradientRect(SDL_Renderer* renderer, const SDL_Rect& rect, SDL_Color startColor, SDL_Color endColor) {
        float colorStep[3] = {
            (float)(endColor.r - startColor.r) / rect.h,
            (float)(endColor.g - startColor.g) / rect.h,
            (float)(endColor.b - startColor.b) / rect.h
        };
        for (int row = 0; row < rect.h; ++row) {
            SDL_Color currentColor = {
                (Uint8)(startColor.r + colorStep[0] * row),
                (Uint8)(startColor.g + colorStep[1] * row),
                (Uint8)(startColor.b + colorStep[2] * row),
                255
            };
            SDL_SetRenderDrawColor(renderer, currentColor.r, currentColor.g, currentColor.b, currentColor.a);
            SDL_Rect rowRect = { rect.x, rect.y + row, rect.w, 1 };
            SDL_RenderFillRect(renderer, &rowRect);
        }
    }
};

class Paddle {
public:
    int  width = 200, y, x;
    Paddle() : width(200), y(720 / 2 + 150), x(1280 / 2 - width / 2) {}
    void draw(SDL_Renderer* renderer) {
        SDL_Rect rect = { x, y, width, 25 };
        GradientRenderer::drawGradientRect(renderer, rect, { 150, 150, 150, 255 }, { 255, 255, 255, 255 });
    }
    void moveLeft() { if (x > 10) x -= 10; }
    void moveRight() { if (x + width < 1280 - 10) x += 10; }
};

class Grid {
public:
    int cols, rows;
    std::vector<std::vector<int>> grid;
    mx::Texture tex;
    Grid(int width, int height) : cols(width), rows(height) { reset(); }
    void load(mx::mxWindow *win) {
        
    }
    void reset() {
        grid.assign(cols, std::vector<int>(rows));
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0, 5);
        for (auto& col : grid)
            for (auto& cell : col)
                cell = dist(gen);
    }
    void draw(SDL_Renderer* renderer, mx::Texture *textures) {
        for (int x = 0; x < cols; ++x) {
            for (int y = 0; y < rows; ++y) {
                if(grid[x][y] != 0) {
                    SDL_Rect rect = { x * 32, y * 16, 32, 16 };
                    SDL_RenderCopy(renderer, textures[grid[x][y]].wrapper().unwrap(), nullptr, &rect);
                }
            }
        }
    }
    bool isEmpty() const {
        for (const auto& col : grid)
            for (int cell : col)
                if (cell != 0) return false;
        return true;
    }
};

class Ball {
public:
    int x, y, vx, vy,  lives, score;
    Ball() : lives(4), score(0) { reset(true); }
    void reset(bool init = false) {
        x = 1280 / 2;
        y = 720 / 2 - 50;
        vx = (rand() % 2 == 0 ? -5 : 5);
        vy = -5;
        if(init == false)
            --lives;    
    }

    void draw(SDL_Renderer* renderer) {
        SDL_Rect rect = { x, y, 16, 16 };
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &rect);
    }
    void update(Paddle& paddle, Grid& grid) {
        x += vx;
        y += vy;
        if (x <= 0 || x >= 1280 - 16) vx = -vx;
        if (y <= 0) vy = -vy;
        if (y > paddle.y + 25) {
            if (y > 720) reset();
            return;
        }
        if ((x + 16 >= paddle.x && x <= paddle.x + paddle.width) && (y + 16 >= paddle.y)) {
            vy = -vy;
        }
        for (int col = 0; col < grid.cols; ++col) {
            for (int row = 0; row < grid.rows; ++row) {
                if (grid.grid[col][row] > 0) {
                    int brickX = col * 32;
                    int brickY = row * 16;
                    if ((x + 16 > brickX && x < brickX + 32) &&
                        (y + 16 > brickY && y < brickY + 16)) {
                        vy = -vy;
                        grid.grid[col][row] = 0;
                        score += 100;
                    }
                }
            }
        }
        if (y > 720) reset();
    }
};

#endif 
