#ifndef __GAME__H_
#define __GAME__H_

#include"mx.hpp"

class Game : public obj::Object {
public:
    Game() = default;
    virtual ~Game() {}

    constexpr static int tex_width = 640, tex_height = 480;

    virtual void load(mx::mxWindow* win) override {
        std::srand(static_cast<unsigned>(std::time(0)));
        for (int i = 0; i < 5; ++i) {
            spawnAsteroid();
        }
        resetShip();
        score = 0;
        lives = 3;

        const int numStars = 100; 
        for (int i = 0; i < numStars; ++i) {
            Star star;
            star.x = static_cast<float>(std::rand() % tex_width);
            star.y = static_cast<float>(std::rand() % tex_height);
            star.speed = 0.5f + static_cast<float>(std::rand() % 100) / 100.0f; 
            stars.push_back(star);
        }
        the_font.loadFont(win->util.getFilePath("data/font.ttf"), 14);
    }

    void drawStars(SDL_Renderer* renderer) {
        for (const auto& star : stars) {
            int x = static_cast<int>(star.x);
            int y = static_cast<int>(star.y);

            Uint8 brightness = static_cast<Uint8>(150 + star.speed * 70); 

            SDL_SetRenderDrawColor(renderer, brightness, brightness, brightness, 255);
            SDL_RenderDrawPoint(renderer, x, y);
        }
    }

    bool update(mx::mxWindow* win) {
        static Uint32 lastUpdate = SDL_GetTicks();
        Uint32 current = SDL_GetTicks();
        Uint32 delta = current - lastUpdate;

        if (delta >= 16) {
            lastUpdate = current;
            updateShip();
            updateAsteroids();
            updateBullets();
            if(checkShipCollision(win)) {
                return true;
            }

            const Uint8* state = SDL_GetKeyboardState(NULL);
            if (state[SDL_SCANCODE_LEFT]) {
                ship.angle -= 5;
            }
            if (state[SDL_SCANCODE_RIGHT]) {
                ship.angle += 5;
            }
            if (state[SDL_SCANCODE_UP]) {
                ship.speed += 0.1;
            }

            static Uint32 lastFireTime = 0;
            static const Uint32 fireCooldown = 300;
            Uint32 currentTime = SDL_GetTicks();
            if (state[SDL_SCANCODE_SPACE] && currentTime - lastFireTime >= fireCooldown) {
                fireBullet();
                lastFireTime = currentTime;
            }

            for (auto& star : stars) {
                star.y += star.speed;
                if (star.y >= tex_height) {
                    star.y -= tex_height;
                    star.x = static_cast<float>(std::rand() % tex_width);
                }
            }
            
        }
        return false;
    }

    virtual void draw(mx::mxWindow* win) override;

    virtual void event(mx::mxWindow* win, SDL_Event& e) override {
        
    }

private:
    mx::Font the_font;
    int score = 0;
    int lives = 3;

    struct Ship {
        float x, y;
        float angle;
        float speed;
    } ship{tex_width / 2.0f, tex_height / 2.0f, 0.0f, 0.0f};

    struct Asteroid {
        float x, y;
        float dx, dy;
        float radius;
    };

    struct Bullet {
        float x, y;
        float dx, dy;
    };

    struct Star {
        float x, y;
        float speed;
    };

    std::vector<Asteroid> asteroids;
    std::vector<Bullet> bullets;
    std::vector<Star> stars;

    void spawnAsteroid() {
        Asteroid asteroid;
        asteroid.x = static_cast<float>(std::rand() % tex_width);
        asteroid.y = static_cast<float>(std::rand() % tex_height);
        asteroid.dx = (std::rand() % 100 - 50) / 100.0f;
        asteroid.dy = (std::rand() % 100 - 50) / 100.0f;
        asteroid.radius = 20 + std::rand() % 30;
        asteroids.push_back(asteroid);
    }

