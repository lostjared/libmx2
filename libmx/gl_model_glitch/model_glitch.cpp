#include"mx.hpp"
#include"argz.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <GLES3/gl3.h>
#endif

#include"gl.hpp"
#include"loadpng.hpp"
#include"model.hpp"
#include<random>

#ifdef DEBUG_MODE
#define CHECK_GL_ERROR() \
{ GLenum err = glGetError(); \
if (err != GL_NO_ERROR) \
printf("OpenGL Error: %d at %s:%d\n", err, __FILE__, __LINE__); }
#else
#define CHECK_GL_ERROR()
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef __EMSCRIPTEN__
const char *vSource = R"(#version 300 es
precision highp float;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char *fSource = R"(#version 300 es
precision highp float;

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;

uniform sampler2D texture1;

void main()
{
    vec4 texColor = texture(texture1, TexCoord);
    
    // Basic lighting
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);
    vec3 ambient = vec3(0.3, 0.3, 0.3);
    
    vec3 result = (ambient + diffuse) * texColor.rgb;
    FragColor = vec4(result, texColor.a);
}
)";
#else
const char *vSource = R"(#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char *fSource = R"(#version 330 core

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;

uniform sampler2D texture1;

void main()
{
    vec4 texColor = texture(texture1, TexCoord);
    
    // Basic lighting
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);
    vec3 ambient = vec3(0.3, 0.3, 0.3);
    
    vec3 result = (ambient + diffuse) * texColor.rgb;
    FragColor = vec4(result, texColor.a);
}
)";
#endif

float generateRandomFloat(float min, float max) {
    static std::random_device rd; 
    static std::default_random_engine eng(rd()); 
    std::uniform_real_distribution<float> dist(min, max);
    return dist(eng);
}

int generateRandomInt(int min, int max) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(min, max);
    return dist(gen);
}


class ExplodeEmiter {
public:
    static constexpr int MAX_PARTICLES = 20000;

    ExplodeEmiter() = default;
    ~ExplodeEmiter() {
        if (vao != 0) glDeleteVertexArrays(1, &vao);
        if (vbo != 0) glDeleteBuffers(1, &vbo);
    }

    struct Particle {
        float x, y, z;         
        float vx, vy, vz;      
        float r, g, b;         
        float alpha;           
        float size;            
        float life;            
        float maxLife;         
        bool active;           
    };

    
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    
    void load(gl::GLWindow *win) {
        
#ifndef __EMSCRIPTEN__
        const char* particleVS = R"(#version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec4 aColor;
        layout (location = 2) in float aSize;
        
        out vec4 Color;
        
        uniform mat4 projection;
        uniform mat4 view;
        
        void main() {
            vec4 viewPos = view * vec4(aPos, 1.0);
            gl_Position = projection * viewPos;
            
            float distance = length(viewPos.xyz);
            gl_PointSize = aSize / (distance * 0.1); 
            Color = aColor;
        }
        )";

        const char* particleFS = R"(#version 330 core
        in vec4 Color;
        out vec4 FragColor;
        uniform vec4 r_color;
        
        uniform sampler2D particleTexture;
        
        vec4 xor_RGB(vec4 icolor, vec4 source) {
            ivec3 int_color;
            ivec4 isource = ivec4(source * 255);
            for(int i = 0; i < 3; ++i) {
                int_color[i] = int(255 * icolor[i]);
                int_color[i] = int_color[i]^isource[i];
                if(int_color[i] > 255)
                    int_color[i] = int_color[i]%255;
                icolor[i] = float(int_color[i])/255;
            }
            icolor.a = 1.0;
        return icolor;
        }

        void main() {
            vec2 texCoord = gl_PointCoord;
            vec4 texColor = texture(particleTexture, texCoord);
            if(texColor.r < 0.1 && texColor.g < 0.1 && texColor.b < 0.1) {
                discard;
            }
            vec4 finalColor = texColor * Color;
            float dist = length(texCoord - vec2(0.5));
            float alpha = smoothstep(0.5, 0.4, dist) * finalColor.a;
            
            if (alpha < 0.01)
                discard;
            
            FragColor = xor_RGB(finalColor, texColor) * r_color;
            FragColor.a = alpha;
             
            //FragColor = vec4(finalColor.rgb, alpha);

        }
        )";
