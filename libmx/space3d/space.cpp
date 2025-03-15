#include"mx.hpp"
#if defined(__APPLE__) || defined(_WIN32) || defined(__linux__)
#include"argz.hpp"
#endif
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <GLES3/gl3.h>
#endif

#include<ctime>
#include<cstdlib>
#include<tuple>
#include<memory>

#include"gl.hpp"
#include"loadpng.hpp"
#include"model.hpp"
#include<random>
#include"intro.hpp"
#include"explode.hpp"


#define CHECK_GL_ERROR() \
{ GLenum err = glGetError(); \
if (err != GL_NO_ERROR) \
printf("OpenGL Error: %d at %s:%d\n", err, __FILE__, __LINE__); }

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef __EMSCRIPTEN__
const char* vertSource = R"(#version 330 core

layout (location = 0) in vec3 inPosition; 
layout (location = 1) in float inSize;
layout (location = 2) in vec4 inColor;

uniform mat4 MVP;  

out vec4 fragColor;

void main() {
    gl_Position = MVP * vec4(inPosition, 1.0);
    gl_PointSize = inSize;
    fragColor = inColor;
}
)";

const char* fragSource = R"(#version 330 core

in vec4 fragColor;
out vec4 FragColor;

uniform sampler2D spriteTexture;

void main() {
    float dist = length(gl_PointCoord - vec2(0.5));
    if (dist > 0.5) {
        discard;
    }
    vec4 texColor = texture(spriteTexture, gl_PointCoord);
    FragColor = texColor * fragColor;
}
)";

#else
const char* vertSource = R"(#version 300 es

precision highp float;

layout (location = 0) in vec3 inPosition; 
layout (location = 1) in float inSize;
layout (location = 2) in vec4 inColor;

uniform mat4 MVP;

out vec4 fragColor;

void main() {
    gl_Position = MVP * vec4(inPosition, 1.0);
    gl_PointSize = inSize;
    fragColor = inColor;
}
)";

const char* fragSource = R"(#version 300 es

precision highp float;

in vec4 fragColor;
out vec4 FragColor;

uniform sampler2D spriteTexture;

void main() {
    float dist = length(gl_PointCoord - vec2(0.5));
    if (dist > 0.5) {
        discard;
    }

    vec4 texColor = texture(spriteTexture, gl_PointCoord);
    FragColor = texColor * fragColor;
}
)";
#endif
#ifndef __EMSCRIPTEN__
const char* exhaustVertSource = R"(#version 330 core
layout (location = 0) in vec3 inPosition;
layout (location = 1) in float inSize;
layout (location = 2) in vec4 inColor;

uniform mat4 MVP;

out vec4 fragColor;

void main() {
    gl_Position = MVP * vec4(inPosition, 1.0);
    gl_PointSize = inSize;
    fragColor = inColor;
}
)";

const char* exhaustFragSource = R"(#version 330 core
in vec4 fragColor;
out vec4 FragColor;

uniform sampler2D spriteTexture;

void main() {
    float dist = length(gl_PointCoord - vec2(0.5));
    if (dist > 0.5) {
        discard;
    }
    vec4 texColor = texture(spriteTexture, gl_PointCoord);
    FragColor = texColor * fragColor;
}
)";
#else
const char* exhaustVertSource = R"(#version 300 es
precision highp float;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in float inSize;
layout (location = 2) in vec4 inColor;

uniform mat4 MVP;

out vec4 fragColor;

void main() {
    gl_Position = MVP * vec4(inPosition, 1.0);
    gl_PointSize = inSize;
    fragColor = inColor;
}
)";

const char* exhaustFragSource = R"(#version 300 es
precision highp float;

in vec4 fragColor;
out vec4 FragColor;

uniform sampler2D spriteTexture;

void main() {
    float dist = length(gl_PointCoord - vec2(0.5));
    if (dist > 0.5) {
        discard;
    }
    vec4 texColor = texture(spriteTexture, gl_PointCoord);
    FragColor = texColor * fragColor;
}
)";
#endif

class Exhaust {
public:
    struct ExhaustParticle {
        glm::vec3 pos;
        glm::vec3 velocity;
        float life;     
        float maxLife;  
    };

    Exhaust() = default;
    ~Exhaust() {
        glDeleteVertexArrays(1, &exhaustVAO);
        glDeleteBuffers(3, exhaustVBO);
    }

    std::vector<ExhaustParticle> exhaustParticles;
    const size_t maxExhaustParticles = 25;