    void updateAsteroids() {
        for (auto& asteroid : asteroids) {
            asteroid.x += asteroid.dx;
            asteroid.y += asteroid.dy;
            if (asteroid.x < 0) asteroid.x += tex_width;
            if (asteroid.x > tex_width) asteroid.x -= tex_width;
            if (asteroid.y < 0) asteroid.y += tex_height;
            if (asteroid.y > tex_height) asteroid.y -= tex_height;
        }

        if (asteroids.size() < 5) {
            while (asteroids.size() < 5) {
                spawnAsteroid();
            }
        }
    }

    void updateBullets() {
        std::vector<Asteroid> newAsteroids;

        for (auto bulletIt = bullets.begin(); bulletIt != bullets.end();) {
            bulletIt->x += bulletIt->dx;
            bulletIt->y += bulletIt->dy;

            if (bulletIt->x < 0 || bulletIt->x > tex_width || bulletIt->y < 0 || bulletIt->y > tex_height) {
                bulletIt = bullets.erase(bulletIt);
                continue;
            }

            bool bulletErased = false;
            for (auto asteroidIt = asteroids.begin(); asteroidIt != asteroids.end();) {
                float dist = std::sqrt(std::pow(bulletIt->x - asteroidIt->x, 2) + std::pow(bulletIt->y - asteroidIt->y, 2));

                if (dist <= asteroidIt->radius) {
                    bulletIt = bullets.erase(bulletIt);
                    bulletErased = true;
                    score += 100;
                    if (asteroidIt->radius > 10) {
                        for (int i = 0; i < 2; ++i) {
                            Asteroid smallAsteroid;
                            smallAsteroid.x = asteroidIt->x;
                            smallAsteroid.y = asteroidIt->y;
                            smallAsteroid.radius = asteroidIt->radius / 2;
                            smallAsteroid.dx = (std::rand() % 100 - 50) / 100.0f;
                            smallAsteroid.dy = (std::rand() % 100 - 50) / 100.0f;
                            newAsteroids.push_back(smallAsteroid);
                        }
                    }

                    asteroidIt = asteroids.erase(asteroidIt);
                    break;
                } else {
                    ++asteroidIt;
                }
            }

            if (!bulletErased) {
                ++bulletIt;
            }
        }

        asteroids.insert(asteroids.end(), newAsteroids.begin(), newAsteroids.end());
    }

    void resetShip() {
        ship.x = tex_width / 2.0f;
        ship.y = tex_height / 2.0f;
        ship.angle = 0;
        ship.speed = 0;
    }

    bool checkShipCollision(mx::mxWindow *win) {
        for (const auto& asteroid : asteroids) {
            float dist = std::sqrt(std::pow(ship.x - asteroid.x, 2) + std::pow(ship.y - asteroid.y, 2));
            if (dist <= asteroid.radius) {
                lives--;
                if (lives > 0) {
                    resetShip();
                } else {
                    lives = 3;
                    score = 0;
                    resetShip();
                    asteroids.clear();
                    bullets.clear();
                    for (int i = 0; i < 5; ++i) {
                        spawnAsteroid();
                    }
                    return true;
                }
                break;
            }
        }
        return false;
    }
    void updateShip() {
        ship.x += ship.speed * std::cos(ship.angle * M_PI / 180.0);
        ship.y += ship.speed * std::sin(ship.angle * M_PI / 180.0);
        if (ship.x < 0) ship.x += tex_width;
        if (ship.x > tex_width) ship.x -= tex_width;
        if (ship.y < 0) ship.y += tex_height;
        if (ship.y > tex_height) ship.y -= tex_height;
        ship.speed *= 0.99f; 
    }

    template<typename T>
    T min(T one, T two){
        return one < two ? one : two;
    }

    template<typename T>
    T max(T one, T two) {
        return one > two ? one : two;
    }
    