#else
        const char* particleVS = R"(#version 300 es
        precision highp float;
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec4 aColor;
        layout (location = 2) in float aSize;
        
        out vec4 Color;
        
        uniform mat4 projection;
        uniform mat4 view;
        
        void main() {
            vec4 viewPos = view * vec4(aPos, 1.0);
            gl_Position = projection * viewPos;
            
            float distance = length(viewPos.xyz);
            gl_PointSize = aSize / (distance * 0.1);
            Color = aColor;
        }
        )";

        const char* particleFS = R"(#version 300 es
        precision highp float;
        precision highp int;  
        in vec4 Color;
        out vec4 FragColor;
        uniform vec4 r_color;
        
        uniform sampler2D particleTexture;
        
        
        vec4 xor_RGB(vec4 icolor, vec4 source) {
            
            ivec3 int_color = ivec3(icolor.rgb * 255.0 + 0.5);  
            ivec3 isource = ivec3(source.rgb * 255.0 + 0.5);    
            
            
            for(int i = 0; i < 3; i++) {
                int_color[i] = (int_color[i] ^ isource[i]) & 255;  
            }
            
            
            return vec4(float(int_color.r) / 255.0, 
                        float(int_color.g) / 255.0, 
                        float(int_color.b) / 255.0, 
                        1.0);
        }
        
        void main() {
            vec2 texCoord = gl_PointCoord;
            vec4 texColor = texture(particleTexture, texCoord);
            
            
            if(texColor.r < 0.1 && texColor.g < 0.1 && texColor.b < 0.1) {
                discard;
            }
            
            vec4 finalColor = texColor * Color;
            float dist = length(texCoord - vec2(0.5));
            float alpha = smoothstep(0.5, 0.4, dist) * finalColor.a;
            
            if (alpha < 0.01)
                discard;
            
            vec4 glitchColor = xor_RGB(finalColor, texColor);
            FragColor = vec4(glitchColor.rgb * r_color.rgb, alpha);
        }
        )";
#endif
        if (!shader.loadProgramFromText(particleVS, particleFS)) {
            throw mx::Exception("Failed to load particle shader program");
        }

        
        particles.resize(MAX_PARTICLES); 
        for (auto& p : particles) {
            p.active = false;
        }

        
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        
        glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * 8 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(7 * sizeof(float)));
        glEnableVertexAttribArray(2);
        
        glBindVertexArray(0);
    }

    bool active() const {
        return activeParticles == 0;
    }

    void update(float deltaTime) {
        if (activeParticles == 0) return;
        
        int newActiveCount = 0;
        
        const float GRAVITATIONAL_CONSTANT = 0.5f;      
        const float MIN_DISTANCE = 0.8f;                
        const float PARTICLE_MASS = 1.0f;               
        const float DEBRIS_DRAG = 0.995f;               
        
        for (auto& p : particles) {
            if (!p.active) continue;
            
            glm::vec3 totalForce(0.0f);
            glm::vec3 currentPos(p.x, p.y, p.z);
            glm::vec3 toCenter = position - currentPos;
            float distanceToCenter = glm::length(toCenter);
            
            if (distanceToCenter > MIN_DISTANCE) {
                glm::vec3 centerDirection = glm::normalize(toCenter);
                float centralForce = GRAVITATIONAL_CONSTANT * PARTICLE_MASS / (distanceToCenter * distanceToCenter);
                totalForce += centerDirection * centralForce * 0.1f; 
            }
            glm::vec3 radiusVector = currentPos - position;
            if (glm::length(radiusVector) > 0.1f) {
                glm::vec3 tangentialDirection = glm::normalize(glm::cross(radiusVector, glm::vec3(0.0f, 1.0f, 0.0f)));
                float angularVelocity = 0.5f / glm::length(radiusVector); 
                totalForce += tangentialDirection * angularVelocity * deltaTime * 0.1f; 
            }
            
            p.vx += totalForce.x * deltaTime;
            p.vy += totalForce.y * deltaTime;
            p.vz += totalForce.z * deltaTime;
            p.vx *= DEBRIS_DRAG;
            p.vy *= DEBRIS_DRAG;
            p.vz *= DEBRIS_DRAG;
            p.x += p.vx * deltaTime;
            p.y += p.vy * deltaTime;
            p.z += p.vz * deltaTime;
            p.life += deltaTime;
            float lifeRatio = p.life / p.maxLife;
            if (lifeRatio < 0.1f) {
                p.alpha = lifeRatio / 0.1f;
            } else if (lifeRatio > 0.9f) {
                p.alpha = (1.0f - lifeRatio) / 0.1f;
            } else {
                p.alpha = 1.0f - (lifeRatio - 0.1f) * 0.3f;
            }
            if (lifeRatio < 0.2f) {
                p.size *= 1.002f; 
            } else if (lifeRatio < 0.6f) {
                p.size *= 0.9995f; 
            } else {
                p.size *= 0.998f;
            }
            if (lifeRatio > 0.3f) {
                float fadeFactor = (lifeRatio - 0.3f) / 0.7f; 
                p.r *= (1.0f - fadeFactor * 0.8f);  
                p.g *= (1.0f - fadeFactor * 0.8f);  
                p.b *= (1.0f - fadeFactor * 0.8f);  
            }
            
            if (p.life >= p.maxLife || p.alpha < 0.01f || p.size < 0.1f) {
                p.active = false;
            } else {
                newActiveCount++;
            }
        }
        
        activeParticles = newActiveCount;
    }

    void draw(gl::GLWindow *win) {
        if (activeParticles == 0) return;
        
        
        std::vector<float> particleData;
        particleData.reserve(activeParticles * 8); 
        
        for (const auto& p : particles) {
            if (!p.active) continue;
            
            
            particleData.push_back(p.x);
            particleData.push_back(p.y);
            particleData.push_back(p.z);
            
            
            particleData.push_back(p.r);
            particleData.push_back(p.g);
            particleData.push_back(p.b);
            particleData.push_back(p.alpha);
            
            
            particleData.push_back(p.size);
        }
        
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE); 
        glDepthMask(GL_FALSE); 
        
        shader.useProgram();
        shader.setUniform("projection", projectionMatrix);
        shader.setUniform("view", viewMatrix);
        shader.setUniform("particleTexture", 0);
         shader.setUniform("r_color", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        
        
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, particleData.size() * sizeof(float), particleData.data());
        
        
#ifndef __EMSCRIPTEN__
        glEnable(GL_PROGRAM_POINT_SIZE);
