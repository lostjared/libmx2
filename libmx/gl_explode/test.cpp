#include "mx.hpp"
#include "argz.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <GLES3/gl3.h>
#endif

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
    std::unique_ptr<effect::Explosion> explosion;
    GLuint texture = 0;
    glm::mat4 projection, view, model;
    Game()  {}

    ~Game() override {
        if(texture) {
            glDeleteTextures(1, &texture);
        }
    }

    void load(gl::GLWindow* win) override {
        font.loadFont(win->util.getFilePath("data/font.ttf"), 24);
        effect::load_program();
        explosion.reset(new effect::Explosion(1000));
        explosion->load(win);
        texture = gl::loadTexture(win->util.getFilePath("data/star.png"));
        explosion->setInfo(&effect::Explosion::shader_program, texture);
        float fov = glm::radians(45.0f); 
        float aspectRatio = static_cast<float>(win->w) / static_cast<float>(win->h);
        float nearPlane = 0.1f;
        float farPlane = 100.0f;
        projection = glm::perspective(fov, aspectRatio, nearPlane, farPlane);
        glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f, 5.0f);
        glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f); 
        glm::vec3 upVector    = glm::vec3(0.0f, 1.0f, 0.0f); 
        view = glm::lookAt(cameraPos, cameraTarget, upVector);
        model = glm::mat4(1.0f);
        effect::Explosion::shader_program.useProgram();
        effect::Explosion::shader_program.setUniform("projection", projection);
        effect::Explosion::shader_program.setUniform("view", view);
        effect::Explosion::shader_program.setUniform("model", model);
    }

    void draw(gl::GLWindow* win) override {
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; // seconds
        lastUpdateTime = currentTime;
        if(deltaTime > 0.1)
            deltaTime = 0.1;
        update(deltaTime);
        win->text.setColor({255, 0, 0, 255});
        win->text.printText_Solid(font,25.0f, 25.0f,"Explosion - Press Space to Trigger");
        if(explosion) {
            explosion->draw(win);
        }
    }

    void event(gl::GLWindow* win, SDL_Event &e) override {
        if(e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_SPACE)
            explosion->trigger(generateRandomPosition());
    }

    void update(float deltaTime) {
        explosion->update(deltaTime);
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
    }

    void draw() override {
        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
    // For WebAssembly build
    MainWindow main_window("/", 1920, 1080);
    main_w = &main_window;
    emscripten_set_main_loop(eventProc, 0, 1);

#else
    // Desktop build
    Argz<std::string> parser(argc, argv);
    parser.addOptionSingle('h', "Display help message")
          .addOptionSingleValue('p', "assets path")
          .addOptionDoubleValue('P', "path", "assets path")
          .addOptionSingleValue('r', "Resolution WidthxHeight")
          .addOptionDoubleValue('R', "resolution", "Resolution WidthxHeight");

    Argument<std::string> arg;
    std::string path;
    int value = 0;
    int tw = 1920, th = 1080;

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
                    std::string left  = arg.arg_value.substr(0, pos);
                    std::string right = arg.arg_value.substr(pos + 1);
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
        mx::system_out << "mx: No path provided, trying default current directory.\n";
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