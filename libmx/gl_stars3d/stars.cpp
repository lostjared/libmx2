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
    struct Star {
        float x, y, z;   
        float vx, vy, vz; 
        float magnitude;     
        float temperature;   
        float twinkle;
        float size;
        int starType;        
        bool isConstellation; 
    };

    static constexpr int NUM_STARS = 60000;  
    gl::ShaderProgram program;
    GLuint VAO, VBO[3];
    GLuint texture;
    std::vector<Star> stars;
    Uint32 lastUpdateTime = 0;
    
    float cameraX = 0.0f, cameraY = 0.0f, cameraZ = 0.0f;
    float cameraYaw = 0.0f, cameraPitch = 0.0f;
    float cameraSpeed = 10.0f;
    
    float atmosphericTwinkle = 1.0f;
    float lightPollution = 0.1f;  

    StarField() : stars(NUM_STARS) {}

    ~StarField() override {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(3, VBO);
        glDeleteTextures(1, &texture);
    }

    void load(gl::GLWindow *win) override {
        if(!program.loadProgramFromText(vertSource, fragSource)) {
            throw mx::Exception("Error loading shader");
        }

        for (int i = 0; i < NUM_STARS; ++i) {
            auto& star = stars[i];
            
            float theta = generateRandomFloat(0.0f, 2.0f * M_PI);
            float phi = acos(generateRandomFloat(-1.0f, 1.0f));
            float radius = generateRandomFloat(50.0f, 200.0f);
            
            star.x = radius * sin(phi) * cos(theta);
            star.y = radius * sin(phi) * sin(theta);
            star.z = radius * cos(phi);
            
            star.vx = generateRandomFloat(-0.001f, 0.001f);
            star.vy = generateRandomFloat(-0.001f, 0.001f);
            star.vz = generateRandomFloat(-0.001f, 0.001f);
            
            float rand = generateRandomFloat(0.0f, 1.0f);
            if (rand < 0.05f) {
                star.magnitude = generateRandomFloat(-1.0f, 2.0f);
                star.starType = 1; 
            } else if (rand < 0.3f) {
                star.magnitude = generateRandomFloat(2.0f, 4.0f);
                star.starType = 0; 
            } else {
                star.magnitude = generateRandomFloat(4.0f, 6.5f);
                star.starType = 2; 
            }
            
            if (star.starType == 1) {
                star.temperature = generateRandomFloat(3000.0f, 5000.0f);
            } else if (star.starType == 0) {
                star.temperature = generateRandomFloat(4000.0f, 8000.0f);
            } else {
                star.temperature = generateRandomFloat(2500.0f, 4000.0f);
            }
            
            star.twinkle = generateRandomFloat(0.5f, 3.0f);
            star.size = magnitudeToSize(star.magnitude);
            
            star.isConstellation = (star.magnitude < 3.0f && generateRandomFloat(0.0f, 1.0f) < 0.3f);
        }
        
        glGenVertexArrays(1, &VAO);
        glGenBuffers(3, VBO);
        glBindVertexArray(VAO);
        
        glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
        glBufferData(GL_ARRAY_BUFFER, NUM_STARS * 3 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        
        glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
        glBufferData(GL_ARRAY_BUFFER, NUM_STARS * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, (void*)0);
        
        glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
        glBufferData(GL_ARRAY_BUFFER, NUM_STARS * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
        
        texture = gl::loadTexture(win->util.getFilePath("data/star.png"));
        lastUpdateTime = SDL_GetTicks();
    }

    void event(gl::GLWindow *win, SDL_Event &e) override {

    }
    
    void draw(gl::GLWindow *win) override {
#ifndef __EMSCRIPTEN__
        glEnable(GL_PROGRAM_POINT_SIZE);
#endif
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; 
        lastUpdateTime = currentTime;

        update(deltaTime);

        program.useProgram();

        glm::mat4 projection = glm::perspective(
            glm::radians(60.0f),  
            (float)win->w / (float)win->h, 
            0.1f, 
            1000.0f
        );

        glm::vec3 front;
        front.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
        front.y = sin(glm::radians(cameraPitch));
        front.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
        front = glm::normalize(front);
        
        glm::vec3 cameraPos(cameraX, cameraY, cameraZ);
        glm::vec3 cameraTarget = cameraPos + front;
        glm::vec3 up(0.0f, 1.0f, 0.0f);
        
        glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, up);
        glm::mat4 model = glm::mat4(1.0f);  
        glm::mat4 MVP = projection * view * model;
        
        program.setUniform("MVP", MVP);
        program.setUniform("spriteTexture", 0);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindVertexArray(VAO);
        glDrawArrays(GL_POINTS, 0, NUM_STARS);
    }

    void update(float deltaTime) {
        if(deltaTime > 0.1f) 
            deltaTime = 0.1f;

        std::vector<float> positions;
        std::vector<float> sizes;
        std::vector<float> colors;
        positions.reserve(NUM_STARS * 3);
        sizes.reserve(NUM_STARS);
        colors.reserve(NUM_STARS * 4);

        float time = SDL_GetTicks() * 0.001f;

        for (auto& star : stars) {
            star.x += star.vx * deltaTime;
            star.y += star.vy * deltaTime;
            star.z += star.vz * deltaTime;

            positions.push_back(star.x);
            positions.push_back(star.y);
            positions.push_back(star.z);

            float twinkleFactor = 1.0f;
            if (atmosphericTwinkle > 0.0f) {
                twinkleFactor = 0.7f + 0.3f * sin(time * star.twinkle) * atmosphericTwinkle;
            }

            float size = star.size * twinkleFactor;
            if (star.isConstellation) {
                size *= 1.2f; 
            }
            sizes.push_back(size);

            glm::vec3 starColor = getStarColor(star.temperature);
            float alpha = magnitudeToAlpha(star.magnitude) * twinkleFactor;

            colors.push_back(starColor.r);
            colors.push_back(starColor.g);
            colors.push_back(starColor.b);
            colors.push_back(alpha);
        }

        glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, positions.size() * sizeof(float), positions.data());

        glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizes.size() * sizeof(float), sizes.data());

        glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, colors.size() * sizeof(float), colors.data());
    }

    glm::vec3 getStarColor(float temperature) {
        float r, g, b;
        
        if (temperature < 3700) {
            r = 1.0f;
            g = temperature / 3700.0f * 0.6f;
            b = 0.0f;
        } else if (temperature < 5200) {
            r = 1.0f;
            g = 0.6f + (temperature - 3700) / 1500.0f * 0.4f;
            b = (temperature - 3700) / 1500.0f * 0.3f;
        } else if (temperature < 6000) {
            r = 1.0f;
            g = 1.0f;
            b = (temperature - 5200) / 800.0f * 0.7f;
        } else if (temperature < 7500) {
            r = 1.0f;
            g = 1.0f;
            b = 0.7f + (temperature - 6000) / 1500.0f * 0.3f;
        } else {
            r = 0.7f - (temperature - 7500) / 10000.0f * 0.4f;
            g = 0.8f + (temperature - 7500) / 10000.0f * 0.2f;
            b = 1.0f;
        }
        
        return glm::vec3(r, g, b);
    }

    float magnitudeToSize(float magnitude) {
        return glm::clamp(15.0f - magnitude * 2.0f, 1.0f, 25.0f);
    }

    float magnitudeToAlpha(float magnitude) {
        float alpha = (6.5f - magnitude) / 6.5f; 
        return glm::clamp(alpha - lightPollution, 0.0f, 1.0f);
    }
};