    void spawnExhaustParticle(const glm::vec3& shipPos, float shipRotation) {
        
        ExhaustParticle p1;
        glm::vec3 leftWingOffset = glm::rotate(glm::mat4(1.0f), glm::radians(shipRotation), glm::vec3(0,0,1))
                             * glm::vec4(-2.0f, -0.8f, 0.0f, 1.0f); 
        p1.pos = shipPos + leftWingOffset;
        p1.velocity = glm::rotate(glm::mat4(1.0f), glm::radians(shipRotation), glm::vec3(0,0,1)) * glm::vec4(0.0f, -10.0f, 0.0f, 0.0f);
        p1.life = 0.0f;
        p1.maxLife = 1.0f;  
        exhaustParticles.push_back(p1);

        
        ExhaustParticle p2;
        glm::vec3 rightWingOffset = glm::rotate(glm::mat4(1.0f), glm::radians(shipRotation), glm::vec3(0,0,1))
                             * glm::vec4(2.0f, -0.8, 0.0f, 1.0f); 
        p2.pos = shipPos + rightWingOffset;
        p2.velocity = glm::rotate(glm::mat4(1.0f), glm::radians(shipRotation), glm::vec3(0,0,1)) * glm::vec4(0.0f, -10.0f, 0.0f, 0.0f);
        p2.life = 0.0f;
        p2.maxLife = 1.0f;  
        exhaustParticles.push_back(p2);

        while (exhaustParticles.size() > maxExhaustParticles) {
            exhaustParticles.erase(exhaustParticles.begin());
        }
    }

    void updateExhaustParticles(float deltaTime) {
        for(auto& p : exhaustParticles) {
            p.pos += p.velocity * deltaTime;
            p.life += deltaTime;
        }
        exhaustParticles.erase(
            std::remove_if(exhaustParticles.begin(), exhaustParticles.end(),
                [](const ExhaustParticle& p) { return p.life >= p.maxLife; }),
            exhaustParticles.end()
        );
    }


    GLuint exhaustVAO = 0;
    GLuint exhaustVBO[3];
    gl::ShaderProgram exhaustShader;


    void initializeExhaustBuffers() {
        glGenVertexArrays(1, &exhaustVAO);
        glGenBuffers(3, exhaustVBO);
        glBindVertexArray(exhaustVAO);
        glBindBuffer(GL_ARRAY_BUFFER, exhaustVBO[0]);
        glBufferData(GL_ARRAY_BUFFER, maxExhaustParticles * 3 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glBindBuffer(GL_ARRAY_BUFFER, exhaustVBO[1]);
        glBufferData(GL_ARRAY_BUFFER, maxExhaustParticles * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glBindBuffer(GL_ARRAY_BUFFER, exhaustVBO[2]);
        glBufferData(GL_ARRAY_BUFFER, maxExhaustParticles * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glBindVertexArray(0);
    }

    void drawExhaustParticles(const glm::mat4& MVP, GLuint exhaustTexture) {
        std::vector<float> positions;
        std::vector<float> sizes;
        std::vector<float> colors;
        positions.reserve(exhaustParticles.size() * 3);
        sizes.reserve(exhaustParticles.size());
        colors.reserve(exhaustParticles.size() * 4);
        float index = 0.3f;
        for(auto& p : exhaustParticles) {
            positions.push_back(p.pos.x);
            positions.push_back(p.pos.y);
            positions.push_back(p.pos.z);
            float lifeFactor = 1.0f - (p.life / p.maxLife);
            sizes.push_back(10.0f * lifeFactor);  
            colors.push_back(1.0f - index);            
            colors.push_back(1.0f);            
            colors.push_back(1.0f);            
            colors.push_back(lifeFactor);
            index -= 0.2f;
        }
        
        glBindBuffer(GL_ARRAY_BUFFER, exhaustVBO[0]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, positions.size() * sizeof(float), positions.data());
        
        glBindBuffer(GL_ARRAY_BUFFER, exhaustVBO[1]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizes.size() * sizeof(float), sizes.data());
        
        glBindBuffer(GL_ARRAY_BUFFER, exhaustVBO[2]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, colors.size() * sizeof(float), colors.data());
        exhaustShader.useProgram();
        exhaustShader.setUniform("MVP", MVP);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, exhaustTexture);
        exhaustShader.setUniform("spriteTexture", 0);
        
        glBindVertexArray(exhaustVAO);
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(exhaustParticles.size()));
        CHECK_GL_ERROR(); // Add this line to check for errors after drawing
    }
};

float generateRandomFloat(float min, float max) {
    static std::random_device rd; 
    static std::default_random_engine eng(rd()); 
    std::uniform_real_distribution<float> dist(min, max);
    return dist(eng);
}

class StarField : public gl::GLObject {
public:
    struct Particle {
        float x, y, z;   
        float vx, vy, vz; 
        float life;
        float twinkle;
    };

