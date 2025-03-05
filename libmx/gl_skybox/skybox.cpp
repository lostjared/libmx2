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


GLfloat vertices[] = { -1.0f, -1.0f, 1.0f, // front face
    1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    
    -1.0f, -1.0f, 1.0f,
    1.0f, -1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    
    -1.0f, -1.0f, -1.0f, // left side
    -1.0f, -1.0f, 1.0f,
    -1.0f, 1.0f, -1.0f,
    
    -1.0f, 1.0f, -1.0f,
    -1.0f, -1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    
    -1.0f, 1.0f, -1.0f, // top
    -1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, -1.0f,
    -1.0f, 1.0f, -1.0f,
    
    -1.0f, -1.0f, -1.0f, // bottom
    -1.0f, -1.0f, 1.0f,
    1.0f, -1.0f, 1.0f,
    
    1.0f, -1.0f, 1.0f,
    1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
    
    1.0f, -1.0f, -1.0f, // right
    1.0f, -1.0f, 1.0f,
    1.0f, 1.0f, -1.0f,
    
    1.0f, 1.0f, -1.0f,
    1.0f, -1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    
    -1.0f, -1.0f, -1.0f, // back face
    1.0f, 1.0f, -1.0f,
    -1.0f, 1.0f, -1.0f,
    
    -1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,
    1.0f, 1.0f, -1.0f,
    
};

class Cube : public gl::GLObject {
public:
    gl::ShaderProgram shaderProgram;
    GLuint VAO, VBO;
    GLuint texture;
    float yaw = 0.0f;
    float pitch = 0.0f;
    bool mouseControl = false;
    int lastMouseX = 0;
    int lastMouseY = 0;
    float mouseSensitivity = 0.15f;
    bool touchActive = false;
    float lastTouchX = 0.0f;
    float lastTouchY = 0.0f;
            
    Cube() = default;
    virtual ~Cube() override {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteTextures(1, &texture);
    }

    std::vector<GLfloat> cubeData;

