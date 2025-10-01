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

const char* lineVertSource = R"(#version 330 core
layout (location = 0) in vec3 inPosition; 
layout (location = 2) in vec4 inColor;

uniform mat4 MVP;  
out vec4 fragColor;

void main() {
    gl_Position = MVP * vec4(inPosition, 1.0);
    fragColor = inColor;
}
)";

const char* lineFragSource = R"(#version 330 core
in vec4 fragColor;
out vec4 FragColor;

void main() {
    FragColor = fragColor;
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

const char* lineVertSource = R"(#version 300 es
precision highp float;
layout (location = 0) in vec3 inPosition; 
layout (location = 2) in vec4 inColor;

uniform mat4 MVP;
out vec4 fragColor;

void main() {
    gl_Position = MVP * vec4(inPosition, 1.0);
    fragColor = inColor;
}
)";

const char* lineFragSource = R"(#version 300 es
precision highp float;
in vec4 fragColor;
out vec4 FragColor;

void main() {
    FragColor = fragColor;
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

    static constexpr int NUM_STARS = 35000;  
    gl::ShaderProgram program;      
    gl::ShaderProgram lineProgram;  
    GLuint VAO, VBO[3];
    GLuint texture;
    std::vector<Star> stars;
    Uint32 lastUpdateTime = 0;
    
    float cameraX = 0.0f, cameraY = 0.0f, cameraZ = 0.0f;
    float cameraYaw = 0.0f, cameraPitch = 0.0f;
    float cameraSpeed = 10.0f;
    
    float rotationX = 0.0f;
    float rotationY = 0.0f;
    float rotationZ = 0.0f;
    float rotationSpeed = 1.0f;  
    
    float atmosphericTwinkle = 1.0f;
    float lightPollution = 0.1f;  
    GLuint lineVAO, lineVBO;
    std::vector<float> lineVertices;
    float connectionDistance = 25.0f;  
    float lineOpacity = 0.5f;          
    int maxConnections = 5;            
    
    StarField() : stars(NUM_STARS) {}

    ~StarField() override {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(3, VBO);
        glDeleteVertexArrays(1, &lineVAO);
        glDeleteBuffers(1, &lineVBO);
        glDeleteTextures(1, &texture);
    }

    void load(gl::GLWindow *win) override {
        if(!program.loadProgramFromText(vertSource, fragSource)) {
            throw mx::Exception("Error loading shader");
        }
        
        if(!lineProgram.loadProgramFromText(lineVertSource, lineFragSource)) {
            throw mx::Exception("Error loading line shader");
        }

        for (int i = 0; i < NUM_STARS; ++i) {
            Star star;
            
            float theta = generateRandomFloat(0.0f, 2.0f * M_PI);
            float phi = acos(generateRandomFloat(-1.0f, 1.0f));
            float radius = generateRandomFloat(50.0f, 200.0f);
            
            star.x = radius * sin(phi) * cos(theta);
            star.y = radius * sin(phi) * sin(theta);
            star.z = radius * cos(phi);
            
            star.vx = generateRandomFloat(-0.001f, 0.001f);
            star.vy = generateRandomFloat(-0.001f, 0.001f);
            star.vz = generateRandomFloat(-0.001f, 0.001f);
            
            star.magnitude = generateRandomFloat(-1.5f, 6.5f);
            star.temperature = generateRandomFloat(2000.0f, 40000.0f);
            star.size = magnitudeToSize(star.magnitude);
            star.twinkle = generateRandomFloat(0.5f, 2.0f);
            
            star.starType = 0;
            if (generateRandomFloat(0.0f, 1.0f) < 0.1f) {
                star.starType = 1; 
            } else if (generateRandomFloat(0.0f, 1.0f) < 0.05f) {
                star.starType = 2; 
            }
            
            star.isConstellation = (generateRandomFloat(0.0f, 1.0f) < 0.01f);
            
            stars[i] = star;
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
        
        glGenVertexArrays(1, &lineVAO);
        glGenBuffers(1, &lineVBO);
        
        glBindVertexArray(lineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
        glBufferData(GL_ARRAY_BUFFER, NUM_STARS * maxConnections * 14 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
        
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
        model = glm::rotate(model, glm::radians(rotationX), glm::vec3(1.0f, 0.0f, 0.0f)); 
        model = glm::rotate(model, glm::radians(rotationY), glm::vec3(0.0f, 1.0f, 0.0f)); 
        model = glm::rotate(model, glm::radians(rotationZ), glm::vec3(0.0f, 0.0f, 1.0f)); 
        
        glm::mat4 MVP = projection * view * model;
        
        if (lineVertices.size() > 0) {
            lineProgram.useProgram();
            lineProgram.setUniform("MVP", MVP);
            glBindVertexArray(lineVAO);
            glLineWidth(1.0f);  
            glDrawArrays(GL_LINES, 0, lineVertices.size() / 7);
        }
        
        program.useProgram();
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

        lineVertices.clear();
        
        const float gridSize = connectionDistance;
        std::unordered_map<int, std::vector<int>> grid;
        
        for (int i = 0; i < NUM_STARS; i++) {
            int gridX = static_cast<int>(stars[i].x / gridSize);
            int gridY = static_cast<int>(stars[i].y / gridSize);
            int gridZ = static_cast<int>(stars[i].z / gridSize);
            int key = (gridX * 73856093) ^ (gridY * 19349663) ^ (gridZ * 83492791);
            grid[key].push_back(i);
        }
        
        for (int i = 0; i < NUM_STARS; i++) {
            if (stars[i].magnitude > 5.0f) continue;
            
            int connections = 0;
            
            int gridX = static_cast<int>(stars[i].x / gridSize);
            int gridY = static_cast<int>(stars[i].y / gridSize);
            int gridZ = static_cast<int>(stars[i].z / gridSize);
            
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dz = -1; dz <= 1; dz++) {
                        int key = ((gridX + dx) * 73856093) ^ 
                                  ((gridY + dy) * 19349663) ^ 
                                  ((gridZ + dz) * 83492791);
                        
                        if (grid.find(key) == grid.end()) continue;
                        
                        for (int neighborIdx : grid[key]) {
                            if (neighborIdx <= i) continue;
                            
                            auto& neighbor = stars[neighborIdx];
                            if (neighbor.magnitude > 5.0f) continue;
                            
                            float dx = stars[i].x - neighbor.x;
                            float dy = stars[i].y - neighbor.y;
                            float dz = stars[i].z - neighbor.z;
                            float distSq = dx*dx + dy*dy + dz*dz;
                            
                            if (distSq < connectionDistance * connectionDistance) {
                                float distance = sqrt(distSq);
                                float opacity = lineOpacity * (1.0f - distance / connectionDistance);
                                
                                lineVertices.push_back(stars[i].x);
                                lineVertices.push_back(stars[i].y);
                                lineVertices.push_back(stars[i].z);
                                
                                glm::vec3 color1 = getStarColor(stars[i].temperature);
                                lineVertices.push_back(color1.r);
                                lineVertices.push_back(color1.g);
                                lineVertices.push_back(color1.b);
                                lineVertices.push_back(opacity);
                                
                                lineVertices.push_back(neighbor.x);
                                lineVertices.push_back(neighbor.y);
                                lineVertices.push_back(neighbor.z);
                                
                                glm::vec3 color2 = getStarColor(neighbor.temperature);
                                lineVertices.push_back(color2.r);
                                lineVertices.push_back(color2.g);
                                lineVertices.push_back(color2.b);
                                lineVertices.push_back(opacity);
                                
                                connections++;
                                if (connections >= maxConnections) break;
                            }
                        }
                        if (connections >= maxConnections) break;
                    }
                    if (connections >= maxConnections) break;
                }
                if (connections >= maxConnections) break;
            }
        }

        static int frameCount = 0;
        if (frameCount++ % 60 == 0) {
            printf("Generated %d line segments connecting stars\n", (int)lineVertices.size() / 14);
        }

        glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, positions.size() * sizeof(float), positions.data());

        glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizes.size() * sizeof(float), sizes.data());

        glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, colors.size() * sizeof(float), colors.data());
        
        glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
        if (lineVertices.size() > 0) {
            glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(float), lineVertices.data(), GL_DYNAMIC_DRAW);
        }
    }

    glm::vec3 getStarColor(float temperature) {
        float r, g, b;
        r = generateRandomFloat(0.1f, 1.0f);
        g = generateRandomFloat(0.1f, 1.0f);
        b = generateRandomFloat(0.1f, 1.0f);
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
                
                case SDLK_i:  
                    field.rotationX += field.rotationSpeed;
                    printf("Rotation X: %.1f\n", field.rotationX);
                    break;
                case SDLK_k:  
                    field.rotationX -= field.rotationSpeed;
                    printf("Rotation X: %.1f\n", field.rotationX);
                    break;
                case SDLK_j:  
                    field.rotationY += field.rotationSpeed;
                    printf("Rotation Y: %.1f\n", field.rotationY);
                    break;
                case SDLK_l:  
                    field.rotationY -= field.rotationSpeed;
                    printf("Rotation Y: %.1f\n", field.rotationY);
                    break;
                case SDLK_u:  
                    field.rotationZ += field.rotationSpeed;
                    printf("Rotation Z: %.1f\n", field.rotationZ);
                    break;
                case SDLK_o:  
                    field.rotationZ -= field.rotationSpeed;
                    printf("Rotation Z: %.1f\n", field.rotationZ);
                    break;
                case SDLK_p:  
                    field.rotationX = 0.0f;
                    field.rotationY = 0.0f;
                    field.rotationZ = 0.0f;
                    printf("Rotations reset\n");
                    break;
                case SDLK_LEFTBRACKET:  
                    field.rotationSpeed -= 0.5f;
                    if (field.rotationSpeed < 0.5f) field.rotationSpeed = 0.5f;
                    printf("Rotation speed: %.1f\n", field.rotationSpeed);
                    break;
                case SDLK_RIGHTBRACKET:  
                    field.rotationSpeed += 0.5f;
                    if (field.rotationSpeed > 10.0f) field.rotationSpeed = 10.0f;
                    printf("Rotation speed: %.1f\n", field.rotationSpeed);
                    break;
            
            case SDLK_r:
                field.connectionDistance += 1.0f;
                if (field.connectionDistance > 30.0f) field.connectionDistance = 30.0f;
                printf("Connection distance: %.1f\n", field.connectionDistance);
                break;
            case SDLK_f:
                field.connectionDistance -= 1.0f;
                if (field.connectionDistance < 1.0f) field.connectionDistance = 1.0f;
                printf("Connection distance: %.1f\n", field.connectionDistance);
                break;
                
            case SDLK_t:
                field.lineOpacity += 0.05f;
                if (field.lineOpacity > 1.0f) field.lineOpacity = 1.0f;
                printf("Line opacity: %.2f\n", field.lineOpacity);
                break;
            case SDLK_g:
                field.lineOpacity -= 0.05f;
                if (field.lineOpacity < 0.0f) field.lineOpacity = 0.0f;
                printf("Line opacity: %.2f\n", field.lineOpacity);
                break;
                
            case SDLK_y:
                field.maxConnections += 1;
                if (field.maxConnections > 10) field.maxConnections = 10;
                printf("Max connections: %d\n", field.maxConnections);
                break;
            case SDLK_h:
                field.maxConnections -= 1;
                if (field.maxConnections < 1) field.maxConnections = 1;
                printf("Max connections: %d\n", field.maxConnections);
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