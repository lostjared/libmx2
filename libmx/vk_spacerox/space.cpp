

#include "vk.hpp"
#include "SDL.h"
#include <random>
#include <cmath>
#include <format>
#include <ctime>
#include <climits>
#include <cstring>
#include "argz.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


constexpr int GAME_W = 640;
constexpr int GAME_H = 360;
constexpr int STAR_COUNT = 800;
constexpr int MAX_PROJECTILES = 50;
constexpr int MAX_ASTEROIDS = 30;
constexpr int MAX_PARTICLES = 40;
constexpr int ASTEROID_VERTICES = 8;
constexpr float PROJECTILE_SPEED = 5.0f;
constexpr int PROJECTILE_LIFETIME = 60;
constexpr int FIRE_COOLDOWN = 5;
constexpr int SHOTS_PER_BURST = 5;
constexpr int FIRE_DELAY = 3;
constexpr int EXPLOSION_DURATION = 90;
constexpr float MIN_ASTEROID_RADIUS = 8.0f;
constexpr int LARGE_ASTEROID_POINTS = 20;
constexpr int MEDIUM_ASTEROID_POINTS = 50;
constexpr int SMALL_ASTEROID_POINTS = 100;


struct Ship {
    float x, y;
    float vx, vy;
    float angle;
    int lives;
    int fire_cooldown;
    int burst_count;
    bool exploding;
    int explosion_timer;
    int score;
    int continuous_fire_timer;
    bool overheated;
    int overheat_cooldown;
};

struct Projectile {
    float x, y;
    float vx, vy;
    float lifetime;
    bool active;
};

struct Asteroid {
    float x, y;
    float vx, vy;
    float radius;
    bool active;
    float vertices[ASTEROID_VERTICES][2];
    float rotation_angle;
    float rotation_speed;
};

struct Particle {
    float x, y;
    float vx, vy;
    int lifetime;
    bool active;
};

struct Star {
    float x, y, speed;
    float size;
    float brightness;
    int layer; 
    float r, g, b; 
    float twinklePhase;
    float twinkleSpeed;
};

enum GameState {
    STATE_COUNTDOWN,
    STATE_LAUNCH,
    STATE_PLAYING,
    STATE_GAMEOVER
};



template<typename Func>
void bresenhamLine(int x0, int y0, int x1, int y1, Func plot) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    while (true) {
        plot(x0, y0);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx)  { err += dx; y0 += sy; }
    }
}




class SpaceRoxWindow : public mx::VKWindow {
public:
    SpaceRoxWindow(const std::string &path, int wx, int wy, bool full)
        : mx::VKWindow("-[ SpaceRox - Vulkan ]-", wx, wy, full) {
        setPath(path);
        srand(static_cast<unsigned>(time(nullptr)));
    }
    virtual ~SpaceRoxWindow() {}

    
    void initVulkan() override {
        mx::VKWindow::initVulkan();
        
        SDL_Surface *whiteSurf = SDL_CreateRGBSurfaceWithFormat(0, 2, 2, 32, SDL_PIXELFORMAT_RGBA32);
        SDL_FillRect(whiteSurf, nullptr, SDL_MapRGBA(whiteSurf->format, 255, 255, 255, 255));
        pixel = createSprite(whiteSurf,
                             util.path + "/data/sprite_vert.spv",
                             util.path + "/data/solid_frag.spv");
        SDL_FreeSurface(whiteSurf);
        
        
        SDL_Surface *starSurf = SDL_CreateRGBSurfaceWithFormat(0, 4, 4, 32, SDL_PIXELFORMAT_RGBA32);
        Uint32 *pixels = (Uint32*)starSurf->pixels;
        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 4; x++) {
                float dx = (x - 1.5f) / 1.5f;
                float dy = (y - 1.5f) / 1.5f;
                float dist = sqrtf(dx*dx + dy*dy);
                Uint8 alpha = (dist < 1.0f) ? (Uint8)(255 * (1.0f - dist)) : 0;
                pixels[y * 4 + x] = SDL_MapRGBA(starSurf->format, 255, 255, 255, alpha);
            }
        }
        starSprite = createSprite(starSurf,
                                  util.path + "/data/sprite_vert.spv",
                                  util.path + "/data/sprite_frag.spv");
        SDL_FreeSurface(starSurf);

        initGame();
    }

    
    void onResize() override {
        vkDeviceWaitIdle(device);
        clearTextQueue();
        lastFontSize = 0;
        updateFontSize();
    }

    
    void proc() override {
        
        Uint32 currentTime = SDL_GetTicks();
        Uint32 frameTime = currentTime - lastFrameTime;
        if (frameTime < targetFrameTime) {
            SDL_Delay(targetFrameTime - frameTime);
            currentTime = SDL_GetTicks();
            frameTime = currentTime - lastFrameTime;
        }
        
        if (lastFrameTime > 0) {
            float deltaTime = frameTime / 1000.0f;
            globalTime += deltaTime;
        }
        lastFrameTime = currentTime;

        updateFontSize();

        
        const Uint8 *keys = SDL_GetKeyboardState(nullptr);

        switch (state) {
        case STATE_COUNTDOWN:
            updateCountdown();
            drawCountdown();
            break;
        case STATE_LAUNCH:
            updateLaunch();
            drawLaunch();
            break;
        case STATE_PLAYING:
            
            if (keys[SDL_SCANCODE_SPACE] && canFire()) {
                fireProjectile(ship.x, ship.y, ship.angle);
            }
            updateShip();
            updateFireTimer(keys);
            updateProjectiles();
            updateAsteroids();
            updateExplosion();
            checkAsteroidCollisions();
            checkShipAsteroidCollision();
            checkProjectileAsteroidCollisions();
            checkAndSpawnAsteroids();
            handleDeathSequence();
            drawGame();
            break;
        case STATE_GAMEOVER:
            drawGameOver();
            break;
        }
    }

    
    void event(SDL_Event &e) override {
        switch (e.type) {
        case SDL_QUIT:
            quit();
            break;
        case SDL_KEYDOWN:
            if (e.key.keysym.sym == SDLK_ESCAPE)
                quit();
            if (e.key.keysym.sym == SDLK_SPACE && state == STATE_GAMEOVER) {
                restartGame();
            }
            if (state == STATE_PLAYING) {
                if (e.key.keysym.sym == SDLK_LEFT)  keyLeft = true;
                if (e.key.keysym.sym == SDLK_RIGHT) keyRight = true;
                if (e.key.keysym.sym == SDLK_UP)    keyThrust = true;
            }
            break;
        case SDL_KEYUP:
            if (state == STATE_PLAYING) {
                if (e.key.keysym.sym == SDLK_LEFT)  keyLeft = false;
                if (e.key.keysym.sym == SDLK_RIGHT) keyRight = false;
                if (e.key.keysym.sym == SDLK_UP)    keyThrust = false;
            }
            break;
        }
    }

