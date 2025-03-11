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


struct SnowFlake {
    float x, y, z;
    float intensity;
    float size;
    float speed;
    float angle;
    float rotation;
    float rotationSpeed;
    float rotationAngle;
    float rotationDirection;
    float rotationIntensity;
    float rotationSize;
    float rotationSpeedSize;
    float rotationSpeedIntensity;
};

class SnowEmiter {
public:
    static const int MAX_SNOWFLAKES = 3000;
    
    SnowEmiter() {}
    ~SnowEmiter() {
        releaseSnowFlakes();
    }

    void initPointSprites(gl::GLWindow *win) {
        SDL_Surface* snowflakeSurface = png::LoadPNG(win->util.getFilePath("data/snowflake.png").c_str());
        if (!snowflakeSurface) {
            throw mx::Exception("Failed to load snowflake texture");
        }

        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        
    
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
    
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, snowflakeSurface->w, snowflakeSurface->h, 
                     0, GL_RGBA, GL_UNSIGNED_BYTE, snowflakeSurface->pixels);
        
        SDL_FreeSurface(snowflakeSurface);
        
        if (!snowShader.loadProgram(win->util.getFilePath("data/snow_vert.glsl"), 
                                   win->util.getFilePath("data/snow_frag.glsl"))) {
            throw mx::Exception("Failed to load snow shaders");
        }
        
        snowflakes.reserve(MAX_SNOWFLAKES);
        for (int i = 0; i < MAX_SNOWFLAKES; ++i) {
            SnowFlake flake;
            flake.x = static_cast<float>(rand() % 2000 - 1000) / 100.0f;
            flake.y = static_cast<float>(rand() % 2000 - 1000) / 100.0f;
            flake.z = static_cast<float>(rand() % 1000) / 1000.0f - 2.0f;
            flake.intensity = static_cast<float>(rand() % 100) / 100.0f;
            flake.size = 0.005f + static_cast<float>(rand() % 100) / 3000.0f;
            flake.speed = 0.4f + static_cast<float>(rand() % 100) / 500.0f;
            flake.angle = static_cast<float>(rand() % 360) * (M_PI / 180.0f);
            flake.rotation = static_cast<float>(rand() % 360) * (M_PI / 180.0f);
            flake.rotationSpeed = (static_cast<float>(rand() % 100) / 1000.0f) - 0.05f;
            flake.rotationAngle = static_cast<float>(rand() % 360) * (M_PI / 180.0f);
            flake.rotationDirection = (rand() % 2) ? 1.0f : -1.0f;
            flake.rotationIntensity = static_cast<float>(rand() % 100) / 100.0f;
            flake.rotationSize = 0.01f + static_cast<float>(rand() % 100) / 4000.0f;
            flake.rotationSpeedSize = (static_cast<float>(rand() % 100) / 2000.0f) - 0.025f;
            flake.rotationSpeedIntensity = static_cast<float>(rand() % 100) / 100.0f;
            snowflakes.push_back(flake);
        }
        
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, snowflakes.size() * sizeof(SnowFlake), 
                    snowflakes.data(), GL_DYNAMIC_DRAW);
    
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SnowFlake), (void*)0);
        glEnableVertexAttribArray(0);
    
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(SnowFlake), 
                            (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glBindVertexArray(0);
        
        CHECK_GL_ERROR();
    }

    void update(float deltaTime, int width, int height) {
        windTime += deltaTime * 1.2f;
        float windOffsetX = std::sin(windTime) * 0.3f;
        float windOffsetZ = std::cos(windTime * 0.7f) * 0.2f;
        
        for (auto& flake : snowflakes) {
        
            flake.y -= flake.speed * deltaTime;
            
            flake.x += windOffsetX * deltaTime;
            flake.z += windOffsetZ * deltaTime;
            
            flake.rotation += flake.rotationSpeed * deltaTime;
            
            if (flake.y < -10.0f) {
                flake.y = 10.0f;
                flake.x = static_cast<float>(rand() % 2000 - 1000) / 100.0f;
                flake.z = static_cast<float>(rand() % 1000) / 1000.0f - 2.0f;
            }
        }
        
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, snowflakes.size() * sizeof(SnowFlake), 
                       snowflakes.data());
    }

    void draw(gl::GLWindow *win) {
        
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 
                                              (float)win->w / (float)win->h, 
                                              0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 10.0f),  
            glm::vec3(0.0f, 0.0f, 0.0f),   
            glm::vec3(0.0f, 1.0f, 0.0f)    
        );
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#ifndef __EMSCRIPTEN__
        glEnable(GL_PROGRAM_POINT_SIZE);
#endif
        glDepthMask(GL_FALSE);
        
        snowShader.useProgram();
        
        GLint viewProjLoc = glGetUniformLocation(snowShader.id(), "viewProj");
        glUniformMatrix4fv(viewProjLoc, 1, GL_FALSE, glm::value_ptr(projection * view));
        
        GLint texLoc = glGetUniformLocation(snowShader.id(), "snowflakeTexture");
        glUniform1i(texLoc, 0);
        
        GLint timeLoc = glGetUniformLocation(snowShader.id(), "time");
        glUniform1f(timeLoc, SDL_GetTicks() / 1000.0f);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        
        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, snowflakes.size());
        glBindVertexArray(0);

#ifndef __EMSCRIPTEN__
        glDisable(GL_PROGRAM_POINT_SIZE);
#endif
        glDepthMask(GL_TRUE);
        CHECK_GL_ERROR();
    }

    void releaseSnowFlakes() {
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

private:
    std::vector<SnowFlake> snowflakes;
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint textureID = 0;
    gl::ShaderProgram snowShader;
    float windTime = 0.0f;
};

class Game : public gl::GLObject {
public:
    Game() = default;
    virtual ~Game() override {}

    void load(gl::GLWindow *win) override {
        font.loadFont(win->util.getFilePath("data/font.ttf"), 36);
        snowEmitter.initPointSprites(win);
        if(!bg_shader.loadProgramFromText(gl::vSource, gl::fSource)) {
            throw mx::Exception("Failed to load background shader");
        }
        background.initSize(win->w, win->h);
        background.loadTexture(&bg_shader, win->util.getFilePath("data/cabin.png"), 0.0f, 0.0f, win->w, win->h);
    }

    void draw(gl::GLWindow *win) override {

        glDisable(GL_DEPTH_TEST);
        bg_shader.useProgram();
        background.draw();
        glEnable(GL_DEPTH_TEST);

        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; 
        lastUpdateTime = currentTime;
        snowEmitter.update(deltaTime, win->w, win->h);
        snowEmitter.draw(win);
        glDisable(GL_DEPTH_TEST);
        win->text.setColor({255, 255, 255, 255});
        win->text.printText_Solid(font, 25.0f, 25.0f, "FPS: " + 
                                 std::to_string(1.0f / (deltaTime > 0.0001f ? deltaTime : 0.0001f)));
    }
    
    void event(gl::GLWindow *win, SDL_Event &e) override {}
    
private:
    mx::Font font;
    Uint32 lastUpdateTime = SDL_GetTicks();
    SnowEmiter snowEmitter;
    gl::GLSprite background;
    gl::ShaderProgram bg_shader;
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