    virtual void load(gl::GLWindow *win) override {     
        font.loadFont(win->util.getFilePath("data/font.ttf"), 24);
        size_t vertexCount = sizeof(vertices) / (3 * sizeof(GLfloat));      
        cubeData.resize(vertexCount * 3);   

        for (size_t i = 0; i < vertexCount; ++i) {
            cubeData[i * 3] = vertices[i * 3];         // Position X
            cubeData[i * 3 + 1] = vertices[i * 3 + 1]; // Position Y
            cubeData[i * 3 + 2] = vertices[i * 3 + 2]; // Position Z
        }

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, cubeData.size() * sizeof(GLfloat), cubeData.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
        if (!shaderProgram.loadProgram(win->util.getFilePath("data/cube.vert"), win->util.getFilePath("data/cube.frag"))) {
            throw mx::Exception("Failed to load shader program");
        }
        shaderProgram.useProgram();
        shaderProgram.setUniform("cubemapTexture", 0);
        glUniform2f(glGetUniformLocation(shaderProgram.id(), "iResolution"), win->w, win->h);
        glm::mat4 model = glm::mat4(1.0f); 
        glm::mat4 view = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 5.0f),  
            glm::vec3(0.0f, 0.0f, 0.0f),  
            glm::vec3(0.0f, 1.0f, 0.0f)   
        );
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)win->w / win->h, 0.1f, 100.0f);
        shaderProgram.setUniform("model", model);
        shaderProgram.setUniform("view", view);
        shaderProgram.setUniform("projection", projection);
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
        std::vector<std::string> faces = {
            win->util.getFilePath("data/cm_right.png"),
            win->util.getFilePath("data/cm_left.png"),
            win->util.getFilePath("data/cm_top.png"),
            win->util.getFilePath("data/cm_bottom.png"),
            win->util.getFilePath("data/cm_back.png"),
            win->util.getFilePath("data/cm_front.png")
        };

        for (unsigned int i = 0; i < faces.size(); i++) {
            SDL_Surface *surface = png::LoadPNG(faces[i].c_str());
            if (!surface) {
                mx::system_err << "Error: loading PNG " << faces[i] << "\n";
                exit(EXIT_FAILURE);
            }
            
            std::cout << "Loading texture " << i << ": " << faces[i] << "\n";
            std::cout << "  Size: " << surface->w << "x" << surface->h << "\n";
            std::cout << "  BytesPerPixel: " << (int)surface->format->BytesPerPixel << "\n";
            
            GLenum format = (surface->format->BytesPerPixel == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, 
                         surface->w, surface->h, 0, format, 
                         GL_UNSIGNED_BYTE, surface->pixels);
            CHECK_GL_ERROR();
            SDL_FreeSurface(surface);
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        CHECK_GL_ERROR();
    }

    virtual void draw(gl::GLWindow *win) override {
        glEnable(GL_DEPTH_TEST);
        static Uint32 lastTime = SDL_GetTicks();
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;
        
        static float time_f = 0.0f;
        time_f += deltaTime;

        const Uint8* keystate = SDL_GetKeyboardState(NULL);
        if (keystate[SDL_SCANCODE_LEFT])
            yaw -= 60.0f * deltaTime;
        if (keystate[SDL_SCANCODE_RIGHT])
            yaw += 60.0f * deltaTime;
        if (keystate[SDL_SCANCODE_DOWN])
            pitch += 60.0f * deltaTime;
        if (keystate[SDL_SCANCODE_UP])
            pitch -= 60.0f * deltaTime;
        
        pitch = glm::clamp(pitch, -89.0f, 89.0f);
        glm::mat4 view = glm::mat4(1.0f);
        view = glm::rotate(view, glm::radians(pitch), glm::vec3(1.0f, 0.0f, 0.0f));
        view = glm::rotate(view, glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 projection = glm::perspective(glm::radians(70.0f), 
                                              (float)win->w / win->h, 
                                              0.1f, 100.0f);
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shaderProgram.useProgram();
        shaderProgram.setUniform("model", glm::mat4(1.0f));
        shaderProgram.setUniform("view", view);
        shaderProgram.setUniform("projection", projection);
        shaderProgram.setUniform("time_f", time_f);
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, cubeData.size() / 3);
        CHECK_GL_ERROR();
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);
        win->text.setColor({255, 255, 255, 255});
        win->text.printText_Solid(font, 10, 10, "Skybox Demo - Arrow keys or mouse to look around");
        win->text.printText_Solid(font, 10, 40, "Press M to toggle mouse control");
        
        if (mouseControl) {
            win->text.printText_Solid(font, win->w/2 - 10, win->h/2 - 10, "+");
        }
    }

    virtual void event(gl::GLWindow *window, SDL_Event &e) override {
        if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.sym == SDLK_m) {
                mouseControl = !mouseControl;
                
                if (mouseControl) {
                    SDL_WarpMouseInWindow(window->getWindow(), window->w/2, window->h/2);
                    SDL_ShowCursor(SDL_DISABLE);
                    lastMouseX = window->w/2;
                    lastMouseY = window->h/2;
                } else {
                    SDL_ShowCursor(SDL_ENABLE);
                }
            }
        } 
        else if (e.type == SDL_MOUSEMOTION && mouseControl) {
            int deltaX = e.motion.x - lastMouseX;
            int deltaY = e.motion.y - lastMouseY;
            yaw += deltaX * mouseSensitivity;
            pitch += deltaY * mouseSensitivity; 
            pitch = glm::clamp(pitch, -89.0f, 89.0f);
            lastMouseX = e.motion.x;
            lastMouseY = e.motion.y;
            if (e.motion.x <= 10 || e.motion.x >= window->w - 10 || 
                e.motion.y <= 10 || e.motion.y >= window->h - 10) {
                SDL_WarpMouseInWindow(window->getWindow(), window->w/2, window->h/2);
                lastMouseX = window->w/2;
                lastMouseY = window->h/2;
            }
        }
        else if (e.type == SDL_FINGERDOWN) {
            touchActive = true;
            lastTouchX = e.tfinger.x * window->w;  
            lastTouchY = e.tfinger.y * window->h;
        }
        else if (e.type == SDL_FINGERUP) {
            touchActive = false;
        }
        else if (e.type == SDL_FINGERMOTION && touchActive) {
            float touchX = e.tfinger.x * window->w;
            float touchY = e.tfinger.y * window->h;
            float deltaX = touchX - lastTouchX;
            float deltaY = touchY - lastTouchY;
            float touchSensitivity = 0.2f;
            yaw += deltaX * touchSensitivity;
            pitch += deltaY * touchSensitivity;
            pitch = glm::clamp(pitch, -89.0f, 89.0f);
            lastTouchX = touchX;
            lastTouchY = touchY;
        }
    }
    
private:
    mx::Font font;
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("OpenGL Example", tw, th) {
        setPath(path);
        setObject(new Cube());
		object->load(this);
    }
    
    ~MainWindow() override {}
    
    virtual void event(SDL_Event &e) override {}

    virtual void draw() override {
        glClearColor(0.0f, 0.0f,0.0f, 1.0f);  
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        object->draw(this);
        swap();
    }
private:
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
        if(args.fullscreen)
            main_window.setFullScreen(true);
        main_window.loop();
    } catch(const mx::Exception &e) {
        mx::system_err << "mx: Exception: " << e.text() << "\n";
        mx::system_err.flush();
        exit(EXIT_FAILURE);
    } 
#endif
    return 0;
}