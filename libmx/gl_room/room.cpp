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

class Floor {
public:
    Floor() = default;
    ~Floor() {
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
    }

    void load(gl::GLWindow *win) {
#ifndef __EMSRIPTEN__
        const char *vertexShader = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos;
            layout(location = 1) in vec2 aTexCoord;
            
            out vec2 TexCoord;
            
            uniform mat4 model;
            uniform mat4 view;
            uniform mat4 projection;
            
            void main() {
                gl_Position = projection * view * model * vec4(aPos, 1.0);
                TexCoord = aTexCoord;
            }
        )";

        const char *fragmentShader = R"(
            #version 330 core
            out vec4 FragColor;
            
            in vec2 TexCoord;
            
            uniform sampler2D floorTexture;
            
            void main() {
                FragColor = texture(floorTexture, TexCoord);
            }
        )";
#else
const char *vertexShader = R"(#version 300 es
    precision highp float;
    layout(location = 0) in vec3 aPos;
    layout(location = 1) in vec2 aTexCoord;
    out vec2 TexCoord;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    void main() {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        TexCoord = aTexCoord;
    }
)";

const char *fragmentShader = R"(#version 300 es
    precision highp float;
    out vec4 FragColor;
    in vec2 TexCoord;
    uniform sampler2D floorTexture;
    void main() {
        FragColor = texture(floorTexture, TexCoord);
    }
)";
#endif
        if(floorShader.loadProgramFromText(vertexShader, fragmentShader) == false) {
            throw mx::Exception("Failed to load floor shader program");
        }

        float vertices[] = {
            -50.0f, 0.0f, -50.0f,  0.0f,  0.0f,
             50.0f, 0.0f, -50.0f, 25.0f,  0.0f,
             50.0f, 0.0f,  50.0f, 25.0f, 25.0f,
            -50.0f, 0.0f,  50.0f,  0.0f, 25.0f
        };
        
        unsigned int indices[] = {
            0, 1, 2,
            2, 3, 0
        };

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);
        
        glBindVertexArray(vao);
        
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        SDL_Surface* floorSurface = png::LoadPNG(win->util.getFilePath("data/ground.png").c_str());
        if(!floorSurface) {
            throw mx::Exception("Failed to load floor texture");
        }

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, floorSurface->w, floorSurface->h,
                    0, GL_RGBA, GL_UNSIGNED_BYTE, floorSurface->pixels);
        
        SDL_FreeSurface(floorSurface);
    }

    void update(float deltaTime) {
    }

    void draw(gl::GLWindow *win) {
        glm::mat4 model = glm::mat4(1.0f);
        
        glm::mat4 view = glm::lookAt(
            glm::vec3(cameraPos.x, cameraPos.y, cameraPos.z),
            glm::vec3(cameraPos.x + cameraFront.x, cameraPos.y + cameraFront.y, cameraPos.z + cameraFront.z),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 
                                            static_cast<float>(win->w) / static_cast<float>(win->h),
                                            0.1f, 100.0f);
        
        floorShader.useProgram();
        
        GLuint modelLoc = glGetUniformLocation(floorShader.id(), "model");
        GLuint viewLoc = glGetUniformLocation(floorShader.id(), "view");
        GLuint projectionLoc = glGetUniformLocation(floorShader.id(), "projection");
        
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glUniform1i(glGetUniformLocation(floorShader.id(), "floorTexture"), 0);
        
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    void setCameraPosition(const glm::vec3& pos) { cameraPos = pos; }
    void setCameraFront(const glm::vec3& front) { cameraFront = front; }
    glm::vec3 getCameraPosition() const { return cameraPos; }
    glm::vec3 getCameraFront() const { return cameraFront; }

private:
    gl::ShaderProgram floorShader;
    GLuint vao = 0, vbo = 0, ebo = 0;
    GLuint textureId = 0;
    glm::vec3 cameraPos = glm::vec3(0.0f, 1.7f, 3.0f); 
    glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f); 
};


class Game : public gl::GLObject {
public:
    Game() = default;
    virtual ~Game() override {}

    void load(gl::GLWindow *win) override {
        font.loadFont(win->util.getFilePath("data/font.ttf"), 36);
        game_floor.load(win);
        
        SDL_SetRelativeMouseMode(SDL_TRUE);
        lastX = win->w / 2.0f;
        lastY = win->h / 2.0f;
        
        
        yaw = -90.0f; 
        pitch = 0.0f;
        
        updateCameraVectors();
    }
    
    void updateCameraVectors() {
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        
        glm::vec3 cameraFront = glm::normalize(front);
        game_floor.setCameraFront(cameraFront);
    }

    void draw(gl::GLWindow *win) override {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f); 
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glEnable(GL_DEPTH_TEST); 
        
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; 
        lastUpdateTime = currentTime;
        
        update(deltaTime);
        game_floor.draw(win);
        
        win->text.setColor({255, 255, 255, 255});
        win->text.printText_Solid(font, 25.0f, 25.0f, 
                               "3D Room - WASD to move, Mouse to look around");
        
        if (showFPS) {
            float fps = 1.0f / deltaTime;
            win->text.printText_Solid(font, 25.0f, 65.0f, 
                                  "FPS: " + std::to_string(static_cast<int>(fps)));
        }
    }
    
    void event(gl::GLWindow *win, SDL_Event &e) override {
        const float cameraSpeed = 0.1f;
        
        if (e.type == SDL_KEYDOWN) {
            glm::vec3 cameraPos = game_floor.getCameraPosition();
            glm::vec3 cameraFront = game_floor.getCameraFront();
            glm::vec3 cameraRight = glm::normalize(glm::cross(cameraFront, glm::vec3(0.0f, 1.0f, 0.0f)));
            
            switch (e.key.keysym.sym) {
                case SDLK_w:
                    cameraPos += cameraSpeed * cameraFront;
                    break;
                case SDLK_s:
                    cameraPos -= cameraSpeed * cameraFront;
                    break;
                case SDLK_a:
                    cameraPos -= cameraSpeed * cameraRight;
                    break;
                case SDLK_d:
                    cameraPos += cameraSpeed * cameraRight;
                    break;
                case SDLK_ESCAPE:
                    toggleMouseCapture();
                    break;
                case SDLK_f: 
                    showFPS = !showFPS;
                    break;
            }
            
            game_floor.setCameraPosition(cameraPos);
        }
        
        if (e.type == SDL_MOUSEMOTION && mouseCapture) {
            float xoffset = e.motion.xrel * mouseSensitivity;
            float yoffset = -e.motion.yrel * mouseSensitivity; 
            
            yaw += xoffset;
            pitch += yoffset;
            
            if (pitch > 89.0f)
                pitch = 89.0f;
            if (pitch < -89.0f)
                pitch = -89.0f;
                
            updateCameraVectors();
        }
    }
    
    void toggleMouseCapture() {
        mouseCapture = !mouseCapture;
        SDL_SetRelativeMouseMode(mouseCapture ? SDL_TRUE : SDL_FALSE);
    }
    
    void update(float deltaTime) {
        this->deltaTime = deltaTime; 
        game_floor.update(deltaTime);
    }
    
private:
    mx::Font font;
    Uint32 lastUpdateTime = SDL_GetTicks();
    Floor game_floor;
    float lastX = 0.0f, lastY = 0.0f;
    float yaw = -90.0f; 
    float pitch = 0.0f;
    bool firstMouse = true;
    bool mouseCapture = true;
    float mouseSensitivity = 0.15f;
    bool showFPS = true;
    float deltaTime = 0.0f;
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("Room", tw, th) {
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