    void drawShip(SDL_Renderer* renderer, int ship_x, int ship_y, float ship_angle) {
        SDL_Color color_inner = {200, 200, 200, 255};
        SDL_Color color_outer = {100, 100, 100, 255};

        SDL_Point points[3];
        points[0] = {static_cast<int>(ship_x + 15 * std::cos(ship_angle * M_PI / 180.0)),
                     static_cast<int>(ship_y + 15 * std::sin(ship_angle * M_PI / 180.0))};
        points[1] = {static_cast<int>(ship_x + 15 * std::cos((ship_angle + 140) * M_PI / 180.0)),
                     static_cast<int>(ship_y + 15 * std::sin((ship_angle + 140) * M_PI / 180.0))};
        points[2] = {static_cast<int>(ship_x + 15 * std::cos((ship_angle + 220) * M_PI / 180.0)),
                     static_cast<int>(ship_y + 15 * std::sin((ship_angle + 220) * M_PI / 180.0))};

        int minY = points[0].y;
        int maxY = points[0].y;

        if (points[1].y < minY) minY = points[1].y;
        if (points[2].y < minY) minY = points[2].y;

        if (points[1].y > maxY) maxY = points[1].y;
        if (points[2].y > maxY) maxY = points[2].y;

        for (int y = minY; y <= maxY; y++) {
            int startX = tex_width, endX = 0;
            for (int i = 0; i < 3; i++) {
                int next = (i + 1) % 3;
                if ((points[i].y <= y && points[next].y > y) || (points[next].y <= y && points[i].y > y)) {
                    float t = static_cast<float>(y - points[i].y) / (points[next].y - points[i].y);
                    int x = static_cast<int>(points[i].x + t * (points[next].x - points[i].x));
                    if (x < startX) {
                        startX = x;
                    }
                    if (x > endX) {
                        endX = x;
                    }
                }
            }

            for (int x = startX; x <= endX; x++) {
                float t = static_cast<float>(x - startX) / (endX - startX);
                Uint8 r = static_cast<Uint8>(color_outer.r * (1 - t) + color_inner.r * t);
                Uint8 g = static_cast<Uint8>(color_outer.g * (1 - t) + color_inner.g * t);
                Uint8 b = static_cast<Uint8>(color_outer.b * (1 - t) + color_inner.b * t);
                SDL_SetRenderDrawColor(renderer, r, g, b, 255);
                SDL_RenderDrawPoint(renderer, x, y);
            }
        }
    }

    void drawAsteroids(SDL_Renderer* renderer) {
        for (const auto& asteroid : asteroids) {
            int rounded_x = static_cast<int>(std::round(asteroid.x));
            int rounded_y = static_cast<int>(std::round(asteroid.y));
            int radius = static_cast<int>(std::round(asteroid.radius));

            draw_circle(renderer, rounded_x, rounded_y, radius);
        }
    }

    void draw_circle(SDL_Renderer* renderer, int center_x, int center_y, int radius) {
        for (int y = -radius; y <= radius; y++) {
            for (int x = -radius; x <= radius; x++) {
                int dist_sq = x * x + y * y;
                if (dist_sq <= radius * radius) {
                    float dist = std::sqrt(static_cast<float>(dist_sq));
                    float shade = 255 * (1 - (dist / radius));

                    if (shade < 0) shade = 0;
                    if (shade > 255) shade = 255;

                    SDL_SetRenderDrawColor(renderer, static_cast<Uint8>(shade), static_cast<Uint8>(shade), static_cast<Uint8>(shade), 255);
                    SDL_RenderDrawPoint(renderer, center_x + x, center_y + y);
                }
            }
        }
    }

    void drawBullets(SDL_Renderer* renderer) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        for (const auto& bullet : bullets) {
            SDL_Rect rect = {static_cast<int>(bullet.x - 2),
                             static_cast<int>(bullet.y - 2),
                             4, 4};
            SDL_RenderFillRect(renderer, &rect);
        }
    }

    void fireBullet() {
        Bullet bullet;
        bullet.x = ship.x;
        bullet.y = ship.y;
        bullet.dx = 5 * std::cos(ship.angle * M_PI / 180.0);
        bullet.dy = 5 * std::sin(ship.angle * M_PI / 180.0);
        bullets.push_back(bullet);
    }
};

#endif