    static constexpr int NUM_PARTICLES = 2500;
    gl::ShaderProgram program;
    GLuint VAO, VBO[3];
    GLuint texture;
    std::vector<Particle> particles;
    Uint32 lastUpdateTime = 0;
    float cameraZoom = 3.0f;   
    float cameraRotation = 0.0f; 

    StarField() : particles(NUM_PARTICLES) {}

    ~StarField() override {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(3, VBO);
        glDeleteTextures(1, &texture);
    }

    void load(gl::GLWindow *win) override {
        if(!program.loadProgramFromText(vertSource, fragSource)) {
            throw mx::Exception("Error loading shader");
        }

        for (auto& p : particles) {
            p.x = generateRandomFloat(-1.0f, 1.0f);
            p.y = generateRandomFloat(-1.0f, 1.0f);
            p.z = generateRandomFloat(-5.0f, -2.0f); 
            p.vx = generateRandomFloat(-0.02f, 0.02f); 
            p.vy = generateRandomFloat(-0.02f, 0.02f); 
            p.vz = generateRandomFloat(0.2f, 0.5f);    
            p.life = generateRandomFloat(0.6f, 1.0f);
            p.twinkle = generateRandomFloat(1.0f, 5.0f);
        }
        glGenVertexArrays(1, &VAO);
        glGenBuffers(3, VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
        glBufferData(GL_ARRAY_BUFFER, NUM_PARTICLES * 3 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);\
        glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
        glBufferData(GL_ARRAY_BUFFER, NUM_PARTICLES * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
        glBufferData(GL_ARRAY_BUFFER, NUM_PARTICLES * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
        texture = gl::loadTexture(win->util.getFilePath("data/star.png"));
        cameraRotation = 356.0f;
        cameraZoom = 0.09f;
        lastUpdateTime = SDL_GetTicks();
    }

    void event(gl::GLWindow *win, SDL_Event &e) override {

    }
    
    void draw(gl::GLWindow *win) override {
#ifndef __EMSCRIPTEN__
        glEnable(GL_PROGRAM_POINT_SIZE);
#endif
        glDisable(GL_DEPTH_TEST);

        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; // seconds
        lastUpdateTime = currentTime;

        update(deltaTime);

        CHECK_GL_ERROR();

        program.useProgram();

        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f), 
            (float)win->w / (float)win->h, 
            0.1f, 
            100.0f
        );

        glm::vec3 cameraPos(
            cameraZoom * sin(glm::radians(cameraRotation)), 
            0.0f,                                           
            cameraZoom * cos(glm::radians(cameraRotation))  
        );
        glm::vec3 target(0.0f, 0.0f, 0.0f); 
        glm::vec3 up(0.0f, 1.0f, 0.0f);     \
        glm::mat4 view = glm::lookAt(cameraPos, target, up);
        glm::mat4 model = glm::mat4(1.0f);  
        glm::mat4 MVP = projection * view * model;
        program.setUniform("MVP", MVP);
        program.setUniform("spriteTexture", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindVertexArray(VAO);
        glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);
        CHECK_GL_ERROR();
    }

    void update(float deltaTime) {
        if(deltaTime > 0.1f) 
            deltaTime = 0.1f;
        std::vector<float> positions;
        std::vector<float> sizes;
        std::vector<float> colors;
        positions.reserve(NUM_PARTICLES * 3);
        sizes.reserve(NUM_PARTICLES);
        colors.reserve(NUM_PARTICLES * 4);
        for (auto& p : particles) {
            p.x += p.vx * deltaTime;
            p.y += p.vy * deltaTime;
            p.z += p.vz * deltaTime;
            if (p.z > 0.0f) {
                p.x = generateRandomFloat(-1.0f, 1.0f);
                p.y = generateRandomFloat(-1.0f, 1.0f);
                p.z = generateRandomFloat(-5.0f, -2.0f); 
                p.vx = generateRandomFloat(-0.02f, 0.02f); 
                p.vy = generateRandomFloat(-0.02f, 0.02f); 
                p.vz = generateRandomFloat(0.2f, 0.5f);    
                p.life = generateRandomFloat(0.6f, 1.0f);
                p.twinkle = generateRandomFloat(1.0f, 5.0f);
            }
            float twinkleFactor = 0.5f * (1.0f + sin(SDL_GetTicks() * 0.001f * p.twinkle));
            float brightness = p.life * twinkleFactor;
            positions.push_back(p.x);
            positions.push_back(p.y);
            positions.push_back(p.z);
            float size = 10.0f * p.life;
            sizes.push_back(size);
            float alpha = p.life;
            colors.push_back(brightness);
            colors.push_back(brightness);
            colors.push_back(brightness);
            colors.push_back(alpha);
        }

        glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, positions.size() * sizeof(float), positions.data());

        glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizes.size() * sizeof(float), sizes.data());

        glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, colors.size() * sizeof(float), colors.data());
    }
};



bool isColliding(const glm::vec3 &posA, float radiusA, const glm::vec3 &posB, float radiusB) {
    float dx = posA.x - posB.x;
    float dy = posA.y - posB.y;
    float dz = posA.z - posB.z;
    float distSq = dx * dx + dy * dy + dz * dz;
    float radiusSum = radiusA + radiusB;
    float radiusSumSq = radiusSum * radiusSum;
    return distSq <= radiusSumSq;
}

enum class EnemyType { SHIP=1, SAUCER, TRIANGLE, BOSS };

class Enemy {
public:
    
    mx::Model *object = nullptr;
    gl::ShaderProgram *shader = nullptr;
    glm::vec3 object_pos;
    bool ready = false;
    float initial_x = 0.0f, initial_y = 0.0f;
    int verticalDirection = 1;
    bool isSpinning = false;
    float spinAngle = 0.0f;
    float spinSpeed = 360.0f; 
    float spinDuration = 0.8f;
    float elapsedSpinTime = 0.0f;
    GLuint fire = 0;
    glm::vec4 particleColor;

    Enemy(mx::Model *m, EnemyType type, gl::ShaderProgram *shaderv) : object{m}, shader{shaderv}, etype{type} {
    }
    virtual ~Enemy() = default;
    Enemy(const Enemy &e) = delete;
    Enemy &operator=(const Enemy &e) = delete;
    Enemy(Enemy &&e) :  object{e.object}, shader{e.shader}, object_pos{e.object_pos}, etype{e.etype} {
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
    virtual void draw() {
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
                elapsedSpinTime = 0.0f;
            }
        }
    }

protected:
    EnemyType etype;    
};

class EnemyShip : public Enemy {
public:

    EnemyShip(mx::Model *m, gl::ShaderProgram *shadev) : Enemy(m, EnemyType::SHIP, shadev) {
        particleColor = glm::vec4(1.0f, 0.3f, 0.0f, 1.0f);

    }
    ~EnemyShip() override {}
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
    EnemySaucer(mx::Model *m, gl::ShaderProgram *shadev) : Enemy(m, EnemyType::SAUCER, shadev) {
        particleColor = glm::vec4(1.0f, 0.3f, 0.0f, 1.0f);
    }
    ~EnemySaucer() override {}
    virtual void update(float deltatime) override {
    
    }
    virtual void move(float deltaTime, float speed) override {
        object_pos.y -= speed * deltaTime;
    }
};

class EnemyTriangle : public Enemy {
public:
    EnemyTriangle(mx::Model *m, gl::ShaderProgram *shadev) : Enemy(m, EnemyType::TRIANGLE, shadev) {
        particleColor = glm::vec4(1.0f, 0.3f, 0.0f, 1.0f);
    }
    ~EnemyTriangle() override {}
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

class EnemyBoss : public Enemy {
public:
    int hit = 0;
    bool active = false;
    bool hit_flash = false;
    int max_hits = 25;

    EnemyBoss(mx::Model *m, gl::ShaderProgram *shadev) : Enemy(m, EnemyType::BOSS, shadev) {
        particleColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    }
    ~EnemyBoss() override {}
    virtual void update(float deltatime) override {
        
    }

    void reset() {
        hit = 0;
        active = false;
        angleOffset = 0.0f;
        isSpinning = false;
    }

    void hitBoss() {
        if(isSpinning == false) {
            ++hit;
            if(hit > max_hits) {
                isSpinning = true;
                hit = 0;
                max_hits += 10;
                spinAngle = 0.0f;
            }
        }
    }
    