#endif
        glDrawArrays(GL_POINTS, 0, activeParticles);
#ifndef __EMSCRIPTEN__
        glDisable(GL_PROGRAM_POINT_SIZE);
#endif
        
        
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    void explode() {
        
        reset();
        
        const int WAVE_COUNT = 4;
        int particlesPerWave = MAX_PARTICLES / WAVE_COUNT;
        
        struct WaveParams {
            float minSpeed, maxSpeed;
            float minSize, maxSize;
            float minLife, maxLife;
            glm::vec3 baseColor;
        };
        
        WaveParams waves[WAVE_COUNT] = {
            {40.0f, 60.0f, 25.0f, 40.0f, 1.5f, 2.5f, {1.0f, 0.9f, 0.3f}},
            {30.0f, 45.0f, 20.0f, 30.0f, 2.0f, 3.0f, {1.0f, 0.6f, 0.2f}},
            {20.0f, 35.0f, 15.0f, 25.0f, 2.5f, 3.5f, {1.0f, 0.3f, 0.1f}},
            {10.0f, 25.0f, 5.0f, 15.0f, 3.0f, 4.0f, {0.8f, 0.2f, 0.1f}}
        };
        
        int particleIndex = 0;
        
        for (int wave = 0; wave < WAVE_COUNT; wave++) {
            for (int i = 0; i < particlesPerWave && particleIndex < MAX_PARTICLES; i++) {
                float theta = generateRandomFloat(0.0f, 2.0f * M_PI);
                float phi = generateRandomFloat(0.0f, M_PI);
                float x = sin(phi) * cos(theta);
                float y = sin(phi) * sin(theta);
                float z = cos(phi);        
                auto& p = particles[particleIndex++];
                float offset = 0.8f + 0.2f * static_cast<float>(wave) / WAVE_COUNT;
                p.x = position.x + x * offset;
                p.y = position.y + y * offset;
                p.z = position.z + z * offset;
                float speed = generateRandomFloat(waves[wave].minSpeed, waves[wave].maxSpeed);
                p.vx = x * speed;
                p.vy = y * speed;
                p.vz = z * speed;
                p.vx += generateRandomFloat(-5.0f, 5.0f);
                p.vy += generateRandomFloat(-5.0f, 5.0f);
                p.vz += generateRandomFloat(-5.0f, 5.0f);
                //const auto& baseColor = waves[wave].baseColor;
                p.r = generateRandomFloat(0.5f, 1.0f);  
                p.g = generateRandomFloat(0.5f, 1.0f);  
                p.b = generateRandomFloat(0.5f, 1.0f);  
                 p.alpha = 0.1f; 
                p.size = generateRandomFloat(waves[wave].minSize, waves[wave].maxSize);
                p.maxLife = generateRandomFloat(waves[wave].minLife, waves[wave].maxLife);
                p.life = 0.0f; 
                p.active = true;
            }
        }
        activeParticles = particleIndex;
    }
    
    void reset() {
        for (auto& p : particles) {
            p.active = false;
        }
        activeParticles = 0;
    }
    
    void setProjectionMatrix(const glm::mat4& proj) { projectionMatrix = proj; }
    void setViewMatrix(const glm::mat4& view) { viewMatrix = view; }
    void setTextureID(GLuint id) { textureID = id; }

