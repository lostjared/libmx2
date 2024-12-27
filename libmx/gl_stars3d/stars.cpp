#include "mx.hpp"
#include "argz.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <GLES3/gl3.h>
#endif

#include "gl.hpp"
#include "loadpng.hpp"

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

class StarField : public gl::GLObject {
public:
    struct Particle {
        float x, y, z;   
        float vx, vy, vz; 
        float life;
        float twinkle;
    };

    static constexpr int NUM_PARTICLES = 5000;
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

class Game : public gl::GLObject {
public:
    mx::Font font;
    StarField field; //GLObjects inside of GLObjects
    Game()  {}

    ~Game() override {
    }

    void load(gl::GLWindow* win) override {
        font.loadFont(win->util.getFilePath("data/font.ttf"), 24);
        field.load(win);
    }

    void draw(gl::GLWindow* win) override {
        field.draw(win);
        win->text.setColor({255, 0, 0, 255});
        win->text.printText_Solid(font,25.0f, 25.0f,"3D Stars - Zoom: " + std::to_string(field.cameraZoom) + " Rotation: " + std::to_string(field.cameraRotation) + " Count: " + std::to_string(field.NUM_PARTICLES));
    }

    void event(gl::GLWindow* win, SDL_Event &e) override {
        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_UP:    
                    field.cameraZoom -= 0.1f;
                    if (field.cameraZoom < -10.0f) field.cameraZoom = -10.0f; 
                    break;
                case SDLK_DOWN:  
                    field.cameraZoom += 0.1f;
                    if (field.cameraZoom > 10.0f) field.cameraZoom = 10.0f; 
                    break;
                case SDLK_LEFT:  
                    field.cameraRotation -= 1.0f;
                    break;
                case SDLK_RIGHT: 
                    field.cameraRotation += 1.0f;
                    break;
            }
        }
    }

    void update(float deltaTime) {
        CHECK_GL_ERROR();
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