    virtual void move(float deltaTime, float speed) override {
        if (active) {
            angleOffset += 0.4 * deltaTime;
            float radius = 35.0f;
            object_pos.x = initial_x + radius * cos(angleOffset);
            object_pos.y += verticalDirection * speed * 0.5f * deltaTime; 
       }
    }
    virtual void reverseDirection(float screenTop, float screenBottom) override {
        if(active) {
            if (object_pos.y > screenBottom) {
                verticalDirection = -1; 
            } else if (object_pos.y < screenTop) {
                verticalDirection = 1;  
            }
        }
    }
    
private:
    float angleOffset = 0.0f;
    
};

class SpaceGame : public gl::GLObject {
public:
    bool fill = true;   
    GLuint fire = 0;
    int snd_fire = 0, snd_crash = 0, snd_takeoff = 0;
    bool launch_ship = true;
    gl::ShaderProgram shaderProgram, textShader;
    mx::Font font;
    mx::Input stick;
    int score = 0, lives = 5;
    mx::Model ship;
    mx::Model projectile;
    mx::Model enemy_ship, saucer, triangle, ufo_boss, planet_boss, star_boss;
    glm::vec3 ship_pos;
    std::vector<std::tuple<glm::vec3, glm::vec3>> projectiles;
    std::vector<std::unique_ptr<Enemy>> enemies;
    EnemyBoss boss;
    effect::ExplosionEmiter ex_emiter;
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
    int enemies_crashed = 0, max_crashed = 20; 
    float bossRadius = 9.2f;
    float bossSize = 9.2f;
    int level = 1;
    StarField field;
    Exhaust exhaust;
    
    SpaceGame(gl::GLWindow *win) : score{0}, lives{5}, ship_pos(0.0f, -20.0f, -70.0f), boss(&enemy_ship, &shaderProgram) {}
    
    virtual ~SpaceGame() {
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
        if(exhaust.exhaustShader.loadProgramFromText(exhaustVertSource, exhaustFragSource) == false) {
            throw mx::Exception("Could not load shader program exhaust");
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
        if(!ufo_boss.openModel(win->util.getFilePath("data/objects/g_ufo.mxmod"))) {
            throw mx::Exception("Could not open g_ufo.mxmod");
        }
        if(!planet_boss.openModel(win->util.getFilePath("data/objects/saturn.mxmod"))) {
            throw mx::Exception("Could not open saturn.mxmod");
        }
        if(!star_boss.openModel(win->util.getFilePath("data/objects/star.mxmod"))) {
            throw mx::Exception("Could not open star.mxmod");
        }
        
        field.load(win);
        exhaust.initializeExhaustBuffers();

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
        ex_emiter.projection = projection;
        glm::mat4 view = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 5.0f),  
            glm::vec3(0.0f, 0.0f, 0.0f),  
            glm::vec3(0.0f, 1.0f, 0.0f)   
        );
        shaderProgram.setUniform("view", view);   
        ex_emiter.view = view;
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
        ufo_boss.setShaderProgram(&shaderProgram, "texture1");
        ufo_boss.setTextures(win, win->util.getFilePath("data/objects/metal.tex"),  win->util.getFilePath("data/objects"));
        planet_boss.setShaderProgram(&shaderProgram, "texture1");
        planet_boss.setTextures(win, win->util.getFilePath("data/objects/planet.tex"), win->util.getFilePath("data/objects"));
        star_boss.setShaderProgram(&shaderProgram, "texture1");
        star_boss.setTextures(win, win->util.getFilePath("data/objects/planet.tex"), win->util.getFilePath("data/objects"));
        
        fire = gl::loadTexture(win->util.getFilePath("data/objects/flametex.png"));
        ex_emiter.load(win);
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
        CHECK_GL_ERROR(); // Add this line to check for errors before drawing
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
        /*starsShader.useProgram();
        glBindVertexArray(starsVAO);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDrawArrays(GL_POINTS, 0, numStars);
        glBindVertexArray(0);*/
        field.draw(win);
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
        if(!wait_explode) {
            if(!isSpinning)
                ship.drawArrays();
            else
                ship.drawArraysWithTexture(fire, "texture1");
        }
        model = glm::translate(glm::mat4(1.0f), ship_pos);
        glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        float aspectRatio = (float)win->w / (float)win->h;
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);
        glm::mat4 mvp = projection * view;
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        exhaust.drawExhaustParticles(mvp, fire);
        glDisable(GL_BLEND);
        shaderProgram.useProgram();
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
                    default:
                    break;
                }    
                if(e->isSpinning)
                    projModel = glm::rotate(projModel, glm::radians(e->spinAngle), glm::vec3(1.0f, 0.0f, 0.0f));

                shaderProgram.setUniform("model", projModel);
                e->draw();
            }
        }

        if(boss.active) {
            glm::mat4 projModel = glm::translate(glm::mat4(1.0f), boss.object_pos);
            projModel = glm::rotate(projModel, glm::radians(-90.0f), glm::vec3(1.0f,0.0f,0.0f));
            projModel = glm::rotate(projModel, glm::radians(projectileRotation), glm::vec3(0.0f,1.0f,0.0f));
            projModel = glm::scale(projModel, glm::vec3(bossSize, bossSize, bossSize));
            projModel = glm::rotate(projModel, glm::radians(spinAngle), glm::vec3(0.0f, 1.0f, 0.0f));
            projModel = glm::rotate(projModel, glm::radians(spinAngle), glm::vec3(0.0f, 0.0f, 1.0f));
            shaderProgram.setUniform("model", projModel);
            boss.draw();
        }

        ex_emiter.draw(win);

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        textShader.useProgram();
        SDL_Color white = {255, 255, 255, 255};
        int textWidth = 0, textHeight = 0;
        GLuint textTexture;
