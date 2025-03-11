#include"mx.hpp"
#include"argz.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <GLES3/gl3.h>
#endif

#include"gl.hpp"
#include"loadpng.hpp"

#define CHECK_GL_ERROR() \
{ GLenum err = glGetError(); \
if (err != GL_NO_ERROR) \
printf("OpenGL Error: %d at %s:%d\n", err, __FILE__, __LINE__); }

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct FireParticle {
    float x,y,z;
    float speed;
    float life;
    float fade;
    float r,g,b;
    float scale;
    float gravity;
    float wind;  
    float angle;
    float angleSpeed;
    float angleSpeedVar;    
    float scaleSpeed;
    float scaleSpeedVar;
    float speedVar;
    float gravityVar;       
    float windVar;
    float rVar; 
};


class FireEmiter {
public:
    static const int MAX_PARTICLES = 25000;
    
    FireEmiter() = default;
    ~FireEmiter() {
        releaseResources();
    }

    void load(gl::GLWindow *win) {
        SDL_Surface* fireSurface = png::LoadPNG(win->util.getFilePath("data/fire_particle.png").c_str());
        if(!fireSurface) {
            throw mx::Exception("Failed to load fire texture");
        }
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fireSurface->w, fireSurface->h, 
                    0, GL_RGBA, GL_UNSIGNED_BYTE, fireSurface->pixels);
        
        SDL_FreeSurface(fireSurface);

#ifdef __EMSCRIPTEN__

const char *vertexShader = R"(#version 300 es
precision highp float;
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 lifeFade;
layout(location = 2) in vec3 color;
layout(location = 3) in float scale;
out vec4 particleColor;
out float particleLife;
uniform mat4 viewProj;
uniform float time;

void main() {
    vec3 pos = position;
    pos.x += sin(time * 1.5 + position.y * 2.0) * 0.05;
    gl_Position = viewProj * vec4(pos, 1.0);
    float particleScale = scale * (1.0 + sin(time * 3.0 + position.y * 5.0) * 0.1);
    float life = lifeFade.x;
    float distanceToCamera = gl_Position.w;
    gl_PointSize = particleScale * 100.0 / distanceToCamera;
    particleColor = vec4(color, life);
    particleLife = life;
}
)";
            
const char *fragmentShader = R"(#version 300 es
precision highp float;
in vec4 particleColor;
in float particleLife;
out vec4 fragColor;
uniform sampler2D fireTexture;
uniform float time;

void main() {
    vec2 texCoord = gl_PointCoord;
    vec4 texColor = texture(fireTexture, texCoord);
    
    float alpha = texColor.a * particleLife * 1.8;
    if(texColor.r < 0.1) discard;
    vec3 fireColor = texColor.rgb;
    fireColor.r = 1.0;
    fireColor.g = 0.2;
    fireColor.b = 0.0;

    
    float flicker = 0.7 + 0.3 * sin(time * 10.0 + gl_FragCoord.y * 0.2) * 
                   (0.8 + 0.2 * sin(time * 7.0 + gl_FragCoord.x * 0.15));
    
    fragColor = vec4(fireColor * flicker, alpha);
    if(alpha < 0.01) discard;
}
)";

#else
        
const char *vertexShader = R"(#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 lifeFade;
layout(location = 2) in vec3 color;
layout(location = 3) in float scale;
out vec4 particleColor;
out float particleLife;
uniform mat4 viewProj;
uniform float time;

void main() {
    vec3 pos = position;
    pos.x += sin(time * 1.5 + position.y * 2.0) * 0.05;
    gl_Position = viewProj * vec4(pos, 1.0);
    float particleScale = scale * (1.0 + sin(time * 3.0 + position.y * 5.0) * 0.1);
    float life = lifeFade.x;
    float distanceToCamera = gl_Position.w;
    gl_PointSize = particleScale * 100.0 / distanceToCamera;
    particleColor = vec4(color, life);
    particleLife = life;
}
)";
            
const char *fragmentShader = R"(#version 330 core
in vec4 particleColor;
in float particleLife;
out vec4 fragColor;
uniform sampler2D fireTexture;
uniform float time;

