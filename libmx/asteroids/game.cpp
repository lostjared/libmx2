#include"game.hpp"
#include"gameover.hpp"

void Game::draw(mx::mxWindow* win)  {
    SDL_Renderer* renderer = win->renderer;
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    drawStars(renderer);
    drawShip(renderer, static_cast<int>(ship.x), static_cast<int>(ship.y), ship.angle);
    drawAsteroids(renderer);
    drawBullets(renderer);

    win->text.setColor({255, 255, 255, 255});
    win->text.printText_Solid(the_font, 25, 25, "Lives: " + std::to_string(lives));
    win->text.printText_Solid(the_font, 25, 50, "Score: " + std::to_string(score));
    if(update(win)) {
        win->setObject(new GameOver());
        win->object->load(win);
    }
}