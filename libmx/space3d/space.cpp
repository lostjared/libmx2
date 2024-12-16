#include"mx.hpp"
#include"argz.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <GLES3/gl3.h>
#endif

#include<ctime>
#include<cstdlib>
#include<tuple>

#include"gl.hpp"
#include"loadpng.hpp"
#include"model.hpp"
#include<random>

#define CHECK_GL_ERROR() \
{ GLenum err = glGetError(); \
if (err != GL_NO_ERROR) \
printf("OpenGL Error: %d at %s:%d\n", err, __FILE__, __LINE__); }

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

float getRandomFloat(float min, float max) {
    static std::random_device rd;  
    static std::mt19937 gen(rd()); 
    std::uniform_real_distribution<float> dis(min, max);
    return dis(gen);
}

bool isColliding(const glm::vec3 &posA, float radiusA, const glm::vec3 &posB, float radiusB) {
    float dx = posA.x - posB.x;
    float dy = posA.y - posB.y;
    float dz = posA.z - posB.z;
    float distSq = dx * dx + dy * dy + dz * dz;
    float radiusSum = radiusA + radiusB;
    float radiusSumSq = radiusSum * radiusSum;
    return distSq <= radiusSumSq;
}

enum class EnemyType { SHIP=1, SAUCER, TRIANGLE };

class Enemy {
public:
    mx::Model *object = nullptr;
    gl::ShaderProgram *shader = nullptr;
    glm::vec3 object_pos;
    bool ready = false;
    float initial_x = 0.0f;
    int verticalDirection = 1;
    bool isSpinning = false;
    float spinAngle = 0.0f;
    float spinSpeed = 360.0f; 
    float spinDuration = 0.6f;
    float elapsedSpinTime = 0.0f;
    GLuint fire = 0;
    Enemy(mx::Model *m, EnemyType type, gl::ShaderProgram *shaderv) : object{m}, shader{shaderv}, etype{type} {}
    virtual ~Enemy() = default;
    Enemy(const Enemy &e) = delete;
    Enemy &operator=(const Enemy &e) = delete;
    Enemy(Enemy &&e) : object{e.object}, shader{e.shader}, object_pos{e.object_pos}, etype{e.etype} {
        e.object = nullptr;
        e.shader = nullptr;
    }
    Enemy &operator=(Enemy &&e) {
        if(this != &e) {
            object = e.object;
            object_pos = e.object_pos;
            etype = e.etype;
            e.object = nullptr;
            e.shader = nullptr;
        }
        return *this;
    }
    void load(gl::GLWindow *win, mx::Model *m) {
        object = m;
    }
    void setEnemyType(EnemyType e) {
        etype = e;
    }
    void draw() {
        if(!isSpinning)
             object->drawArrays();
        else {
            object->drawArraysWithTexture(fire, "texture1");
        }
    }

    virtual void update(float deltaTime) = 0;
    virtual void move(float deltatime, float speed) = 0;
    virtual void reverseDirection(float screenTop, float screenBottom) {}
    EnemyType getType() const { return etype; }

    void updateSpin(float deltaTime) {
        if (isSpinning) {
            elapsedSpinTime += deltaTime;
            spinAngle += spinSpeed * deltaTime;

            if (elapsedSpinTime >= spinDuration) {
                isSpinning = false; 
                ready = true;
            }
        }
    }

protected:
    EnemyType etype;    
};

class EnemyShip : public Enemy {
public:

    EnemyShip(mx::Model *m, gl::ShaderProgram *shadev) : Enemy(m, EnemyType::SHIP, shadev) {}
    virtual void update(float deltatime) override {

    }
    
    virtual void move(float deltaTime, float speed) override {
        angleOffset += 0.05f;
        object_pos.x = initial_x + 5.0f * cos(angleOffset);
        object_pos.y += verticalDirection * 0.25f;
    }
    virtual void reverseDirection(float screenTop, float screenBottom) override {
        if (object_pos.y > screenBottom) {
            verticalDirection = -1; 
        } else if (object_pos.y < screenTop) {
            verticalDirection = 1;  
        }
    }
protected:
    float angleOffset = 0.0f;
};

