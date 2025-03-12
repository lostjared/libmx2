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


struct WaterParticle {
    float x, y, z;
    float vx, vy, vz;
    float life;
    float size;
    float angle;
    float speed;
    float gravity;
    float friction;
    float rotation;
    float rotationSpeed;
    float color[4];
};

class WaterEmiter {
public:
    static const int MAX_PARTICLES = 30000;
    
    WaterEmiter() = default;
    ~WaterEmiter() {
        releaseResources();
    }

    void load(gl::GLWindow *win) {
        SDL_Surface* waterSurface = png::LoadPNG(win->util.getFilePath("data/water_droplet.png").c_str());
        if(!waterSurface) {
            throw mx::Exception("Failed to load water droplet texture");
        }

        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);        

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, waterSurface->w, waterSurface->h, 
                    0, GL_RGBA, GL_UNSIGNED_BYTE, waterSurface->pixels);
        
        SDL_FreeSurface(waterSurface);

#ifdef __EMSCRIPTEN__
        const char *vertexShader = R"(#version 300 es
precision highp float;
layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in float size;
layout(location = 3) in float life;
out vec4 particleColor;
out float particleLife;
uniform mat4 viewProj;
uniform float time;

void main() {
    vec3 pos = position;
    gl_Position = viewProj * vec4(pos, 1.0);
    float particleScale = size * (0.9 + sin(time * 5.0 + position.y * 10.0) * 0.1);
    float distanceToCamera = gl_Position.w;
    gl_PointSize = particleScale * 30.0 / distanceToCamera;
    particleColor = color;
    particleLife = life;
}
)";
        
        const char *fragmentShader = R"(#version 300 es
precision highp float;
in vec4 particleColor;
in float particleLife;
out vec4 fragColor;
uniform sampler2D waterTexture;
uniform float time;

void main() {
    vec2 texCoord = gl_PointCoord;
    vec4 texColor = texture(waterTexture, texCoord);
    if(texColor.r < 0.1 && texColor.g < 0.1 && texColor.b < 0.1) discard;
    float alpha = texColor.a * particleLife * 0.8;
    vec3 waterColor = vec3(0.6, 0.8, 1.0) * texColor.rgb;
    float highlight = pow(texCoord.y, 3.0) * 0.5;
    waterColor += highlight * vec3(1.0);
    float ripple = sin(time * 8.0 + gl_FragCoord.y * 0.2) * 0.05 + 0.95;
    fragColor = vec4(waterColor * ripple * particleColor.rgb, alpha);
    if(alpha < 0.01) discard;
}
)";
#else
        const char *vertexShader = R"(#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in float size;
layout(location = 3) in float life;
out vec4 particleColor;
out float particleLife;
uniform mat4 viewProj;
uniform float time;

void main() {
    vec3 pos = position;
    gl_Position = viewProj * vec4(pos, 1.0);
    float particleScale = size * (0.9 + sin(time * 5.0 + position.y * 10.0) * 0.1);
    float distanceToCamera = gl_Position.w;
    gl_PointSize = particleScale * 30.0 / distanceToCamera;
    particleColor = color;
    particleLife = life;
}
)";
        
        const char *fragmentShader = R"(#version 330 core
in vec4 particleColor;
in float particleLife;
out vec4 fragColor;
uniform sampler2D waterTexture;
uniform float time;