protected:
    gl::ShaderProgram shader;
    std::vector<Particle> particles;
    GLuint vao = 0, vbo = 0;
    int activeParticles = 0;
    glm::mat4 projectionMatrix{1.0f};
    glm::mat4 viewMatrix{1.0f};
    GLuint textureID = 0;
};    


class Game : public gl::GLObject {
    ExplodeEmiter emiter;
    GLuint textureID;
    
public:
    Game() = default;
    virtual ~Game() override {

        if(textureID != 0)
            glDeleteTextures(1, &textureID);

    }

    void load(gl::GLWindow *win) override {
        font.loadFont(win->util.getFilePath("data/font.ttf"), 12);
        if(!shader_program.loadProgramFromText(vSource, fSource)) {
            throw mx::Exception("Failed to load shader program");
        }
        if(!object_file.openModel(win->util.getFilePath("data/cube.mxmod.z"))) {
            throw mx::Exception("Failed to load model");
        }

        object_file.setTextures(win, win->util.getFilePath("data/bg.tex"), win->util.getFilePath("data"));
        object_file.setShaderProgram(&shader_program, "texture1");
        shader_program.useProgram();
        textureID = gl::loadTexture(win->util.getFilePath("data/star.png"));
        emiter.setTextureID(textureID);
        emiter.load(win);
    
    }

    void draw(gl::GLWindow *win) override {
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; 
        lastUpdateTime = currentTime;
        
        glEnable(GL_DEPTH_TEST);
        shader_program.useProgram();
        glm::mat4 projection = glm::perspective(45.0f, (float)win->w / (float)win->h, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, cubePosition);
        model = glm::scale(model, glm::vec3(2.0f, 2.0f, 2.0f));
        model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 1.0f, 0.0f));
        shader_program.setUniform("projection", projection);            
        shader_program.setUniform("view", view);
        shader_program.setUniform("model", model);
        shader_program.setUniform("texture1", 0);
        if(emiter.active())
            object_file.drawArrays();
        emiter.setProjectionMatrix(projection);
        emiter.setViewMatrix(view);
        emiter.update(deltaTime);
        emiter.draw(win);
        glDisable(GL_DEPTH_TEST);
        win->text.setColor({255, 255, 255, 255});
        win->text.printText_Solid(font, 25.0f, 25.0f, "FPS: " + calculateFPS());
        win->text.printText_Solid(font, 25.0f, 50.0f, "Press SPACE to create explosion");
#ifdef __EMSCRIPTEN__
        win->text.printText_Solid(font, 25.0f, 75.0f, "Swipe LEFT/RIGHT to move cube");
        win->text.printText_Solid(font, 25.0f, 100.0f, "Swipe UP/DOWN to move in/out");
        win->text.printText_Solid(font, 25.0f, 125.0f, "Double-tap for explosion");
#endif

        update(deltaTime);
    }
    
    void event(gl::GLWindow *win, SDL_Event &e) override {
        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_SPACE: {
                    create_explosion();
                }
                    break;
                case SDLK_LEFT:
                    cubePosition.x -= 0.5f; 
                    break;
                case SDLK_RIGHT:
                    cubePosition.x += 0.5f; 
                    break;
                case SDLK_UP:
                    cubePosition.z -= 0.5f; 
                    break;
                case SDLK_DOWN:
                    cubePosition.z += 0.5f; 
                    break;
            }
        }