class EnemySaucer : public Enemy {
public:
    EnemySaucer(mx::Model *m, gl::ShaderProgram *shadev) : Enemy(m, EnemyType::SAUCER, shadev) {}
    virtual void update(float deltatime) override {
    
    }
    virtual void move(float deltaTime, float speed) override {
        object_pos.y -= speed * deltaTime;
    }
};

class EnemyTriangle : public Enemy {
public:
    EnemyTriangle(mx::Model *m, gl::ShaderProgram *shadev) : Enemy(m, EnemyType::TRIANGLE, shadev) {}
    virtual void update(float deltatime) override {
        
    }
    virtual void move(float deltaTime, float speed) override {
        object_pos.y += verticalDirection * speed * deltaTime;
    }

    virtual void reverseDirection(float screenTop, float screenBottom) override {
        if (object_pos.y > screenBottom) {
            verticalDirection = -1; 
        } else if (object_pos.y < screenTop) {
            verticalDirection = 1;  
        }
    }
};

class SpaceGame : public gl::GLObject {
public:
    GLuint fire = 0;
    int snd_fire = 0, snd_crash = 0, snd_takeoff = 0;
    bool launch_ship = true;
    gl::ShaderProgram shaderProgram, textShader, starsShader;
    mx::Font font;
    mx::Input stick;
    int score = 0, lives = 5;
    GLuint starsVBO, starsVAO;
    const int numStars = 1000;
    glm::vec3 stars[1000];
    mx::Model ship;
    mx::Model projectile;
    mx::Model enemy_ship, saucer, triangle;
    glm::vec3 ship_pos;
    std::vector<std::tuple<glm::vec3, glm::vec3>> projectiles;
    std::vector<std::unique_ptr<Enemy>> enemies;
    float shipRotation = 0.0f;        
    float rotationSpeed = 90.0f;      
    float maxTiltAngle = 10.0f;       
    std::tuple<float, float, float, float> screenx;
    float barrelRollAngle = 0.0f;      
    bool isBarrelRolling = false;      
    float barrelRollSpeed = 360.0f; 
    float projectileSpeed = 100.0f;       
    float projectileLifetime = 50.0f;     
    float projectileRotation = 0.0f;
    float enemyReleaseTimer = 0.0f;
    float enemyReleaseInterval = 1.5f; 
    float enemySpeed = 15.0f;
    float enemyLifetime = 50.0f;
    bool game_over = false;
    float initial_x = 0.0f;
    int verticalDirection = 1;
    bool isSpinning = false;
    float spinAngle = 0.0f;
    float spinSpeed = 360.0f; 
    float spinDuration = 0.6f;
    float elapsedSpinTime = 0.0f;
    bool ready = false;

    SpaceGame(gl::GLWindow *win) : score{0}, lives{5}, ship_pos(0.0f, -20.0f, -70.0f) {
           
    }
    
    virtual ~SpaceGame() {
        glDeleteBuffers(1, &starsVBO);
        glDeleteVertexArrays(1, &starsVAO);
        glDeleteTextures(1, &fire);
    }