#ifndef __EMSCRIPTEN__
        if(fill == false) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);        
        }
#endif

        if(game_over == true) {
            textTexture = createTextTexture("Double Tap or Press Button Start or Enter Key to Start New Game Your Score: " + std::to_string(score), font.wrapper().unwrap(), white, textWidth, textHeight);
        } else if(launch_ship == true) {
            textTexture = createTextTexture("Double Tap or Press X Button or Z Key to Launch Ship", font.wrapper().unwrap(), white, textWidth, textHeight);
        } else {
            std::string text;
            if(boss.active == false)
                text = "Level: " + std::to_string(level) + " Lives: " + std::to_string(lives) + " Score: " + std::to_string(score) + " Cleared: " + std::to_string(enemies_crashed) + "/" + std::to_string(max_crashed);
            else
                text = "Level: " + std::to_string(level) + " Lives: " + std::to_string(lives) + " Score: " + std::to_string(score) + " Mothership: " + std::to_string(boss.hit) + "/" + std::to_string(boss.max_hits);

            textTexture = createTextTexture(text, font.wrapper().unwrap(), white, textWidth, textHeight);
        }
        if(wait_explode == false || game_over == true)
            renderText(textTexture, textWidth, textHeight, win->w, win->h);
        glDeleteTextures(1, &textTexture);
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
#ifndef __EMSCRIPTEN__
        if(fill == false) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);        
        }