void main() {
    vec2 texCoord = gl_PointCoord;
    vec4 texColor = texture(fireTexture, texCoord);
    
    float alpha = texColor.a * particleLife * 1.8;
    if(texColor.r < 0.1) discard;
    vec3 fireColor = texColor.rgb;
    fireColor.r = 1.0;
    fireColor.g = 0.2;
    fireColor.b = 0.0;

    
    float flicker = 0.7 + 0.3 * sin(time * 10.0 + gl_FragCoord.y * 0.2) * 
                   (0.8 + 0.2 * sin(time * 7.0 + gl_FragCoord.x * 0.15));
    
    fragColor = vec4(fireColor * flicker, alpha);
    if(alpha < 0.01) discard;
}
)";
#endif        
        if (!fireShader.loadProgramFromText(vertexShader, fragmentShader)) {
            throw mx::Exception("Failed to load fire shaders");
        }
        
        particles.resize(MAX_PARTICLES);
        resetParticles();
        
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, particles.size() * sizeof(FireParticle), 
                    particles.data(), GL_DYNAMIC_DRAW);
    
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(FireParticle), (void*)0);
        glEnableVertexAttribArray(0);
        
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(FireParticle), 
                            (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(FireParticle), 
                            (void*)(5 * sizeof(float)));
        glEnableVertexAttribArray(2);
        
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(FireParticle), 
                            (void*)(8 * sizeof(float)));
        glEnableVertexAttribArray(3);
        
        glBindVertexArray(0);
        CHECK_GL_ERROR();
    }
    
    void resetParticles() {
        for (auto& p : particles) {
            resetParticle(p, true);
        }
    }
    
    void resetParticle(FireParticle& p, bool initialSetup = false) {
        float baseWidth = 1.2f;  
        float baseDepth = 0.5f;  
        
        p.x = ((float)(rand() % 100) / 100.0f - 0.5f) * baseWidth;
        p.y = -0.1f + ((float)(rand() % 20) / 100.0f - 0.1f); 
        p.z = ((float)(rand() % 100) / 100.0f - 0.5f) * baseDepth;
        
        int particleType = rand() % 100;
        
        if (particleType < 75) {  
            p.speed = 0.8f + ((float)(rand() % 100) / 100.0f) * 1.0f;
            p.life = 0.8f + ((float)(rand() % 40) / 100.0f);
            p.fade = 0.25f + ((float)(rand() % 100) / 100.0f) * 0.2f;
            
            p.r = 1.0f;
            p.g = 0.0f;
            p.b = 0.0f;

        
            p.r = 1.0f;
            p.g = 0.2f + ((float)(rand() % 100) / 100.0f) * 0.4f;  
            p.b = 0.0f + ((float)(rand() % 100) / 100.0f) * 0.1f;

            p.scale = 0.25f + ((float)(rand() % 100) / 100.0f) * 0.25f;
            p.gravity = 0.02f + ((float)(rand() % 100) / 100.0f) * 0.03f;
        } 
        else if (particleType < 95) {  
            
            p.speed = 1.2f + ((float)(rand() % 100) / 100.0f) * 1.5f;
            p.life = 0.6f + ((float)(rand() % 40) / 100.0f);
            p.fade = 0.4f + ((float)(rand() % 100) / 100.0f) * 0.3f;
            
            
            p.r = 1.0f;
            p.g = 0.7f + ((float)(rand() % 30) / 100.0f);
            p.b = 0.2f + ((float)(rand() % 30) / 100.0f);
            
            
            p.scale = 0.09f + ((float)(rand() % 100) / 100.0f) * 0.09f;
            p.gravity = 0.01f; 
        }
        else {  
            
            p.speed = 1.5f + ((float)(rand() % 100) / 100.0f) * 1.8f;
            p.life = 1.0f;
            p.fade = 0.2f + ((float)(rand() % 100) / 100.0f) * 0.15f;
            
            
            p.r = 1.0f;
            p.g = 0.5f + ((float)(rand() % 50) / 100.0f);
            p.b = 0.05f + ((float)(rand() % 20) / 100.0f);
            
            
            p.scale = 0.35f + ((float)(rand() % 100) / 100.0f) * 0.35f;
            p.gravity = 0.01f + ((float)(rand() % 100) / 100.0f) * 0.02f;
        }
        
        p.wind = ((float)(rand() % 100) / 100.0f - 0.5f) * 0.2f;
        
        p.angle = ((float)(rand() % 100) / 100.0f) * 2 * M_PI;
        p.angleSpeed = ((float)(rand() % 100) / 100.0f - 0.5f) * 2.5f;
        p.angleSpeedVar = ((float)(rand() % 100) / 100.0f) * 0.2f;
        p.scaleSpeed = 0.03f + ((float)(rand() % 100) / 100.0f) * 0.07f;
        p.scaleSpeedVar = ((float)(rand() % 100) / 100.0f) * 0.02f;
        p.speedVar = ((float)(rand() % 100) / 100.0f) * 0.2f;
        p.gravityVar = ((float)(rand() % 100) / 100.0f) * 0.05f;
        p.windVar = ((float)(rand() % 100) / 100.0f - 0.5f) * 0.1f;
        p.rVar = ((float)(rand() % 100) / 100.0f) * 0.15f;
    }

    void update(float deltaTime) {
        windTime += deltaTime;
        
        float windEffect = std::sin(windTime * 0.5f) * 0.2f + 
                          std::sin(windTime * 1.2f) * 0.1f;
        
        if (rand() % 200 == 0) {
            windTime = 0;  
        }
        
        int newParticlesPerFrame = 10000;  
        for (int i = 0; i < newParticlesPerFrame; i++) {
            int idx = rand() % particles.size();
            if (particles[idx].life < 0.3f) { 
                resetParticle(particles[idx]);
            }
        }
        
        for (auto& p : particles) {
            p.y += (p.speed + p.speedVar) * deltaTime;
            p.x += (p.wind + p.windVar + windEffect) * deltaTime;
            p.z += p.windVar * deltaTime;
            p.speed -= (p.gravity + p.gravityVar) * deltaTime;
            p.angle += (p.angleSpeed + p.angleSpeedVar) * deltaTime;
            p.scale += (p.scaleSpeed + p.scaleSpeedVar) * deltaTime;
            
            if (p.g < 0.6f) {  
                p.g = std::min(p.g + 0.15f * deltaTime * (1.0f - p.y), 0.6f); 
            }
            p.r = std::max(p.r - p.rVar * deltaTime, 0.5f);
            p.life -= p.fade * deltaTime;
        
            if (p.life <= 0.0f || p.y > 2.5f) {
                resetParticle(p);
            }
        }
        
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, particles.size() * sizeof(FireParticle), 
                       particles.data());
    }
    void draw(gl::GLWindow *win) {
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 
                                              static_cast<float>(win->w) / static_cast<float>(win->h), 
                                              0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(
            glm::vec3(0.0f, 0.5f, cameraDistance),   
            glm::vec3(0.0f, 0.3f, 0.0f),            
            glm::vec3(0.0f, 1.0f, 0.0f)             
        );
        
        glm::mat4 viewProj = projection * view;

        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE); 
    #ifndef __EMSCRIPTEN__
        glEnable(GL_PROGRAM_POINT_SIZE);
    #endif
        glDepthMask(GL_FALSE); 
        
        fireShader.useProgram();
        
        GLint viewProjLoc = glGetUniformLocation(fireShader.id(), "viewProj");
        glUniformMatrix4fv(viewProjLoc, 1, GL_FALSE, glm::value_ptr(viewProj));
        
        GLint texLoc = glGetUniformLocation(fireShader.id(), "fireTexture");
        glUniform1i(texLoc, 0);
        
        GLint timeLoc = glGetUniformLocation(fireShader.id(), "time");
        glUniform1f(timeLoc, SDL_GetTicks() / 1000.0f);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        
        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, particles.size());
        glBindVertexArray(0);
        
    #ifndef __EMSCRIPTEN__
        glDisable(GL_PROGRAM_POINT_SIZE);
    #endif
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE); 
        CHECK_GL_ERROR();
    }

    void releaseResources() {
        if (vbo != 0) {
            glDeleteBuffers(1, &vbo);
            vbo = 0;
        }
        
        if (vao != 0) {
            glDeleteVertexArrays(1, &vao);
            vao = 0;
        }
        
        if (textureID != 0) {
            glDeleteTextures(1, &textureID);
            textureID = 0;
        }
    }

    void setCameraDistance(float distance) {
        cameraDistance = distance;
    }

    private:
        std::vector<FireParticle> particles;
        GLuint vao = 0;
        GLuint vbo = 0;
        GLuint textureID = 0;
        gl::ShaderProgram fireShader;
        float windTime = 0.0f;
        float cameraDistance = 4.0f; 
};