    void load(gl::GLWindow *win) override {
        font.loadFont(win->util.getFilePath("data/font.ttf"), 24);
        if(!shaderProgram.loadProgram(win->util.getFilePath("data/tri.vert"), win->util.getFilePath("data/tri.frag"))) {
            throw mx::Exception("Could not load shader program tri");
        }        
        if(!textShader.loadProgram(win->util.getFilePath("data/text.vert"), win->util.getFilePath("data/text.frag"))) {
            throw mx::Exception("Could not load shader program text");
        }
        if(!starsShader.loadProgram(win->util.getFilePath("data/stars.vert"), win->util.getFilePath("data/stars.frag"))) {
            throw mx::Exception("Could not load shader program text");
        }
        if(!ship.openModel(win->util.getFilePath("data/objects/bird.mxmod"))) {
            throw mx::Exception("Could not open model bird.mxmod");
        }
        if(!projectile.openModel(win->util.getFilePath("data/objects/sphere.mxmod"))) {
            throw mx::Exception("Could not open sphere.mxmod");
        }

        if(!enemy_ship.openModel(win->util.getFilePath("data/objects/ship1.mxmod"))) {
            throw mx::Exception("Could not open ship1.mxmod");
        }
        if(!saucer.openModel(win->util.getFilePath("data/objects/ufox.mxmod"))) {
            throw mx::Exception("Could not open sphere.mxmod");
        }
        if(!triangle.openModel(win->util.getFilePath("data/objects/shipshooter.mxmod"))) {
            throw mx::Exception("Could not open sphere.mxmod");
        }

        snd_fire = win->mixer.loadWav(win->util.getFilePath("data/sound/shoot.wav"));
        snd_crash = win->mixer.loadWav(win->util.getFilePath("data/sound/crash.wav"));
        snd_takeoff = win->mixer.loadWav(win->util.getFilePath("data/sound/takeoff.wav"));

        shaderProgram.useProgram();
        float aspectRatio = (float)win->w / (float)win->h;
        float fov = glm::radians(45.0f); 
        float z = ship_pos.z; 
        float height = 2.0f * glm::tan(fov / 2.0f) * std::abs(z);
        float width = height * aspectRatio;
        std::get<0>(screenx) = -width / 2.0f;
        std::get<1>(screenx) =  width / 2.0f;
        std::get<2>(screenx) = -height / 2.0f;
        std::get<3>(screenx) = height / 2.0f;
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);
        shaderProgram.setUniform("projection", projection);
        glm::mat4 view = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 5.0f),  
            glm::vec3(0.0f, 0.0f, 0.0f),  
            glm::vec3(0.0f, 1.0f, 0.0f)   
        );
        shaderProgram.setUniform("view", view);   
        ship.setShaderProgram(&shaderProgram, "texture1");
        ship.setTextures(win, win->util.getFilePath("data/objects/bird.tex"), win->util.getFilePath("data/objects"));
        projectile.setShaderProgram(&shaderProgram, "texture1");
        projectile.setTextures(win, win->util.getFilePath("data/objects/proj.tex"), win->util.getFilePath("data/objects"));
        enemy_ship.setShaderProgram(&shaderProgram, "texture1");
        enemy_ship.setTextures(win, win->util.getFilePath("data/objects/textures.tex"), win->util.getFilePath("data/objects"));
        saucer.setShaderProgram(&shaderProgram, "texture1");
        saucer.setTextures(win, win->util.getFilePath("data/objects/metal.tex"), win->util.getFilePath("data/objects"));
        triangle.setShaderProgram(&shaderProgram, "texture1");
        triangle.setTextures(win, win->util.getFilePath("data/objects/metal_ship.tex"), win->util.getFilePath("data/objects"));
        fire = gl::loadTexture(win->util.getFilePath("data/objects/flametex.png"));
        starsShader.useProgram();
        starsShader.setUniform("starColor", glm::vec3(1.0f, 1.0f, 1.0f));
        glm::mat4 star_projection = glm::ortho(-100.0f, 100.0f, -75.0f, 75.0f, -100.0f, 100.0f);
        starsShader.setUniform("projection", star_projection);
        initStars();
        glGenVertexArrays(1, &starsVAO);
        glGenBuffers(1, &starsVBO);
        glBindVertexArray(starsVAO);
        glBindBuffer(GL_ARRAY_BUFFER, starsVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(stars), stars, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
#ifndef __EMSCRIPTEN__
        glEnable(GL_PROGRAM_POINT_SIZE);
#endif
    }
#ifndef __EMSCRIPTEN__
    Uint32 lastUpdateTime = SDL_GetTicks();
#else
    double lastUpdateTime = emscripten_get_now();