private:
    
    mx::VKSprite *pixel = nullptr;
    mx::VKSprite *starSprite = nullptr;

    
    Uint32 lastFrameTime = 0;
    static constexpr Uint32 targetFrameTime = 1000 / 60; 
    float globalTime = 0.0f;

    
    Ship ship{};
    Projectile projectiles[MAX_PROJECTILES]{};
    Asteroid asteroids[MAX_ASTEROIDS]{};
    Particle particles[MAX_PARTICLES]{};
    Star stars[STAR_COUNT * 3]{}; 
    bool starsInit = false;

    GameState state = STATE_COUNTDOWN;
    bool keyLeft = false, keyRight = false, keyThrust = false;
    bool wasExploding = false;

    
    int countdownTimer = 0;
    int countdownDuration = 60;
    int countdownNumber = 3;
    int launchTimer = 0;
    int launchDuration = 60;
    float shipLaunchY = 0;

    int lastFontSize = 0;

    
    float sx() const { return static_cast<float>(w) / GAME_W; }
    float sy() const { return static_cast<float>(h) / GAME_H; }

    int toScreenX(float gx) const { return static_cast<int>(gx * sx()); }
    int toScreenY(float gy) const { return static_cast<int>(gy * sy()); }
    int toScreenW(float gw) const { return std::max(1, static_cast<int>(gw * sx())); }
    int toScreenH(float gh) const { return std::max(1, static_cast<int>(gh * sy())); }

    
    void setColor(float r, float g, float b, float a = 1.0f) {
        pixel->setShaderParams(r, g, b, a);
    }

    void drawPixel(float gx, float gy) {
        int psz = std::max(1, static_cast<int>(2.0f * std::min(sx(), sy())));
        pixel->drawSpriteRect(toScreenX(gx), toScreenY(gy), psz, psz);
    }

    void drawRect(float gx, float gy, float gw, float gh) {
        pixel->drawSpriteRect(toScreenX(gx), toScreenY(gy), toScreenW(gw), toScreenH(gh));
    }

    void drawLine(float x0f, float y0f, float x1f, float y1f) {
        int x0 = toScreenX(x0f), y0 = toScreenY(y0f);
        int x1 = toScreenX(x1f), y1 = toScreenY(y1f);
        int psz = std::max(1, static_cast<int>(1.5f * std::min(sx(), sy())));
        bresenhamLine(x0, y0, x1, y1, [&](int px, int py) {
            pixel->drawSpriteRect(px, py, psz, psz);
        });
    }

    void drawPoint(int screenX, int screenY, int size = 2) {
        pixel->drawSpriteRect(screenX, screenY, size, size);
    }

    
    void updateFontSize() {
        int fontSize = static_cast<int>(20.0f * (static_cast<float>(h) / 480.0f));
        fontSize = std::max(14, std::min(128, fontSize));
        if (fontSize != lastFontSize) {
            lastFontSize = fontSize;
            setFont("font.ttf", fontSize);
            clearTextQueue();
        }
    }

    
    void initGame() {
        initShip();
        initProjectiles();
        initAsteroids();
        initStars();
        state = STATE_COUNTDOWN;
        countdownTimer = 0;
        countdownNumber = 3;
        wasExploding = false;
    }

    void restartGame() {
        initGame();
    }

    void initStars() {
        int starIndex = 0;
        
        for (int i = 0; i < STAR_COUNT; ++i, ++starIndex) {
            stars[starIndex].x = static_cast<float>(rand() % GAME_W);
            stars[starIndex].y = static_cast<float>(rand() % GAME_H);
            stars[starIndex].speed = 0.15f + (rand() % 40) / 100.0f;
            stars[starIndex].size = 0.8f + (rand() % 12) / 20.0f;
            stars[starIndex].brightness = 0.25f + (rand() % 45) / 100.0f;
            stars[starIndex].layer = 0;
            assignStarColor(stars[starIndex]);
            stars[starIndex].twinklePhase = (rand() % 628) / 100.0f;
            stars[starIndex].twinkleSpeed = 0.4f + (rand() % 150) / 100.0f;
        }
        
        for (int i = 0; i < STAR_COUNT; ++i, ++starIndex) {
            stars[starIndex].x = static_cast<float>(rand() % GAME_W);
            stars[starIndex].y = static_cast<float>(rand() % GAME_H);
            stars[starIndex].speed = 0.4f + (rand() % 80) / 100.0f;
            stars[starIndex].size = 1.2f + (rand() % 18) / 20.0f;
            stars[starIndex].brightness = 0.45f + (rand() % 55) / 100.0f;
            stars[starIndex].layer = 1;
            assignStarColor(stars[starIndex]);
            stars[starIndex].twinklePhase = (rand() % 628) / 100.0f;
            stars[starIndex].twinkleSpeed = 0.8f + (rand() % 250) / 100.0f;
        }
        
        for (int i = 0; i < STAR_COUNT; ++i, ++starIndex) {
            stars[starIndex].x = static_cast<float>(rand() % GAME_W);
            stars[starIndex].y = static_cast<float>(rand() % GAME_H);
            stars[starIndex].speed = 0.8f + (rand() % 180) / 100.0f;
            stars[starIndex].size = 1.8f + (rand() % 25) / 20.0f;
            stars[starIndex].brightness = 0.65f + (rand() % 35) / 100.0f;
            stars[starIndex].layer = 2;
            assignStarColor(stars[starIndex]);
            stars[starIndex].twinklePhase = (rand() % 628) / 100.0f;
            stars[starIndex].twinkleSpeed = 1.2f + (rand() % 350) / 100.0f;
        }
        starsInit = true;
    }

    void assignStarColor(Star& s) {
        float roll = (rand() % 1000) / 1000.0f;
        if (roll < 0.35f) {
            
            s.r = 0.85f + (rand() % 15) / 100.0f;
            s.g = 0.90f + (rand() % 10) / 100.0f;
            s.b = 1.0f;
        } else if (roll < 0.55f) {
            
            s.r = 0.5f + (rand() % 20) / 100.0f;
            s.g = 0.75f + (rand() % 15) / 100.0f;
            s.b = 1.0f;
            s.size *= 1.15f;
            s.brightness *= 1.1f;
        } else if (roll < 0.70f) {
            
            s.r = 1.0f;
            s.g = 0.95f + (rand() % 5) / 100.0f;
            s.b = 0.6f + (rand() % 30) / 100.0f;
            s.size *= 1.05f;
        } else if (roll < 0.82f) {
            
            s.r = 1.0f;
            s.g = 0.6f + (rand() % 20) / 100.0f;
            s.b = 0.2f + (rand() % 20) / 100.0f;
            s.size *= 1.1f;
        } else if (roll < 0.90f) {
            
            s.r = 1.0f;
            s.g = 0.3f + (rand() % 20) / 100.0f;
            s.b = 0.2f + (rand() % 15) / 100.0f;
            s.size *= 1.08f;
        } else if (roll < 0.96f) {
            
            s.r = 1.0f; s.g = 1.0f; s.b = 1.0f;
            s.size *= 1.4f;
            s.brightness *= 1.4f;
        } else {
            
            s.r = 0.9f; s.g = 0.95f; s.b = 1.0f;
            s.size *= 1.6f;
            s.brightness *= 1.5f;
        }
    }

    void initShip() {
        ship.x = GAME_W / 2.0f;
        ship.y = GAME_H / 2.0f;
        ship.vx = 0; ship.vy = 0;
        ship.angle = 0;
        ship.lives = 3;
        ship.fire_cooldown = 0;
        ship.burst_count = 0;
        ship.exploding = false;
        ship.explosion_timer = 0;
        ship.score = 0;
        ship.continuous_fire_timer = 0;
        ship.overheated = false;
        ship.overheat_cooldown = 0;
        for (auto &p : particles) p.active = false;
    }

    void initProjectiles() {
        for (auto &p : projectiles) p.active = false;
    }

    void initAsteroids() {
        for (auto &a : asteroids) a.active = false;
        for (int i = 0; i < 4; ++i) {
            float x, y;
            do {
                x = static_cast<float>(rand() % GAME_W);
                y = static_cast<float>(rand() % GAME_H);
            } while (hypotf(x - ship.x, y - ship.y) < 100);
            float vx = ((rand() % 100) - 50) / 50.0f;
            float vy = ((rand() % 100) - 50) / 50.0f;
            float radius = 20.0f + (rand() % 20);
            spawnAsteroid(x, y, vx, vy, radius);
        }
    }

    
    void updateShip() {
        if (ship.exploding) return;
        if (keyLeft)  ship.angle -= 0.15f;
        if (keyRight) ship.angle += 0.15f;
        if (keyThrust) {
            float thrust = 0.2f;
            ship.vx += sinf(ship.angle) * thrust;
            ship.vy += -cosf(ship.angle) * thrust;
            float spd = sqrtf(ship.vx * ship.vx + ship.vy * ship.vy);
            float maxSpd = 4.0f;
            if (spd > maxSpd) {
                ship.vx = (ship.vx / spd) * maxSpd;
                ship.vy = (ship.vy / spd) * maxSpd;
            }
        }
        ship.vx *= 0.98f;
        ship.vy *= 0.98f;
        ship.x += ship.vx;
        ship.y += ship.vy;
        
        if (ship.x < 0) ship.x += GAME_W;
        if (ship.x >= GAME_W) ship.x -= GAME_W;
        if (ship.y < 0) ship.y += GAME_H;
        if (ship.y >= GAME_H) ship.y -= GAME_H;
        if (ship.fire_cooldown > 0) ship.fire_cooldown--;
    }

    void updateFireTimer(const Uint8 *keys) {
        if (keys[SDL_SCANCODE_SPACE] && !ship.overheated) {
            ship.continuous_fire_timer++;
            ship.overheat_cooldown = 0;
            if (ship.continuous_fire_timer >= 180) {
                ship.overheated = true;
                ship.overheat_cooldown = 0;
                ship.continuous_fire_timer = 0;
                ship.burst_count = 0;
            }
        } else if (keys[SDL_SCANCODE_SPACE] && ship.overheated) {
            ship.overheat_cooldown = 0;
        } else {
            if (ship.overheated) {
                ship.overheat_cooldown++;
                if (ship.overheat_cooldown >= 180) {
                    ship.overheated = false;
                    ship.overheat_cooldown = 0;
                    ship.continuous_fire_timer = 0;
                }
            } else {
                if (ship.continuous_fire_timer > 0) ship.continuous_fire_timer--;
            }
        }
    }

    
    bool canFire() {
        if (ship.overheated) return false;
        if (ship.fire_cooldown <= 0) {
            if (ship.burst_count < SHOTS_PER_BURST) {
                ship.fire_cooldown = FIRE_DELAY;
                ship.burst_count++;
                return true;
            } else {
                ship.fire_cooldown = FIRE_COOLDOWN;
                ship.burst_count = 0;
                return false;
            }
        }
        return false;
    }

    void fireProjectile(float x, float y, float angle) {
        for (auto &p : projectiles) {
            if (!p.active) {
                float c = cosf(angle), s = sinf(angle);
                float lx = 0, ly = -12;
                p.x = x + (lx * c - ly * s);
                p.y = y + (lx * s + ly * c);
                p.vx = sinf(angle) * PROJECTILE_SPEED;
                p.vy = -cosf(angle) * PROJECTILE_SPEED;
                p.lifetime = PROJECTILE_LIFETIME;
                p.active = true;
                break;
            }
        }
    }

    void updateProjectiles() {
        for (auto &p : projectiles) {
            if (!p.active) continue;
            p.x += p.vx;
            p.y += p.vy;
            if (p.x < 0 || p.x >= GAME_W || p.y < 0 || p.y >= GAME_H) {
                p.active = false;
                continue;
            }
            p.lifetime--;
            if (p.lifetime <= 0) p.active = false;
        }
    }

    
    void spawnAsteroid(float x, float y, float vx, float vy, float radius) {
        for (auto &a : asteroids) {
            if (!a.active) {
                a.x = x; a.y = y; a.vx = vx; a.vy = vy;
                a.radius = radius; a.active = true;
                a.rotation_angle = 0;
                a.rotation_speed = ((rand() % 100) - 50) / 1000.0f;
                for (int j = 0; j < ASTEROID_VERTICES; ++j) {
                    float ang = static_cast<float>(j) / ASTEROID_VERTICES * 2 * M_PI;
                    float vr = radius * (0.7f + (rand() % 30) / 100.0f);
                    a.vertices[j][0] = cosf(ang) * vr;
                    a.vertices[j][1] = sinf(ang) * vr;
                }
                break;
            }
        }
    }

    void updateAsteroids() {
        for (auto &a : asteroids) {
            if (!a.active) continue;
            a.x += a.vx;
            a.y += a.vy;
            a.rotation_angle += a.rotation_speed;
            if (a.x < 0) a.x += GAME_W;
            if (a.x >= GAME_W) a.x -= GAME_W;
            if (a.y < 0) a.y += GAME_H;
            if (a.y >= GAME_H) a.y -= GAME_H;
        }
    }

    void splitAsteroid(int idx) {
        float x = asteroids[idx].x;
        float y = asteroids[idx].y;
        float r = asteroids[idx].radius / 2.0f;
        if (r < MIN_ASTEROID_RADIUS) {
            createAsteroidExplosion(x, y);
            ship.score += SMALL_ASTEROID_POINTS;
            asteroids[idx].active = false;
            return;
        }
        int splits = 2 + (rand() % 2);
        for (int i = 0; i < splits; ++i) {
            float ang = (rand() % 628) / 100.0f;
            float spd = 0.5f + (rand() % 15) / 10.0f;
            float nvx = cosf(ang) * spd + asteroids[idx].vx * 0.3f;
            float nvy = sinf(ang) * spd + asteroids[idx].vy * 0.3f;
            float off = r * 0.5f;
            spawnAsteroid(x + cosf(ang) * off, y + sinf(ang) * off, nvx, nvy, r);
        }
        if (r >= 25.0f) ship.score += LARGE_ASTEROID_POINTS;
        else ship.score += MEDIUM_ASTEROID_POINTS;
        createAsteroidExplosion(x, y);
        asteroids[idx].active = false;
    }

    void checkAndSpawnAsteroids() {
        int active = 0;
        for (auto &a : asteroids) if (a.active) active++;
        int minAsteroids = 3;
        if (active < minAsteroids) {
            for (int s = 0; s < minAsteroids - active; ++s) {
                float x, y;
                int attempts = 0;
                do {
                    if (rand() % 2) {
                        x = (rand() % 2) ? -30.0f : GAME_W + 30.0f;
                        y = static_cast<float>(rand() % GAME_H);
                    } else {
                        x = static_cast<float>(rand() % GAME_W);
                        y = (rand() % 2) ? -30.0f : GAME_H + 30.0f;
                    }
                    attempts++;
                } while (hypotf(x - ship.x, y - ship.y) < 100.0f && attempts < 10);
                float vx = ((rand() % 100) - 50) / 50.0f;
                float vy = ((rand() % 100) - 50) / 50.0f;
                float radius;
                int roll = rand() % 100;
                if (roll < 20) radius = 30.0f + (rand() % 15);
                else if (roll < 60) radius = 20.0f + (rand() % 10);
                else radius = 12.0f + (rand() % 8);
                spawnAsteroid(x, y, vx, vy, radius);
            }
        }
    }

    
    void checkAsteroidCollisions() {
        for (int i = 0; i < MAX_ASTEROIDS; ++i) {
            if (!asteroids[i].active) continue;
            for (int j = i + 1; j < MAX_ASTEROIDS; ++j) {
                if (!asteroids[j].active) continue;
                float dx = asteroids[i].x - asteroids[j].x;
                float dy = asteroids[i].y - asteroids[j].y;
                float dist = sqrtf(dx * dx + dy * dy);
                float minDist = asteroids[i].radius + asteroids[j].radius;
                if (dist < minDist && dist > 0) {
                    float nx = dx / dist, ny = dy / dist;
                    float overlap = (minDist - dist) * 0.5f;
                    asteroids[i].x += nx * overlap;
                    asteroids[i].y += ny * overlap;
                    asteroids[j].x -= nx * overlap;
                    asteroids[j].y -= ny * overlap;
                    float rvx = asteroids[i].vx - asteroids[j].vx;
                    float rvy = asteroids[i].vy - asteroids[j].vy;
                    float van = rvx * nx + rvy * ny;
                    if (van > 0) continue;
                    float restitution = 0.8f;
                    float impulse = -(1 + restitution) * van * 0.5f;
                    asteroids[i].vx += impulse * nx;
                    asteroids[i].vy += impulse * ny;
                    asteroids[j].vx -= impulse * nx;
                    asteroids[j].vy -= impulse * ny;
                }
            }
        }
    }

    void checkShipAsteroidCollision() {
        if (ship.exploding) return;
        for (int i = 0; i < MAX_ASTEROIDS; ++i) {
            if (!asteroids[i].active) continue;
            float dx = ship.x - asteroids[i].x;
            float dy = ship.y - asteroids[i].y;
            float dist = sqrtf(dx * dx + dy * dy);
            float shipRadius = 8.0f;
            if (dist < asteroids[i].radius + shipRadius) {
                startShipExplosion();
                createAsteroidExplosion(ship.x + dx * 0.5f, ship.y + dy * 0.5f);
                if (dist > 0) {
                    float nx = -dx / dist, ny = -dy / dist;
                    asteroids[i].vx += nx * 2.0f;
                    asteroids[i].vy += ny * 2.0f;
                }
                break;
            }
        }
    }

    void checkProjectileAsteroidCollisions() {
        for (auto &p : projectiles) {
            if (!p.active) continue;
            for (int j = 0; j < MAX_ASTEROIDS; ++j) {
                if (!asteroids[j].active) continue;
                float dx = p.x - asteroids[j].x;
                float dy = p.y - asteroids[j].y;
                if (sqrtf(dx * dx + dy * dy) < asteroids[j].radius) {
                    p.active = false;
                    if (asteroids[j].radius >= 20.0f)
                        ship.score += LARGE_ASTEROID_POINTS;
                    else if (asteroids[j].radius >= MIN_ASTEROID_RADIUS)
                        ship.score += MEDIUM_ASTEROID_POINTS;
                    else
                        ship.score += SMALL_ASTEROID_POINTS;
                    splitAsteroid(j);
                    break;
                }
            }
        }
    }

    
    void startShipExplosion() {
        if (ship.exploding) return;
        ship.exploding = true;
        ship.explosion_timer = EXPLOSION_DURATION;
        ship.lives--;
        ship.overheated = false;
        ship.overheat_cooldown = 0;
        ship.continuous_fire_timer = 0;
        ship.burst_count = 0;
        for (auto &p : particles) {
            p.x = ship.x + ((rand() % 20) - 10);
            p.y = ship.y + ((rand() % 20) - 10);
            float ang = (rand() % 628) / 100.0f;
            float spd = 0.5f + (rand() % 20) / 10.0f;
            p.vx = cosf(ang) * spd;
            p.vy = sinf(ang) * spd;
            p.lifetime = 30 + (rand() % 60);
            p.active = true;
        }
    }

    void createAsteroidExplosion(float x, float y) {
        int toCreate = 15 + (rand() % 10);
        int created = 0;
        for (auto &p : particles) {
            if (created >= toCreate) break;
            if (!p.active) {
                p.x = x + ((rand() % 16) - 8);
                p.y = y + ((rand() % 16) - 8);
                float ang = (rand() % 628) / 100.0f;
                float spd = 1.0f + (rand() % 20) / 10.0f;
                p.vx = cosf(ang) * spd;
                p.vy = sinf(ang) * spd;
                p.lifetime = 15 + (rand() % 25);
                p.active = true;
                created++;
            }
        }
    }

    void updateExplosion() {
        for (auto &p : particles) {
            if (!p.active) continue;
            p.x += p.vx;
            p.y += p.vy;
            p.vx *= 0.97f;
            p.vy *= 0.97f;
            p.lifetime--;
            if (p.lifetime <= 0) { p.active = false; continue; }
            if (p.x < 0) p.x += GAME_W;
            if (p.x >= GAME_W) p.x -= GAME_W;
            if (p.y < 0) p.y += GAME_H;
            if (p.y >= GAME_H) p.y -= GAME_H;
        }
        if (ship.exploding) {
            ship.explosion_timer--;
            if (ship.explosion_timer <= 0) ship.exploding = false;
        }
    }

    void handleDeathSequence() {
        if (wasExploding && !ship.exploding) {
            if (ship.lives > 0) {
                triggerRespawn();
            } else {
                state = STATE_GAMEOVER;
            }
            wasExploding = false;
        } else if (ship.exploding) {
            wasExploding = true;
        }
    }

    void triggerRespawn() {
        ship.x = GAME_W / 2.0f;
        ship.y = GAME_H / 2.0f;
        ship.vx = 0; ship.vy = 0;
        ship.angle = 0;
        ship.exploding = false;
        ship.explosion_timer = 0;
        keyLeft = keyRight = keyThrust = false;
        state = STATE_COUNTDOWN;
        countdownTimer = 0;
        countdownNumber = 3;
    }

    
    void updateCountdown() {
        countdownTimer++;
        if (countdownTimer >= countdownDuration) {
            countdownTimer = 0;
            countdownNumber--;
            if (countdownNumber < 0) {
                state = STATE_LAUNCH;
                launchTimer = 0;
                shipLaunchY = static_cast<float>(GAME_H);
            }
        }
    }

    void updateLaunch() {
        launchTimer++;
        if (launchTimer < launchDuration / 2) {
            shipLaunchY = GAME_H - (launchTimer * (GAME_H - ship.y) / (launchDuration / 2));
        } else {
            shipLaunchY = ship.y;
        }
        if (launchTimer >= launchDuration) {
            state = STATE_PLAYING;
        }
    }

    void drawStars(float speedMul = 1.0f) {
        
        for (auto &s : stars) {
            s.y += s.speed * speedMul;
            if (s.y >= GAME_H) {
                s.x = static_cast<float>(rand() % GAME_W);
                s.y = 0;
                
                if (s.layer == 0) {
                    s.speed = 0.15f + (rand() % 40) / 100.0f;
                    s.size = 0.8f + (rand() % 12) / 20.0f;
                    s.brightness = 0.25f + (rand() % 45) / 100.0f;
                } else if (s.layer == 1) {
                    s.speed = 0.4f + (rand() % 80) / 100.0f;
                    s.size = 1.2f + (rand() % 18) / 20.0f;
                    s.brightness = 0.45f + (rand() % 55) / 100.0f;
                } else {
                    s.speed = 0.8f + (rand() % 180) / 100.0f;
                    s.size = 1.8f + (rand() % 25) / 20.0f;
                    s.brightness = 0.65f + (rand() % 35) / 100.0f;
                }
                assignStarColor(s);
                s.twinklePhase = (rand() % 628) / 100.0f;
            }
            
            
            s.twinklePhase += s.twinkleSpeed * 0.016f; 
            float twinkle = 0.7f + 0.3f * sinf(s.twinklePhase);
            float finalBrightness = s.brightness * twinkle;
            
            
            starSprite->setShaderParams(
                s.r * finalBrightness,
                s.g * finalBrightness,
                s.b * finalBrightness,
                finalBrightness
            );
            
            int screenSize = static_cast<int>(s.size * std::min(sx(), sy()));
            screenSize = std::max(2, screenSize);
            starSprite->drawSpriteRect(toScreenX(s.x), toScreenY(s.y), screenSize, screenSize);
        }
    }

    void drawShipAt(float px, float py) {
        if (ship.exploding) {
            drawExplosion();
            return;
        }
        float c = cosf(ship.angle), s = sinf(ship.angle);
        auto rot = [&](float lx, float ly, float &ox, float &oy) {
            ox = px + (lx * c - ly * s);
            oy = py + (lx * s + ly * c);
        };
        
        float nose_x, nose_y, lw_x, lw_y, rw_x, rw_y;
        float le_x, le_y, re_x, re_y, ce_x, ce_y;
        float ld_x, ld_y, rd_x, rd_y;
        float cl_x, cl_y, cr_x, cr_y, cc_x, cc_y;

        rot(0, -15, nose_x, nose_y);
        rot(-12, 8, lw_x, lw_y);
        rot(12, 8, rw_x, rw_y);
        rot(-8, 12, le_x, le_y);
        rot(8, 12, re_x, re_y);
        rot(0, 10, ce_x, ce_y);
        rot(-6, 2, ld_x, ld_y);
        rot(6, 2, rd_x, rd_y);
        rot(-3, -8, cl_x, cl_y);
        rot(3, -8, cr_x, cr_y);
        rot(0, -5, cc_x, cc_y);

        
        if (ship.overheated) {
            setColor(1.0f, 0.0f, 0.0f, 1.0f);
        } else if (ship.continuous_fire_timer > 120) {
            int heat = ship.continuous_fire_timer - 120;
            setColor(1.0f, std::max(0.0f, 1.0f - heat / 64.0f), 0.0f, 1.0f);
        } else {
            setColor(1.0f, 1.0f, 1.0f, 1.0f);
        }
        
        drawLine(nose_x, nose_y, lw_x, lw_y);
        drawLine(nose_x, nose_y, rw_x, rw_y);
        drawLine(lw_x, lw_y, rw_x, rw_y);
        
        drawLine(lw_x, lw_y, le_x, le_y);
        drawLine(rw_x, rw_y, re_x, re_y);
        drawLine(le_x, le_y, ce_x, ce_y);
        drawLine(re_x, re_y, ce_x, ce_y);

        
        if (ship.overheated)
            setColor(0.78f, 0.0f, 0.0f, 1.0f);
        else if (ship.continuous_fire_timer > 120)
            setColor(0.78f, std::max(0.0f, 0.78f - (ship.continuous_fire_timer - 120) / 80.0f), 0.0f, 1.0f);
        else
            setColor(0.78f, 0.78f, 0.78f, 1.0f);
        drawLine(nose_x, nose_y, ld_x, ld_y);
        drawLine(nose_x, nose_y, rd_x, rd_y);
        drawLine(ld_x, ld_y, rd_x, rd_y);
        drawLine(ld_x, ld_y, lw_x, lw_y);
        drawLine(rd_x, rd_y, rw_x, rw_y);

        
        if (ship.overheated)
            setColor(0.0f, 0.5f, 0.5f, 1.0f);
        else
            setColor(0.0f, 1.0f, 1.0f, 1.0f);
        drawLine(cl_x, cl_y, cr_x, cr_y);
        drawLine(cl_x, cl_y, cc_x, cc_y);
        drawLine(cr_x, cr_y, cc_x, cc_y);

        
        setColor(0.59f, 0.59f, 0.59f, 1.0f);
        drawLine(cc_x, cc_y, ce_x, ce_y);

        
        if (keyThrust) {
            float f1x, f1y, f2x, f2y, f3x, f3y;
            rot(-6, 18, f1x, f1y);
            rot(0, 22, f2x, f2y);
            rot(6, 18, f3x, f3y);
            setColor(1.0f, 0.39f, 0.0f, 1.0f);
            drawLine(le_x, le_y, f1x, f1y);
            drawLine(ce_x, ce_y, f2x, f2y);
            drawLine(re_x, re_y, f3x, f3y);
            float ifx, ify;
            rot(0, 20, ifx, ify);
            setColor(1.0f, 1.0f, 0.0f, 1.0f);
            drawLine(ce_x, ce_y, ifx, ify);
        }
    }

    void drawProjectiles() {
        setColor(1.0f, 0.0f, 0.0f, 1.0f);
        for (auto &p : projectiles) {
            if (!p.active) continue;
            float endX = p.x - p.vx * 0.5f;
            float endY = p.y - p.vy * 0.5f;
            drawLine(p.x, p.y, endX, endY);
        }
    }

    void drawAsteroids() {
        for (int i = 0; i < MAX_ASTEROIDS; ++i) {
            if (!asteroids[i].active) continue;
            auto &a = asteroids[i];
            float cosR = cosf(a.rotation_angle);
            float sinR = sinf(a.rotation_angle);

            
            float rv[ASTEROID_VERTICES][2];
            for (int j = 0; j < ASTEROID_VERTICES; ++j) {
                rv[j][0] = a.vertices[j][0] * cosR - a.vertices[j][1] * sinR;
                rv[j][1] = a.vertices[j][0] * sinR + a.vertices[j][1] * cosR;
            }
            
            setColor(1.0f, 1.0f, 1.0f, 1.0f);
            for (int j = 0; j < ASTEROID_VERTICES; ++j) {
                int next = (j + 1) % ASTEROID_VERTICES;
                drawLine(a.x + rv[j][0], a.y + rv[j][1],
                         a.x + rv[next][0], a.y + rv[next][1]);
            }
            
            float iv[ASTEROID_VERTICES][2];
            for (int j = 0; j < ASTEROID_VERTICES; ++j) {
                iv[j][0] = rv[j][0] * 0.6f;
                iv[j][1] = rv[j][1] * 0.6f;
            }
            setColor(0.59f, 0.59f, 0.59f, 1.0f);
            for (int j = 0; j < ASTEROID_VERTICES; ++j) {
                int next = (j + 1) % ASTEROID_VERTICES;
                drawLine(a.x + iv[j][0], a.y + iv[j][1],
                         a.x + iv[next][0], a.y + iv[next][1]);
            }
            
            setColor(0.39f, 0.39f, 0.39f, 1.0f);
            for (int j = 0; j < ASTEROID_VERTICES; j += 2) {
                drawLine(a.x + rv[j][0], a.y + rv[j][1],
                         a.x + iv[j][0], a.y + iv[j][1]);
            }
            
            setColor(0.71f, 0.71f, 0.71f, 1.0f);
            for (int j = 0; j < ASTEROID_VERTICES; j += 2) {
                int next = (j + 1) % ASTEROID_VERTICES;
                drawLine(a.x + rv[j][0], a.y + rv[j][1],
                         a.x + iv[next][0], a.y + iv[next][1]);
            }
        }
    }

    void drawExplosion() {
        for (auto &p : particles) {
            if (!p.active) continue;
            float lp = static_cast<float>(p.lifetime) / 50.0f;
            if (lp > 0.7f)
                setColor(1.0f, 1.0f, 1.0f, 1.0f);
            else if (lp > 0.5f)
                setColor(1.0f, 1.0f, 0.0f, 1.0f);
            else if (lp > 0.2f)
                setColor(1.0f, 0.5f, 0.0f, 1.0f);
            else
                setColor(1.0f, 0.0f, 0.0f, 1.0f);
            drawRect(p.x, p.y, 2, 2);
        }
    }

    void drawHUD() {
        std::string buf = std::format("Score: {}", ship.score);
        printText(buf.c_str(), toScreenX(10), toScreenY(10), {255, 255, 255, 255});
        buf = std::format("Lives: {}", ship.lives);
        printText(buf.c_str(), toScreenX(10), toScreenY(30), {255, 255, 255, 255});
    }

    
    void drawCountdown() {
        drawStars(0.3f);
        if (countdownNumber > 0) {
            std::string buf = std::format("{}", countdownNumber);
            
            int textW, textH;
            if (getTextDimensions(buf.c_str(), textW, textH)) {
                
                int centerX = w / 2;
                int centerY = h / 2;
                int textX = centerX - textW / 2;
                int textY = centerY - textH / 2;
                printText(buf.c_str(), textX, textY, {255, 255, 0, 255});
                
                
                if ((countdownTimer / 10) % 2) {
                    setColor(1.0f, 1.0f, 0.0f, 0.5f);
                    int padding = std::max(15, static_cast<int>(15 * std::min(sx(), sy())));
                    int rectX = textX - padding;
                    int rectY = textY - padding;
                    int rectW = textW + padding * 2;
                    int rectH = textH + padding * 2;
                    int thickness = std::max(3, static_cast<int>(3 * std::min(sx(), sy())));
                    
                    
                    pixel->drawSpriteRect(rectX, rectY, rectW, thickness);
                    
                    pixel->drawSpriteRect(rectX, rectY + rectH - thickness, rectW, thickness);
                    
                    pixel->drawSpriteRect(rectX, rectY, thickness, rectH);
                    
                    pixel->drawSpriteRect(rectX + rectW - thickness, rectY, thickness, rectH);
                }
            }
        } else {
            
            int textW, textH;
            if (getTextDimensions("LAUNCH!", textW, textH)) {
                printText("LAUNCH!", w/2 - textW/2, h/2 - textH/2, {0, 255, 0, 255});
            } else {
                printText("LAUNCH!", toScreenX(270), toScreenY(160), {0, 255, 0, 255});
            }
        }
        
        
        int textW, textH;
        if (getTextDimensions("PREPARE FOR MISSION", textW, textH)) {
            printText("PREPARE FOR MISSION", w/2 - textW/2, h/2 + textH * 2, {255, 255, 255, 255});
        } else {
            printText("PREPARE FOR MISSION", toScreenX(220), toScreenY(195), {255, 255, 255, 255});
        }

        std::string lbuf = std::format("Lives: {}", ship.lives);
        std::string sbuf = std::format("Score: {}", ship.score);
        if (getTextDimensions(lbuf.c_str(), textW, textH)) {
            printText(lbuf.c_str(), w/2 - textW/2, h/2 + textH * 3 + 10, {255, 255, 255, 255});
            if (getTextDimensions(sbuf.c_str(), textW, textH)) {
                printText(sbuf.c_str(), w/2 - textW/2, h/2 + textH * 4 + 15, {255, 255, 255, 255});
            }
        } else {
            printText(lbuf.c_str(), toScreenX(260), toScreenY(215), {255, 255, 255, 255});
            printText(sbuf.c_str(), toScreenX(260), toScreenY(235), {255, 255, 255, 255});
        }
    }

    void drawLaunch() {
        drawStars(0.5f);
        if (launchTimer > launchDuration / 4)
            drawAsteroids();
        
        float tempY = ship.y;
        ship.y = shipLaunchY;
        drawShipAt(ship.x, shipLaunchY);
        ship.y = tempY;
        
        if (launchTimer < launchDuration / 2) {
            setColor(1.0f, 0.39f, 0.0f, 1.0f);
            for (int i = 0; i < 12; ++i) {
                float tx = ship.x + (rand() % 8) - 4;
                float ty = shipLaunchY + 15 + i * 2;
                if (ty < GAME_H) drawPixel(tx, ty);
            }
        }
        if (launchTimer >= launchDuration / 2) {
            int textW, textH;
            if (getTextDimensions("MISSION START!", textW, textH)) {
                printText("MISSION START!", w/2 - textW/2, h/2 - textH/2, {0, 255, 255, 255});
            } else {
                printText("MISSION START!", toScreenX(240), toScreenY(170), {0, 255, 255, 255});
            }
        }
    }

    void drawGame() {
        drawStars(1.0f);
        drawShipAt(ship.x, ship.y);
        drawProjectiles();
        drawAsteroids();
        drawExplosion();
        drawHUD();
    }

    void drawGameOver() {
        drawStars(0.2f);
        printText("GAME OVER", toScreenX(255), toScreenY(130), {255, 0, 0, 255});

        std::string buf = std::format("Final Score: {}", ship.score);
        printText(buf.c_str(), toScreenX(235), toScreenY(165), {255, 255, 255, 255});
        printText("Press SPACE to begin", toScreenX(215), toScreenY(195), {255, 255, 0, 255});
    }
};




int main(int argc, char **argv) {
    Arguments args = proc_args(argc, argv);
    try {
        SpaceRoxWindow window(args.path, args.width, args.height, args.fullscreen);
        window.initVulkan();
        window.loop();
        window.cleanup();
    } catch (mx::Exception &e) {
        SDL_Log("mx: Exception: %s\n", e.text().c_str());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