class Game : public gl::GLObject {
public:
    mx::Font font;
    StarField field; 
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
    }

    void event(gl::GLWindow* win, SDL_Event &e) override {
        const float mouseSensitivity = 0.1f;
    
        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_w: case SDLK_LEFT:
                    field.cameraZ -= field.cameraSpeed * 0.1f;
                    break;
                case SDLK_s: case SDLK_RIGHT:
                    field.cameraZ += field.cameraSpeed * 0.1f;
                    break;
                case SDLK_a: case SDLK_DOWN:
                    field.cameraX -= field.cameraSpeed * 0.1f;
                    field.cameraSpeed += 0.5;
                    break;
                case SDLK_d: case SDLK_UP:
                    field.cameraX += field.cameraSpeed * 0.1f;
                    field.cameraSpeed -= 0.5;
                    break;
                case SDLK_q:
                    field.cameraY += field.cameraSpeed * 0.1f;
                    break;
                case SDLK_e:
                    field.cameraY -= field.cameraSpeed * 0.1f;
                    break;
                case SDLK_1:
                    field.lightPollution = 0.0f;
                    field.atmosphericTwinkle = 0.3f;
                    break;
                case SDLK_2:
                    field.lightPollution = 0.3f;
                    field.atmosphericTwinkle = 0.7f;
                    break;
                case SDLK_3:
                    field.lightPollution = 0.7f;
                    field.atmosphericTwinkle = 1.0f;
                    break;
                case SDLK_z:
                    field.cameraSpeed += 0.1f;
                    break;
                case SDLK_x:
                    field.cameraSpeed -= 0.1f;
                    break;
            }
        }
    
        if (e.type == SDL_MOUSEMOTION && (e.motion.state & SDL_BUTTON_LMASK)) {
            field.cameraYaw += e.motion.xrel * mouseSensitivity;
            field.cameraPitch -= e.motion.yrel * mouseSensitivity;
        
            if (field.cameraPitch > 89.0f) field.cameraPitch = 89.0f;
            if (field.cameraPitch < -89.0f) field.cameraPitch = -89.0f;
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
    MainWindow main_window("/", 1920, 1080);
    main_w = &main_window;
    emscripten_set_main_loop(eventProc, 0, 1);

#else
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