#endif
    void draw(gl::GLWindow *win) override {
        shaderProgram.useProgram();
        CHECK_GL_ERROR();
#ifndef __EMSCRIPTEN__
        Uint32 currentTime = SDL_GetTicks();
#else
        double currentTime = emscripten_get_now();
#endif
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; 
        if (deltaTime > 0.1f) 
            deltaTime = 0.1f;
        lastUpdateTime = currentTime;
        update(win, deltaTime);

      
        /// draw objects
        glDisable(GL_DEPTH_TEST);
        starsShader.useProgram();
        glBindVertexArray(starsVAO);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDrawArrays(GL_POINTS, 0, numStars);
        glBindVertexArray(0);

        glEnable(GL_DEPTH_TEST);
        shaderProgram.useProgram();
        glm::vec3 cameraPos(0.0f, 0.0f, 10.0f);
        glm::vec3 lightPos(0.0f, 3.0f, 2.0f); 
        glm::vec3 viewPos = cameraPos;
        glm::vec3 lightColor(1.5f, 1.5f, 1.5f); 

        shaderProgram.setUniform("lightPos", lightPos);
        shaderProgram.setUniform("viewPos", viewPos);
        shaderProgram.setUniform("lightColor", lightColor);
        glm::mat4 model = glm::translate(glm::mat4(1.0f), ship_pos);
        if (isBarrelRolling) {
            model = glm::rotate(model, glm::radians(barrelRollAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        }
        model = glm::rotate(model, glm::radians(shipRotation), glm::vec3(0.0f, 0.0f, 1.0f)); 
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));  
        model = glm::rotate(model, glm::radians(-180.0f), glm::vec3(0.0f, 0.0f, 1.0f));  
        if(isSpinning)
            model = glm::rotate(model, glm::radians(spinAngle), glm::vec3(1.0f, 1.0f, 1.0f));

        shaderProgram.setUniform("model", model);
        if(!isSpinning)
            ship.drawArrays();
        else
            ship.drawArraysWithTexture(fire, "texture1");

        if(!projectiles.empty()) {
            for (auto &pos : projectiles) {
                auto setPos = [&](glm::vec3 &posx) -> void {
                    glm::mat4 projModel = glm::translate(glm::mat4(1.0f), posx);
                    projModel = glm::rotate(projModel, glm::radians(-90.0f), glm::vec3(1.0f,0.0f,0.0f));
                    projModel = glm::rotate(projModel, glm::radians(-180.0f), glm::vec3(0.0f,0.0f,1.0f));
                    projModel = glm::rotate(projModel, glm::radians(projectileRotation), glm::vec3(1.0f,0.0f,1.0f));
                    projModel = glm::scale(projModel, glm::vec3(0.5, 0.5, 0.5));
                    shaderProgram.setUniform("model", projModel);
                    projectile.drawArrays();
                };
                setPos(std::get<0>(pos));
                setPos(std::get<1>(pos));
            }
        }

        if(!enemies.empty()) {
            for(auto &e : enemies) {
                glm::mat4 projModel = glm::translate(glm::mat4(1.0f), e->object_pos);
                switch(e->getType()) {
                    case EnemyType::SHIP:
                        projModel = glm::rotate(projModel, glm::radians(-90.0f), glm::vec3(1.0f,0.0f,0.0f));
                        projModel = glm::rotate(projModel, glm::radians(projectileRotation), glm::vec3(0.0f,1.0f,0.0f));
                        projModel = glm::scale(projModel, glm::vec3(1.2f, 1.2f, 1.2f));
                    break;
                    case EnemyType::SAUCER:
                        projModel = glm::rotate(projModel, glm::radians(projectileRotation), glm::vec3(0.0f, 0.0f, 1.0f));
                        projModel = glm::scale(projModel, glm::vec3(1.5f, 1.5f, 1.5f));
                    break;
                    case EnemyType::TRIANGLE:
                        if(e->verticalDirection == -1)
                            projModel = glm::rotate(projModel, glm::radians(69.0f), glm::vec3(1.0f,0.0f,0.0f));
                        else
                            projModel = glm::rotate(projModel, glm::radians(-69.0f), glm::vec3(1.0f,0.0f,0.0f));
                        projModel = glm::rotate(projModel, glm::radians(projectileRotation), glm::vec3(0.0f, 0.0f, 1.0f));
                    break;
                }    
                if(e->isSpinning)
                    projModel = glm::rotate(projModel, glm::radians(e->spinAngle), glm::vec3(1.0f, 0.0f, 0.0f));

                shaderProgram.setUniform("model", projModel);
                e->draw();
            }
        }

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        textShader.useProgram();
        SDL_Color white = {255, 255, 255, 255};
        int textWidth = 0, textHeight = 0;
        GLuint textTexture;
        if(game_over == true) {
            textTexture = createTextTexture("Double Tap or Press Button Start or Enter Key to Start New Game Your Score: " + std::to_string(score), font.wrapper().unwrap(), white, textWidth, textHeight);
        } else if(launch_ship == true) {
            textTexture = createTextTexture("Double Tap or Press X Button or Z Key to Launch Ship", font.wrapper().unwrap(), white, textWidth, textHeight);
        } else {
            textTexture = createTextTexture("Lives: " + std::to_string(lives) + " Score: " + std::to_string(score), font.wrapper().unwrap(), white, textWidth, textHeight);
        }
        renderText(textTexture, textWidth, textHeight, win->w, win->h);
        glDeleteTextures(1, &textTexture);
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        shaderProgram.useProgram();
    }

    float touch_start_x = 0.0f, touch_start_y = 0.0f;
    bool is_touch_active = false;
    Uint32 last_shot_time = 0;

    void event(gl::GLWindow *win, SDL_Event &e) override {

        if(stick.connectEvent(e)) {
            mx::system_out << "Controller Event\n";
            return;
        }


        switch(e.type) {
            case SDL_FINGERDOWN: {

                if(game_over == true) { 
                    game_over = false;
                    lives = 5;
                    score = 0;
                    launch_ship = true;
                    return;
                }

                if(launch_ship == true) {
                    launch_ship = false;
                    lives = 5;
                    score = 0;
                    win->mixer.playWav(snd_takeoff, 0, 0);
                    return;
                }
            
                touch_start_x = e.tfinger.x * win->w;
                touch_start_y = e.tfinger.y * win->h;
                is_touch_active = true;

#ifndef __EMSCRIPTEN__
                Uint32 ticks = SDL_GetTicks();
#else
                double ticks = emscripten_get_now();
#endif
                if (ticks - last_shot_time > 300) {
                    std::tuple<glm::vec3, glm::vec3> shots;
                    std::get<0>(shots) = ship_pos;
                    std::get<0>(shots).x -= 1.5f;
                    std::get<0>(shots).y += 1.0f;
                    std::get<1>(shots) = ship_pos;
                    std::get<1>(shots).x += 1.5f;
                    std::get<1>(shots).y += 1.0f;
                    projectiles.push_back(shots);
                    win->mixer.playWav(snd_fire, 0, 1);
                    last_shot_time = SDL_GetTicks();
                }
        }
        break;

        case SDL_FINGERMOTION:
            if (is_touch_active) {
                float touch_current_x = e.tfinger.x * win->w;
                float touch_current_y = e.tfinger.y * win->h;
                ship_pos.x += (touch_current_x - touch_start_x) / win->w * (std::get<1>(screenx) - std::get<0>(screenx));
                ship_pos.y -= (touch_current_y - touch_start_y) / win->h * (std::get<3>(screenx) - std::get<2>(screenx));
                touch_start_x = touch_current_x;
                touch_start_y = touch_current_y;
                ship_pos.x = glm::clamp(ship_pos.x, std::get<0>(screenx), std::get<1>(screenx));
                ship_pos.y = glm::clamp(ship_pos.y, std::get<2>(screenx) + 1.0f, std::get<3>(screenx) - 1.0f);
            }
            break;

        case SDL_FINGERUP:
            is_touch_active = false;
            break;
        }
    }