#ifdef __EMSCRIPTEN__  
    if (e.type == SDL_FINGERDOWN) {
        touchStartPos.x = e.tfinger.x * win->w;
        touchStartPos.y = e.tfinger.y * win->h;
        isTouching = true;
        Uint32 currentTime = SDL_GetTicks();
        float tapDistance = glm::length(glm::vec2(touchStartPos.x - lastTapPos.x, 
                                                  touchStartPos.y - lastTapPos.y));
        
        if (currentTime - lastTapTime < DOUBLE_TAP_TIME && 
            tapDistance < TAP_DISTANCE_THRESHOLD) {
            create_explosion();
        }
        lastTapTime = currentTime;
        lastTapPos = touchStartPos;
    }
    else if (e.type == SDL_FINGERUP && isTouching) {
        glm::vec2 touchEndPos(e.tfinger.x * win->w, e.tfinger.y * win->h);
        glm::vec2 swipeVector = touchEndPos - touchStartPos;
        float swipeLength = glm::length(swipeVector);
        if (swipeLength > SWIPE_THRESHOLD) {
            swipeVector = glm::normalize(swipeVector);
            if (abs(swipeVector.x) > abs(swipeVector.y)) {
                if (swipeVector.x > 0) {
                    cubePosition.x += 0.5f;
                } else {
                    cubePosition.x -= 0.5f;
                }
            } else {
                if (swipeVector.y > 0) {
                    cubePosition.z += 0.5f;
                } else {
                    cubePosition.z -= 0.5f;
                }
            }
        }
        
        isTouching = false;
    }
#endif
    }
    void update(float deltaTime) {
        angle += 25.0f * deltaTime;
        if(angle > 360) 
            angle -= 360;
    }
private:
    float angle = 45.0f;
    mx::Font font;
    Uint32 lastUpdateTime = SDL_GetTicks();
    mx::Model object_file;
    gl::ShaderProgram shader_program;
    glm::vec3 cubePosition{0.0f, 0.0f, 0.0f};
#ifdef __EMSCRIPTEN__
    glm::vec2 touchStartPos{0.0f, 0.0f};
    Uint32 lastTapTime = 0;
    bool isTouching = false;
    glm::vec2 lastTapPos{0.0f, 0.0f};
    const float SWIPE_THRESHOLD = 50.0f; 
    const Uint32 DOUBLE_TAP_TIME = 300;  
    const float TAP_DISTANCE_THRESHOLD = 30.0f;
#endif
    std::string calculateFPS() {
        static int frameCount = 0;
        static float timeElapsed = 0.0f;
        static float current_FPS = 0.0f;
        static Uint32 last_update_time = SDL_GetTicks();
        frameCount++;
        Uint32 currentTime = SDL_GetTicks();
        timeElapsed = (currentTime - last_update_time) / 1000.0f;
        
        if (timeElapsed >= 1.0f) {
            current_FPS = frameCount / timeElapsed;
            frameCount = 0;
            last_update_time = currentTime;
        }
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << current_FPS;
        return ss.str();
    }

    void create_explosion() {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, cubePosition);
        model = glm::scale(model, glm::vec3(2.0f, 2.0f, 2.0f));
        model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 1.0f, 0.0f));
        
        glm::vec4 worldPos = model * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        emiter.position = glm::vec3(worldPos);
        emiter.explode();
    }
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("Blank Model", tw, th) {
        setPath(path);
        setObject(new Game());
        object->load(this);
    }
    
    ~MainWindow() override {}
    
    virtual void event(SDL_Event &e) override {}
    
    virtual void draw() override {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glViewport(0, 0, w, h);
        object->draw(this);
        swap();
        delay();
    }
};

MainWindow *main_w = nullptr;

void eventProc() {
    main_w->proc();
}

int main(int argc, char **argv) {
#ifdef __EMSCRIPTEN__
    try {
        MainWindow main_window("/", 1920, 1080);
        main_w = &main_window;
        emscripten_set_main_loop(eventProc, 0, 1);
    } catch(const mx::Exception &e) {
        mx::system_err << "mx: Exception: " << e.text() << "\n";
        mx::system_err.flush();
        exit(EXIT_FAILURE);
    }
#else
    Arguments args = proc_args(argc, argv);
    try {
        MainWindow main_window(args.path, args.width, args.height);
        main_window.loop();
    } catch(const mx::Exception &e) {
        mx::system_err << "mx: Exception: " << e.text() << "\n";
        mx::system_err.flush();
        exit(EXIT_FAILURE);
    }
#endif
    return 0;
}