void main() {
    vec2 texCoord = gl_PointCoord;
    vec4 texColor = texture(waterTexture, texCoord);
    if(texColor.r < 0.1 && texColor.g < 0.1 && texColor.b < 0.1) discard;
    float alpha = texColor.a * particleLife * 0.8;
    vec3 waterColor = vec3(0.6, 0.8, 1.0) * texColor.rgb;
    float highlight = pow(texCoord.y, 3.0) * 0.5;
    waterColor += highlight * vec3(1.0);
    float ripple = sin(time * 8.0 + gl_FragCoord.y * 0.2) * 0.05 + 0.95;
    fragColor = vec4(waterColor * ripple * particleColor.rgb, alpha);
    if(alpha < 0.01) discard;
}
)";
#endif

        if (!waterShader.loadProgramFromText(vertexShader, fragmentShader)) {
            throw mx::Exception("Failed to load water shaders");
        }

        particles.resize(MAX_PARTICLES);
        resetParticles();
        
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, particles.size() * sizeof(WaterParticle), 
                   particles.data(), GL_DYNAMIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(WaterParticle), (void*)0);
        glEnableVertexAttribArray(0);
        
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(WaterParticle), 
                            (void*)(offsetof(WaterParticle, color)));
        glEnableVertexAttribArray(1);
        
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(WaterParticle), 
                            (void*)(offsetof(WaterParticle, size)));
        glEnableVertexAttribArray(2);
        
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(WaterParticle), 
                            (void*)(offsetof(WaterParticle, life)));
        glEnableVertexAttribArray(3);
        
        glBindVertexArray(0);

        CHECK_GL_ERROR();
    }

    void resetParticles() {
        for (auto& p : particles) {
            resetParticle(p, true);
        }
    }

    void resetParticle(WaterParticle& p, bool initialSetup = false) {
        p.x = ((float)(rand() % 40) / 100.0f - 0.2f);
        p.y = -0.2f;  
        p.z = ((float)(rand() % 40) / 100.0f - 0.2f);
        
        float angle = ((float)(rand() % 100) / 100.0f) * 2.0f * M_PI;
        float spread = ((float)(rand() % 30) / 100.0f + 0.05f);
        
        p.vx = cos(angle) * spread;
        p.vy = 1.0f + ((float)(rand() % 50) / 100.0f) * 0.8f; 
        p.vz = sin(angle) * spread;
        if (initialSetup) {
            p.life = ((float)(rand() % 100) / 100.0f);
        } else {
            p.life = 1.0f;
        }
        p.size = 0.08f + ((float)(rand() % 60) / 1000.0f);
        p.gravity = 1.2f + ((float)(rand() % 40) / 100.0f);
        p.friction = 0.02f + ((float)(rand() % 2) / 100.0f);
        p.angle = ((float)(rand() % 100) / 100.0f) * 2.0f * M_PI;
        p.rotationSpeed = ((float)(rand() % 100) / 100.0f - 0.5f) * 2.0f;
        float blueVariation = ((float)(rand() % 30) / 100.0f);
        p.color[0] = 0.7f + blueVariation; 
        p.color[1] = 0.8f + blueVariation; 
        p.color[2] = 1.0f;        
        p.color[3] = 0.8f;        
    }

    void update(float deltaTime) {
        int particlesPerFrame = 20;
        for (int i = 0; i < particlesPerFrame; i++) {
            int idx = rand() % particles.size();
            if (particles[idx].life <= 0.1f || particles[idx].y < -1.0f) {
                resetParticle(particles[idx]);
            }
        }
        
        for (auto& p : particles) {
            p.vy -= p.gravity * deltaTime;  
            p.vx *= (1.0f - p.friction);
            p.vy *= (1.0f - p.friction);
            p.vz *= (1.0f - p.friction);
            
            p.x += p.vx * deltaTime;
            p.y += p.vy * deltaTime;
            p.z += p.vz * deltaTime;
            
        
            p.angle += p.rotationSpeed * deltaTime;
            
        
            p.life -= 0.5f * deltaTime;
            
        
            if (p.life <= 0.0f || p.y < -1.0f) {
                resetParticle(p);
            }
        }
        
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, particles.size() * sizeof(WaterParticle), 
                       particles.data());
    }

    void draw(gl::GLWindow *win) {
        float camX = sin(cameraRotation) * cameraDistance;
        float camZ = cos(cameraRotation) * cameraDistance;
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 
                                             (float)win->w / (float)win->h, 
                                             0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(
            glm::vec3(camX, 0.3f, camZ),     
            glm::vec3(0.0f, 0.0f, 0.0f),     
            glm::vec3(0.0f, 1.0f, 0.0f)      
        );
        
        glm::mat4 viewProj = projection * view;
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#ifndef __EMSCRIPTEN__
        glEnable(GL_PROGRAM_POINT_SIZE);
#endif
        glDepthMask(GL_FALSE); 
        
        waterShader.useProgram();
        
        GLint viewProjLoc = glGetUniformLocation(waterShader.id(), "viewProj");
        glUniformMatrix4fv(viewProjLoc, 1, GL_FALSE, glm::value_ptr(viewProj));
        
        GLint texLoc = glGetUniformLocation(waterShader.id(), "waterTexture");
        glUniform1i(texLoc, 0);
        
        GLint timeLoc = glGetUniformLocation(waterShader.id(), "time");
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

    void setCameraRotation(float angle) {
        cameraRotation = angle;
    }

private:
    std::vector<WaterParticle> particles;
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint textureID = 0;
    gl::ShaderProgram waterShader;
    float cameraDistance = 3.0f;
    float cameraRotation = 0.0f;
};


class Game : public gl::GLObject {
    WaterEmiter emiter;
    float cameraZoom = 3.0f;       
    float zoomSpeed = 0.2f;        
    float cameraRotation = 0.0f;   
    float rotationSpeed = 0.1f;    
    
public:
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
        win->text.printText_Solid(font, 25.0f, 25.0f, 
            "Water Demo - Zoom: " + std::to_string(cameraZoom) + 
            " Rotation: " + std::to_string(int(cameraRotation * 180.0f / M_PI) % 360) + " degrees");
        
        update(deltaTime);
        emiter.setCameraDistance(cameraZoom);  
        emiter.setCameraRotation(cameraRotation); 
        emiter.draw(win);
    }
    
    void event(gl::GLWindow *win, SDL_Event &e) override {
        if (e.type == SDL_KEYDOWN) {
            switch(e.key.keysym.sym) {
                case SDLK_UP:    
                    cameraZoom -= zoomSpeed;
                    if (cameraZoom < 0.5f) cameraZoom = 0.5f;
                    break;
                case SDLK_DOWN:  
                    cameraZoom += zoomSpeed;
                    if (cameraZoom > 10.0f) cameraZoom = 10.0f;
                    break;
                case SDLK_LEFT:  
                    cameraRotation += rotationSpeed;
                    if (cameraRotation > 2.0f * M_PI) 
                        cameraRotation -= 2.0f * M_PI;  
                    break;
                case SDLK_RIGHT: 
                    cameraRotation -= rotationSpeed;
                    if (cameraRotation < 0.0f) 
                        cameraRotation += 2.0f * M_PI;  
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
    MainWindow main_window("/", 960, 720);
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