#endif
        shaderProgram.useProgram();
        CHECK_GL_ERROR(); // Add this line to check for errors after drawing
    }

    void newGame() {
        game_over = false;
        lives = 5;
        score = 0;
        launch_ship = true;
        max_crashed = 20;
        level = 1;
        boss.max_hits = 20; 
        boss.object = &enemy_ship;   
        bossRadius = 9.2f;
        bossSize = 9.2f;
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
            case SDL_KEYDOWN:
            switch(e.key.keysym.sym) {
#ifdef DEBUG_MODE
                case SDLK_F4:
                   level = 5;
                   enemies_crashed = 60;
                break;
                case SDLK_F3: {
                    level = 4;
                    enemies_crashed = 60;
                }
                break;
                case SDLK_F2:
                    level = 3;
                    enemies_crashed = 40;   
                break;
                case SDLK_F1:
                    level = 2;
                    enemies_crashed = 30;
                break; 
#endif

		case SDLK_p: {

#ifndef __EMSCRIPTEN__
                    if(fill == true) {
                        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                        fill = false;
                    } else {
                        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                        fill = true;
                    }
#endif
                }
                break;
            }

            break;
            case SDL_FINGERDOWN: {

                
                if(game_over == true) { 
                    newGame();
                    return;
                }

                if(launch_ship == true && wait_explode == false) {
                    launch_ship = false;
                     if(!wait_explode && !win->mixer.isPlaying(0))
                        Mix_HaltChannel(0);

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
                if ((ticks - last_shot_time > 300) && wait_explode == false) {
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
                ship_pos.x = glm::clamp(ship_pos.x, std::get<0>(screenx) + 1.0f, std::get<1>(screenx) - 1.0f);
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
                newGame();
                launch_ship = true;
            }
            return;
        }

        if(launch_ship == true) {

            if(wait_explode && ex_emiter.explosions.size() != 0) {
                return;
            } else {
                wait_explode = false;
                ship_pos = glm::vec3(0.0f, -20.0f, -70.0f);
            }

            if(stick.getButton(mx::Input_Button::BTN_X) && wait_explode == false) {
                launch_ship = false;

                if(!wait_explode && !win->mixer.isPlaying(0))
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
            if(stick.getButton(mx::Input_Button::BTN_A) && wait_explode == false) {
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

    bool wait_explode = false;

    void die() {
        enemies.clear();
        projectiles.clear();
        launch_ship = true;
        wait_explode = true;
        barrelRollAngle = 0.0f;
        isBarrelRolling = false;
        shipRotation = 0.0f;
        enemies_crashed = 0; 
        lives--;
        if(lives < 0) {
            game_over = true;
            launch_ship = false;
        }
        boss.active = false;
        boss.reset();
    }

    void update(gl::GLWindow *win, float deltaTime) {

        checkInput(win, deltaTime);
        updateSpin(deltaTime);

        ex_emiter.update(win, deltaTime);

        if(ready == true) {
            ex_emiter.explode(win, ship_pos, glm::vec4(1.0f, 0.0f, 1.0f, 1.0f), false);
            die();
            wait_explode = true;
            ready = false;
            return;
        }

        exhaust.updateExhaustParticles(deltaTime);
       
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

        if(launch_ship == true || game_over == true) {
            return;
        }


         if (isBarrelRolling) {
           barrelRollAngle += barrelRollSpeed * deltaTime;
            if (barrelRollAngle >= 360.0f) {
                barrelRollAngle = 0.0f;
                isBarrelRolling = false;
            }
        }

       
        enemyReleaseTimer += deltaTime;
        if (enemyReleaseTimer >= enemyReleaseInterval) {
            releaseEnemy(win);             
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

        if(boss.active) {
            boss.move(deltaTime, enemySpeed);
            boss.reverseDirection(std::get<2>(screenx), std::get<3>(screenx));
        }

        for (int i = (int)enemies.size() - 1; i >= 0; i--) {
            enemies[i]->updateSpin(deltaTime);
            if (enemies[i]->ready) {
                ex_emiter.explode(win, enemies[i]->object_pos, enemies[i]->particleColor, false);
                enemies.erase(enemies.begin() + i);
                if(boss.active == false) enemies_crashed ++;
            }
        }
        if(boss.active) {
            boss.updateSpin(deltaTime);
        }
        if(boss.ready) {
            ex_emiter.explode(win, boss.object_pos, boss.particleColor, true); 
            boss.active = false;
            boss.reset();
            boss.ready = false;
            enemies_crashed = 0;
            max_crashed += 5;
            level++;
            if(level == 2) {
                boss.object = &ufo_boss;
                bossRadius = 10.0;
                bossSize = 25.0f;
            } else if(level == 3) {
                boss.object = &saucer;
                bossRadius = 12.0f;
                bossSize = 15.0f;
            } else if(level == 4) {
                boss.object = &star_boss;
                bossRadius = 9.2f * 2;
                bossSize = 9.2f * 2;
            } else if (level >= 5) {
                boss.object = &planet_boss;
                bossRadius = 25.2f;
                bossSize = 12.2f;
            }
       }

        if (!projectiles.empty() && (!enemies.empty()||boss.active == true)) {
            float projectileRadius = 0.7f;  
            float shipRadius = 1.2f;
            float saucerRadius = 1.5f;
            float triangleRadius = 1.0f; 
            
            for (int pi = (int)projectiles.size() - 1; pi >= 0; pi--) {
                bool projectileDestroyed = false;
                if(boss.active) {
                    if (isColliding(std::get<0>(projectiles[pi]), projectileRadius, boss.object_pos, bossRadius) || isColliding(std::get<1>(projectiles[pi]), projectileRadius, boss.object_pos, bossRadius)) {
                        projectiles.erase(projectiles.begin() + pi);
                        score += 25; 
                        projectileDestroyed = true;
                        boss.hitBoss();

                        if(!win->mixer.isPlaying(2)) {
                            Mix_HaltChannel(1);
                            win->mixer.playWav(snd_crash, 0, 2);

                        }
                        break; 
                    }
                }

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
                        default:
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
                    default:
                        break;
                }
                if (isSpinning == false && isColliding(ship_pos, playerRadius, enemy->object_pos, enemyRadius)) {
                    isSpinning = true;
                    //enemy->explosion->trigger(enemy->object_pos);
                    return;
                }
            }
    }
    
    exhaust.spawnExhaustParticle(ship_pos, shipRotation);
    

    if (boss.active == true && isSpinning == false && isColliding(ship_pos, playerRadius, boss.object_pos, bossRadius)) {
            isSpinning = true;
            return;
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
        CHECK_GL_ERROR();
        glGenBuffers(1, &VBO);
        CHECK_GL_ERROR();
        
        glBindVertexArray(VAO);
        CHECK_GL_ERROR();
        
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        CHECK_GL_ERROR();
        
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // Line 1391
        CHECK_GL_ERROR();
        
        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        CHECK_GL_ERROR();
        glEnableVertexAttribArray(0);
        CHECK_GL_ERROR();
        
        // Texture coord attribute
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        CHECK_GL_ERROR();
        glEnableVertexAttribArray(1);
        CHECK_GL_ERROR();
        
        textShader.useProgram();
        CHECK_GL_ERROR();
        textShader.setUniform("textTexture", 0);
        CHECK_GL_ERROR();
        glActiveTexture(GL_TEXTURE0);
        CHECK_GL_ERROR();
        glBindTexture(GL_TEXTURE_2D, texture);
        CHECK_GL_ERROR();
        
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        CHECK_GL_ERROR();
        
        glBindVertexArray(0);
        CHECK_GL_ERROR();
        glDeleteBuffers(1, &VBO);
        CHECK_GL_ERROR();
        glDeleteVertexArrays(1, &VAO);
        CHECK_GL_ERROR();
    }

private:

    void releaseEnemy(gl::GLWindow *win) {
        if(enemies_crashed < max_crashed) {
            int r = rand()%3;
            switch(r) {
                case 0:
                    enemies.push_back(std::make_unique<EnemyShip>(&enemy_ship, &shaderProgram));
                break;
                case 1:
                    if(boss.active)
                        return;
                    enemies.push_back(std::make_unique<EnemySaucer>(&saucer, &shaderProgram));
                break;
                case 2:
                    enemies.push_back(std::make_unique<EnemyTriangle>(&triangle, &shaderProgram));
                break;
            }
            enemies.back()->fire = fire;
            enemies.back()->object_pos = glm::vec3(generateRandomFloat(std::get<0>(screenx)+6.0f, std::get<1>(screenx))-6.0f, std::get<3>(screenx)-1.0f, -70.0f);
            enemies.back()->initial_x = enemies.back()->object_pos.x;
        } else {
            if(boss.active == false) {
                boss.active = true;
                boss.fire = fire;
                boss.object_pos = glm::vec3(generateRandomFloat(std::get<0>(screenx)+6.0f, std::get<1>(screenx))-6.0f, std::get<3>(screenx)-1.0f, -70.0f);
                boss.initial_x = boss.object_pos.x;
                boss.initial_y = boss.object_pos.y;
                enemies_crashed = 0;
                
            }
        }
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

void IntroScreen::draw(gl::GLWindow *win) {
#ifndef __EMSCRIPTEN__
    Uint32 currentTime = SDL_GetTicks();
#else
    double currentTime = emscripten_get_now();
#endif
    program.useProgram();
    program.setUniform("alpha", fade);
    logo.draw();
    if((currentTime - lastUpdateTime) > 25) {
        lastUpdateTime = currentTime;
        fade += 0.01;
    }
    if(fade >= 1.0) {
        win->setObject(new SpaceGame(win));
        win->object->load(win);
        return;
    }
}

class MainWindow : public gl::GLWindow {
public:
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("Space3D", tw, th) {
        setPath(path);
        mixer.init();
        setObject(new IntroScreen());
        object->load(this);
    }
    
    ~MainWindow() override {}
    
    virtual void event(SDL_Event &e) override {
        
    }

    Uint32 fstart = SDL_GetTicks();
    
    virtual void draw() override {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glViewport(0, 0, w, h);
        fstart = SDL_GetTicks();
        object->draw(this);
        swap();
        delay_();
    }
	
    void delay_() {
#if !defined(__EMSCRIPTEN__) && !defined(__linux__)
        const int frameDelay = 1000 / 60;
        int frameTime = SDL_GetTicks() - fstart;
        if (frameDelay > frameTime) {
            SDL_Delay(frameDelay - frameTime);
        }
#endif 
     }    

private:
};



MainWindow *main_w = nullptr;

void eventProc() {
    main_w->proc();
}

int main(int argc, char **argv) {
    mx::system_out << "Space3D v" << PROJECT_VERSION_MAJOR << "." << PROJECT_VERSION_MINOR << "\nhttps://lostsidedead.biz\n";
#ifdef __EMSCRIPTEN__
    MainWindow main_window("/", 1920, 1080);
    main_w =&main_window;
    emscripten_set_main_loop(eventProc, 0, 1);
#elif defined(__APPLE__) || defined(_WIN32) || defined(__linux__)
    Arguments args = proc_args(argc, argv);
    try {
        MainWindow main_window(args.path, args.width, args.height);
        if(args.fullscreen) {
            SDL_ShowCursor(SDL_FALSE);
            main_window.setFullScreen(true);
        }
        main_window.loop();
        if(args.fullscreen) {
            SDL_ShowCursor(SDL_TRUE);
        }
    } catch(const mx::Exception &e) {
        mx::system_err << "mx: Exception: " << e.text() << "\n";
        mx::system_err.flush();
        exit(EXIT_FAILURE);
    }
#endif
    return 0;
}
