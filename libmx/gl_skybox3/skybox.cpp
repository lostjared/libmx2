#include"mx.hpp"
#include"argz.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <GLES3/gl3.h>
#endif

#include"gl.hpp"
#include"loadpng.hpp"
#include"model.hpp"

#define CHECK_GL_ERROR() \
    { GLenum err = glGetError(); \
      if (err != GL_NO_ERROR) \
        printf("OpenGL Error: %d at %s:%d\n", err, __FILE__, __LINE__); }

class Cube : public gl::GLObject {
public:
    gl::ShaderProgram shaderProgram;
    GLuint texture;
    mx::Model cubeModel;
    
    float yaw = 0.0f;
    float pitch = 0.0f;
    bool mouseControl = false;
    int lastMouseX = 0;
    int lastMouseY = 0;
    float mouseSensitivity = 0.15f;
    bool touchActive = false;
    float lastTouchX = 0.0f;
    float lastTouchY = 0.0f;
    
    
    float twistAngle = 0.0f;
    float wavePhase = 0.0f;
    float expandScale = 1.0f;
    bool expanding = true;
    float twistSpeed = 45.0f;
    float waveSpeed = 2.0f;       
    float expandSpeed = 0.3f;     
    float minScale = 0.8f;
    float maxScale = 50.5f;
    float waveAmplitude = 0.3f;
    float waveFrequency = 3.0f;
            
    Cube() = default;
    virtual ~Cube() override {
        glDeleteTextures(1, &texture);
    }

    virtual void load(gl::GLWindow *win) override {     
        font.loadFont(win->util.getFilePath("data/font.ttf"), 24);
        if (!cubeModel.openModel(win->util.getFilePath("data/cube.mxmod.z"), true)) {
            throw mx::Exception("Failed to load cube.mxmod model");
        }
        cubeModel.saveOriginal();  
        mx::system_out << "mx: Loaded cube model with " << cubeModel.meshes.size() << " meshes\n";
        if (!shaderProgram.loadProgram(win->util.getFilePath("data/cube.vert"), win->util.getFilePath("data/cube.frag"))) {
            throw mx::Exception("Failed to load skybox shader program");
        }
        shaderProgram.useProgram();
        shaderProgram.setUniform("cubemapTexture", 0);
        cubeModel.setShaderProgram(&shaderProgram, "cubemapTexture");
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
        
        twistAngle += twistSpeed * deltaTime;
        wavePhase += waveSpeed * deltaTime * 2.0f * 3.14159f;
        
        if (expanding) {
            expandScale += expandSpeed * deltaTime;
            if (expandScale >= maxScale) {
                expandScale = maxScale;
                expanding = false;
            }
        } else {
            expandScale -= expandSpeed * deltaTime;
            if (expandScale <= minScale) {
                expandScale = minScale;
                expanding = true;
            }
        }
        
        
        cubeModel.resetToOriginal();
        cubeModel.twist(mx::DeformAxis::Y, glm::radians(twistAngle), 0.0f);
        //cubeModel.wave(mx::DeformAxis::X, waveAmplitude, waveFrequency, wavePhase);
        //cubeModel.wave(mx::DeformAxis::Z, waveAmplitude * 0.5f, waveFrequency * 1.5f, wavePhase * 0.7f);
        cubeModel.scale(expandScale);
        cubeModel.recalculateNormals();
        cubeModel.updateBuffers();
        
        static float rotation = 0.0f;
        rotation += deltaTime * 50.0f; 

        shaderProgram.useProgram();
        glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 1.0f, 1.0f));
        model = glm::scale(model, glm::vec3(500.0f, 500.0f, 500.0f));
        shaderProgram.setUniform("model", model);
        shaderProgram.setUniform("view", view);
        shaderProgram.setUniform("projection", projection);
        shaderProgram.setUniform("time_f", time_f);
        
        glDisable(GL_CULL_FACE);
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);
        cubeModel.drawArraysWithCubemap(texture, "cubemapTexture");
        CHECK_GL_ERROR();
        
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
    }

    virtual void event(gl::GLWindow *window, SDL_Event &e) override {
       if (e.type == SDL_FINGERDOWN) {
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