class Game : public gl::GLObject {
public:
    FireEmiter emiter;
    float cameraZoom = 1.0f;  
    float zoomSpeed = 0.5f;   
    Game() = default;
    virtual ~Game() override {}

    void load(gl::GLWindow *win) override {
        font.loadFont(win->util.getFilePath("data/font.ttf"), 36);
        emiter.load(win);
    }

    void draw(gl::GLWindow *win) override {
        glDisable(GL_DEPTH_TEST);
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; 
        lastUpdateTime = currentTime;
        win->text.setColor({255, 255, 255, 255});
        win->text.printText_Solid(font, 25.0f, 25.0f, "Fire Demo - Zoom: " + std::to_string(cameraZoom));
        update(deltaTime);
        
        emiter.setCameraDistance(cameraZoom);
        emiter.draw(win);
    }
    
    void event(gl::GLWindow *win, SDL_Event &e) override {
        if (e.type == SDL_KEYDOWN) {
            switch(e.key.keysym.sym) {
                case SDLK_UP: 
                    cameraZoom -= zoomSpeed;
                    if (cameraZoom < 1.0f) cameraZoom = 1.0f; 
                    break;
                case SDLK_DOWN: 
                    cameraZoom += zoomSpeed;
                    if (cameraZoom > 10.0f) cameraZoom = 10.0f; 
                    break;
                default:
                    break;
            }
        }
    }
    
    void update(float deltaTime) {
        emiter.update(deltaTime);
    }
private:
    mx::Font font;
    Uint32 lastUpdateTime = SDL_GetTicks();
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("Skeleton", tw, th) {
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
    MainWindow main_window("/", 1920, 1080);
    main_w =&main_window;
    emscripten_set_main_loop(eventProc, 0, 1);
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
