#include "mx.hpp"
#include "gl.hpp"
#include "loadpng.hpp"
#include "explode.hpp"
#include <random>
#include <string>
#include <vector>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define CHECK_GL_ERROR()                                    \
{                                                           \
    GLenum err = glGetError();                              \
    if (err != GL_NO_ERROR) {                               \
        printf("OpenGL Error: %d at %s:%d\n", err, __FILE__, __LINE__); \
    }                                                       \
}

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

float generateRandomFloat(float min, float max) {
    static std::random_device rd; 
    static std::default_random_engine eng(rd()); 
    std::uniform_real_distribution<float> dist(min, max);
    return dist(eng);
}

glm::vec3 generateRandomPosition() {
    float x = generateRandomFloat(-3.0f, 3.0f); 
    float y = generateRandomFloat(-3.0f, 3.0f); 
    float z = 0.0f; 
    return glm::vec3(x, y, z);
}



class Game : public gl::GLObject {
public:
    Uint32 lastUpdateTime = SDL_GetTicks();
    mx::Font font;
    effect::ExplosionEmiter ex_emiter;
    GLuint texture = 0;
    glm::mat4 projection, view, model;
    Game()  {}

    ~Game() override {
        if(texture) {
            glDeleteTextures(1, &texture);
        }
    }

    void load(gl::GLWindow* win) override {
        font.loadFont("font.ttf", 24);
        ex_emiter.load(win);
    }

    void draw(gl::GLWindow* win) override {
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; // seconds
        lastUpdateTime = currentTime;
        if(deltaTime > 0.1)
            deltaTime = 0.1;
        update(win, deltaTime);
        win->text.setColor({255, 0, 0, 255});
        win->text.printText_Solid(font,25.0f, 25.0f,"Explosion - Press Space to Trigger");
        ex_emiter.draw(win);
    }

    void resize(gl::GLWindow *win, int w, int h) override {
        ex_emiter.resize(win, w,  h);
    }

    void event(gl::GLWindow* win, SDL_Event &e) override {
        if((e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_SPACE) || e.type == SDL_FINGERDOWN)
            ex_emiter.explode(win, generateRandomPosition());
    }

    void update(gl::GLWindow *win, float deltaTime) {
        ex_emiter.update(win, deltaTime);
    }
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(std::string path, int tw, int th) 
        : gl::GLWindow("Stars - [3D Particle Effect]", tw, th) {
        setPath(path);
        setObject(new Game());
        object->load(this);
    }

    ~MainWindow() override {}

    void event(SDL_Event &e) override {
        if (e.type == SDL_WINDOWEVENT) {
            if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                updateViewport();
            }
        }
    }

    void draw() override {
        SDL_GL_GetDrawableSize(window, &w, &h);        
        glClearColor(0.f, 0.f, 0.f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, w, h);
        object->draw(this);
        swap();
        delay();
    }
};


int main(int argc, char **argv) {
   try {
        MainWindow main_window(".", 800, 600);
        main_window.loop();
    } catch(const mx::Exception &e) {
        SDL_Log("MX EXception: %s\n", e.text().c_str());
    } 
    return 0;
}