#ifndef __EMSCRIPTEN__
    Uint32 lastActionTime = SDL_GetTicks();
    Uint32 fireTime = SDL_GetTicks();
#else
    double lastActionTime = emscripten_get_now();
    double fireTime = emscripten_get_now();
#endif


    
    void checkInput(gl::GLWindow *win, float deltaTime) {

        if(game_over == true) {
            if(stick.getButton(mx::Input_Button::BTN_START)) {
                game_over = false;
                lives = 5;
                score = 0;
                launch_ship = true;
            }
            return;
        }

        if(launch_ship == true) {
            if(stick.getButton(mx::Input_Button::BTN_X)) {
                launch_ship = false;

                if(!win->mixer.isPlaying(0))
                    Mix_HaltChannel(0);

                win->mixer.playWav(snd_takeoff, 0, 0);
            }
            return;
        }
#ifndef __EMSCRIPTEN__
        Uint32 currentTime = SDL_GetTicks();
#else
        double currentTime = emscripten_get_now();
#endif
        if(currentTime - fireTime >= 175) {
            if(stick.getButton(mx::Input_Button::BTN_A)) {
                std::tuple<glm::vec3, glm::vec3> shots;
                std::get<0>(shots) = ship_pos;
                std::get<0>(shots).x -= 1.5f;
                std::get<0>(shots).y += 1.0f;
                std::get<1>(shots) = ship_pos;
                std::get<1>(shots).x += 1.5f;
                std::get<1>(shots).y += 1.0f;
                projectiles.push_back(shots);
                win->mixer.playWav(snd_fire, 0, 1);
                
            }
            fireTime = currentTime;
        }


        if (currentTime - lastActionTime >= 15) {
            float moveX = 0.0f; 
            float moveY = 0.0f; 

               if (!isBarrelRolling && stick.getButton(mx::Input_Button::BTN_B)) {
                    isBarrelRolling = true;
                    barrelRollAngle = 0.0f;
                }

            if (stick.getButton(mx::Input_Button::BTN_D_LEFT)) {
                moveX -= 50.0f; 
                shipRotation = glm::clamp(shipRotation + rotationSpeed * deltaTime, -maxTiltAngle, maxTiltAngle);
            }
            if (stick.getButton(mx::Input_Button::BTN_D_RIGHT)) {
                moveX += 50.0f; 
                shipRotation = glm::clamp(shipRotation - rotationSpeed * deltaTime, -maxTiltAngle, maxTiltAngle);
            }
            if (stick.getButton(mx::Input_Button::BTN_D_UP)) {
                moveY += 50.0f; 
            }
            if (stick.getButton(mx::Input_Button::BTN_D_DOWN)) {
                moveY -= 50.0f; 
            }
            if (moveX != 0.0f || moveY != 0.0f) {
                float magnitude = glm::sqrt(moveX * moveX + moveY * moveY);
                moveX = (moveX / magnitude) * 50.0f * deltaTime;
                moveY = (moveY / magnitude) * 50.0f * deltaTime;
            }
            ship_pos.x += moveX;
            ship_pos.y += moveY;
            if (moveX == 0.0f) {
                if (shipRotation > 0.0f) {
                    shipRotation = glm::max(shipRotation - rotationSpeed * deltaTime, 0.0f);
                } else if (shipRotation < 0.0f) {
                    shipRotation = glm::min(shipRotation + rotationSpeed * deltaTime, 0.0f);
                }
            }
            ship_pos.x = glm::clamp(ship_pos.x, std::get<0>(screenx), std::get<1>(screenx));
            ship_pos.y = glm::clamp(ship_pos.y, std::get<2>(screenx)+1.0f, std::get<3>(screenx) - 1.0f);
            lastActionTime = currentTime;
        }
    }


    void die() {
        enemies.clear();
        projectiles.clear();
        launch_ship = true;
        ship_pos = glm::vec3(0.0f, -20.0f, -70.0f);
        barrelRollAngle = 0.0f;
        isBarrelRolling = false;
        shipRotation = 0.0f;
        lives--;
        if(lives < 0) {
            game_over = true;
            launch_ship = false;
        }
        Mix_HaltChannel(0);
    }

    void update(gl::GLWindow *win, float deltaTime) {
        checkInput(win, deltaTime);
        updateSpin(deltaTime);

        if(ready == true) {
            die();
            ready = false;
            return;
        }

        for (int i = 0; i < numStars; ++i) {
            stars[i].y -= deltaTime * 10.0f; 
            if (stars[i].y < -75.0f) {
                stars[i] = glm::vec3(rand() % 200 - 100, 75.0f, 50.0f);
            }
        }
   
        glBindBuffer(GL_ARRAY_BUFFER, starsVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(stars), stars);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        CHECK_GL_ERROR();

        if(launch_ship == true || game_over == true) return;

         if (isBarrelRolling) {
           barrelRollAngle += barrelRollSpeed * deltaTime;
            if (barrelRollAngle >= 360.0f) {
                barrelRollAngle = 0.0f;
                isBarrelRolling = false;
            }
        }

        projectileRotation += 50.0f * deltaTime;
        if(projectileRotation >= 360.0f) {
            projectileRotation -= 360.0f;
        }

        if(!projectiles.empty()) {
            for (int i = (int)projectiles.size() - 1; i >= 0; i--) {
                std::get<0>(projectiles[i]).y += projectileSpeed * deltaTime;
                std::get<1>(projectiles[i]).y += projectileSpeed * deltaTime;
                if ((std::get<0>(projectiles[i]).y > (std::get<3>(screenx) + projectileLifetime)) || (std::get<1>(projectiles[i]).y > (std::get<3>(screenx) + projectileLifetime))) {
                    projectiles.erase(projectiles.begin() + i);
                }
            }
        }
        enemyReleaseTimer += deltaTime;
        if (enemyReleaseTimer >= enemyReleaseInterval) {
            releaseEnemy();             
            enemyReleaseTimer = 0.0f;   
        }
        if(!enemies.empty()) {
            for(int i = (int)enemies.size() -1; i >= 0; i--) {
                enemies[i]->move(deltaTime, enemySpeed);
                if (enemies[i]->getType() != EnemyType::SHIP && enemies[i]->getType() != EnemyType::TRIANGLE && enemies[i]->object_pos.y < (std::get<2>(screenx))) {
                    isSpinning = true;
                    if(!win->mixer.isPlaying(2)) {
                        Mix_HaltChannel(1);
                        win->mixer.playWav(snd_crash, 0, 2);
                    }
                    return;
                } else {
                    enemies[i]->reverseDirection(std::get<2>(screenx), std::get<3>(screenx));
                } 
            }
        }

        for (int i = (int)enemies.size() - 1; i >= 0; i--) {
            enemies[i]->updateSpin(deltaTime);
            if (enemies[i]->ready) {
                enemies.erase(enemies.begin() + i);
            }
        }

        if (!projectiles.empty() && !enemies.empty()) {
            float projectileRadius = 0.5f;  
            float shipRadius = 1.2f;
            float saucerRadius = 1.5f;
            float triangleRadius = 1.0f; 

            for (int pi = (int)projectiles.size() - 1; pi >= 0; pi--) {
                bool projectileDestroyed = false;
                for (int ei = (int)enemies.size() - 1; ei >= 0; ei--) {

                    if(enemies[ei]->isSpinning == true) break;

                    float enemyRadius = 1.0f; // default
                    switch (enemies[ei]->getType()) {
                        case EnemyType::SHIP:
                            enemyRadius = shipRadius;
                            break;
                        case EnemyType::SAUCER:
                            enemyRadius = saucerRadius;
                            break;
                        case EnemyType::TRIANGLE:
                            enemyRadius = triangleRadius;
                            break;
                    }
                    if (isColliding(std::get<0>(projectiles[pi]), projectileRadius, enemies[ei]->object_pos, enemyRadius) || isColliding(std::get<1>(projectiles[pi]), projectileRadius, enemies[ei]->object_pos, enemyRadius)) {
                        projectiles.erase(projectiles.begin() + pi);
                        score += 10; 
                        projectileDestroyed = true;
                        enemies[ei]->isSpinning = true;
                        if(!win->mixer.isPlaying(2)) {
                            Mix_HaltChannel(1);
                            win->mixer.playWav(snd_crash, 0, 2);
                        }
                        break; 
                    }
                }
                if (projectileDestroyed) {
                    continue;
                }
            }
        }

        float playerRadius = 2.0f; 
        if (!enemies.empty()) {
            for (auto &enemy : enemies) {
                float enemyRadius = 1.0f;
                switch (enemy->getType()) {
                    case EnemyType::SHIP:
                        enemyRadius = 1.2f;
                        break;
                    case EnemyType::SAUCER:
                        enemyRadius = 1.5f;
                        break;
                    case EnemyType::TRIANGLE:
                        enemyRadius = 1.0f;
                        break;
                }
                if (isSpinning == false && isColliding(ship_pos, playerRadius, enemy->object_pos, enemyRadius)) {
                    isSpinning = true;
                    return;
                }
            }
        }
    }
    
    GLuint createTextTexture(const std::string &text, TTF_Font *font, SDL_Color color, int &textWidth, int &textHeight) {
        SDL_Surface *surface = TTF_RenderText_Blended(font, text.c_str(), color);
        if (!surface) {
            mx::system_err << "Failed to create text surface: " << TTF_GetError() << std::endl;
            return 0;
        }
        
        surface = mx::Texture::flipSurface(surface);
        
        textWidth = surface->w;
        textHeight = surface->h;
        
        if (surface->format->format != SDL_PIXELFORMAT_RGBA32) {
            SDL_Surface *converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
            if (!converted) {
                mx::system_err << "Failed to convert surface format: " << SDL_GetError() << std::endl;
                SDL_FreeSurface(surface);
                return 0;
            }
            SDL_FreeSurface(surface);
            surface = converted;
        }
        
        GLuint texture;
        glGenTextures(1, &texture);
        CHECK_GL_ERROR();
        glBindTexture(GL_TEXTURE_2D, texture);
        CHECK_GL_ERROR();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        CHECK_GL_ERROR();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textWidth, textHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
        CHECK_GL_ERROR();
        SDL_FreeSurface(surface);
        return texture;
    }
    
    void renderText(GLuint texture, int textWidth, int textHeight, int screenWidth, int screenHeight) {
        float x = -0.5f * (float)textWidth / screenWidth * 2.0f;
        float y = 0.9f;
        float w = (float)textWidth / screenWidth * 2.0f;
        float h = (float)textHeight / screenHeight * 2.0f;
        
        GLfloat vertices[] = {
            x,     y,      0.0f, 0.0f, 1.0f,
            x + w, y,      0.0f, 1.0f, 1.0f,
            x,     y - h,  0.0f, 0.0f, 0.0f,
            x + w, y - h,  0.0f, 1.0f, 0.0f
        };
        
        GLuint VBO, VAO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        textShader.useProgram();
        textShader.setUniform("textTexture", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        
        glBindVertexArray(0);
        glDeleteBuffers(1, &VBO);
        glDeleteVertexArrays(1, &VAO);
    }

private:
    void initStars() {
        std::srand(std::time(0));
        for (int i = 0; i < numStars; ++i) {
            stars[i] = glm::vec3(rand() % 200 - 100, rand() % 150 - 75, rand() % 100 + 1.0f);
        }
    }

    void releaseEnemy() {
        int r = rand()%3;
        switch(r) {
            case 0:
                enemies.push_back(std::make_unique<EnemyShip>(&enemy_ship, &shaderProgram));
            break;
            case 1:
                enemies.push_back(std::make_unique<EnemySaucer>(&saucer, &shaderProgram));
            break;
            case 2:
                enemies.push_back(std::make_unique<EnemyTriangle>(&triangle, &shaderProgram));
            break;
        }
        enemies.back()->fire = fire;
        enemies.back()->object_pos = glm::vec3(getRandomFloat(std::get<0>(screenx)+6.0f, std::get<1>(screenx))-6.0f, std::get<3>(screenx)-1.0f, -70.0f);
        enemies.back()->initial_x = enemies.back()->object_pos.x;
    }

    void updateSpin(float deltaTime) {
        if (isSpinning) {
            elapsedSpinTime += deltaTime;
            spinAngle += spinSpeed * deltaTime;

            if (elapsedSpinTime >= spinDuration) {
                isSpinning = false; 
                ready = true;
                elapsedSpinTime = 0.0f;
            }
        }
    }
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("Space3D", tw, th) {
        setPath(path);
        mixer.init();
        setObject(new SpaceGame(this));
        object->load(this);
    }
    
    ~MainWindow() override {}
    
    virtual void event(SDL_Event &e) override {
        object->event(this, e);
    }
    
    virtual void draw() override {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glViewport(0, 0, w, h);
        object->draw(this);
        swap();
        delay();
    }
private:
};

MainWindow *main_w = nullptr;

void eventProc() {
    main_w->proc();
}

int main(int argc, char **argv) {
#ifdef __EMSCRIPTEN__
    MainWindow main_window("/", 1280, 720);
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
    int tw = 1280, th = 720;
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
