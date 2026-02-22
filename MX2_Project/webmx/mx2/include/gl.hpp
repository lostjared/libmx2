#ifndef GL_H__
#define GL_H__

#ifdef __EMSCRIPTEN__
#include"config.hpp"
#else
#include"config.h"
#endif

#ifdef WITH_GL
#include "mx.hpp"
#include<string>
#include<memory>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <GLES3/gl3.h>
#include<emscripten/html5.h>
#include "glm.hpp"
#include "gtc/type_ptr.hpp"
#else
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#endif

#include"console.hpp"

namespace gl {

    extern const char *vSource;
    extern const char *fSource;

    class ShaderProgram {
    private:
        struct SharedState {
            GLuint vertex_shader = 0;
            GLuint fragment_shader = 0;
            GLuint shader_id = 0;
            std::string name_;
            bool silent_ = false;
            
            ~SharedState();  
        };
        std::shared_ptr<SharedState> state;
    public:
        ShaderProgram();
        ShaderProgram(GLuint id);
        ~ShaderProgram() = default;  
        ShaderProgram(const ShaderProgram&) = default;
        ShaderProgram& operator=(const ShaderProgram&) = default;    
        ShaderProgram(ShaderProgram&&) noexcept = default;
        ShaderProgram& operator=(ShaderProgram&&) noexcept = default;
        void release();
        GLuint id() const;
        bool loaded() const;
        void useProgram();
        void setName(const std::string &n);
        void setSilent(bool s);
        bool loadProgram(const std::string &v, const std::string &f);
        bool loadProgramFromText(const std::string &v, const std::string &f);
        void setUniform(const std::string &name, int value);
        void setUniform(const std::string &name, float value);
        void setUniform(const std::string &name, const glm::vec2 &value);
        void setUniform(const std::string &name, const glm::vec3 &value);
        void setUniform(const std::string &name, const glm::vec4 &value);
        void setUniform(const std::string &name, const glm::mat4 &value);
    private:
        GLuint createProgram(const char *vshaderSource, const char *fshaderSource);
        GLuint createProgramFromFile(const std::string &vert, const std::string &frag);
        int printShaderLog(GLuint shader);
        void printProgramLog(int p);
        bool checkError();
    };
    
    class GLText {
    public:
        GLText();
        void init(int w, int h);
        GLuint createText(const std::string &text, TTF_Font *font, SDL_Color color, int &textWidth, int &textHeight, bool solid = true);
        void renderText(GLuint texture, float x, float y, int textWidth, int textHeight, int screenWidth, int screenHeight);
        void printText_Solid(const mx::Font &f, float x, float y, const std::string &text);
        void printText_Blended(const mx::Font &f, float x, float y, const std::string &text);
        void setColor(SDL_Color col);
    private:
        ShaderProgram textShader;
        SDL_Color color = {255,255,255,255};
        int w = 0, h = 0;
    };

    class GLSprite {
    public:
        GLSprite();
        ~GLSprite();

        GLSprite(const GLSprite&) = delete;
        GLSprite& operator=(const GLSprite&) = delete;
        GLSprite(GLSprite&&) = delete;
        GLSprite& operator=(GLSprite&&) = delete;

        void release();
        void initSize(float w, float h);
        void setName(const std::string &name);
        void setShader(ShaderProgram *program);
        void initWithTexture(ShaderProgram *program, GLuint texture, float x, float y, int textWidth, int textHeight);
        void loadTexture(ShaderProgram *shader, const std::string &tex, float x, float  y, int textWidth, int textHeight);
        void loadTexture(ShaderProgram *shader, const std::string &tex, float x, float y);
        void draw();
        void draw(GLuint texture_id, float x, float y, int w, int h);
        void updateTexture(SDL_Surface *surf);
    private:
        ShaderProgram *shader;
        GLuint texture = 0;
        GLuint VBO = 0, VAO = 0;
        std::vector<float> vertices;
        float screenWidth = 0.0f, screenHeight = 0.0f;
        int width = 0, height = 0;
        std::string textureName;
    };

    enum class GLMode  { DESKTOP, ES };

    class GLObject;
   
    class GLWindow {
        private:
        GLMode gl_mode;
    public:
#ifdef __EMSCRIPTEN__
        EMSCRIPTEN_WEBGL_CONTEXT_HANDLE webglContext = 0;    
        void restoreContext();
#endif
        GLWindow(const std::string &text, int width, int height, bool resize_ = true, GLMode mode = GLMode::DESKTOP) : gl_mode(mode), glContext{nullptr}, window{nullptr} { 
            initGL(text, width, height, resize_);
        }
        GLWindow(int width, int height, GLMode mode);
        virtual ~GLWindow();

        GLWindow(const GLWindow&) = delete;
        GLWindow& operator=(const GLWindow&) = delete;
        GLWindow(GLWindow&&) = delete;
        GLWindow& operator=(GLWindow&&) = delete;

        void initGL(const std::string &title, int width, int height, bool resize_ = true);
        void initGL(int width, int height);
        void updateViewport();
        void swap();

        virtual void event(SDL_Event &e) = 0;
        virtual void draw() = 0;
        virtual void resize(int w,  int h) {}

        void setObject(GLObject *o);
        void quit();
        void proc();
        void loop();
        void delay();
        void setPath(const std::string &path) { util.path = path; }
        void setWindowTitle(const std::string &title);
        void setWindowSize(int w, int h);
        void setWindowIcon(SDL_Surface *ico);
        void setFullScreen(bool full);
        void activateConsole(const std::string &fnt, int size, const SDL_Color &color);
        void activateConsole(const SDL_Rect &rc, const std::string &fnt, int size, const SDL_Color &color);
        void drawConsole();
        void showConsole(bool show);
        std::unique_ptr<gl::GLObject> object = nullptr;
        mx::mxUtil util;
        console::GLConsole console;
        int w = 0, h = 0;
        SDL_Window *getWindow() { return window; }
#ifdef WITH_MIXER
        mx::Mixer mixer;
#endif
        GLText text;  
        bool console_visible = false;
        bool console_active = false;
    private:
        SDL_GLContext glContext;
        SDL_Window *window = nullptr;
        bool active = false;
        SDL_Event e;
        bool hide_console = false;
    };

    class GLObject {
    public:
        GLObject() = default;
        virtual ~GLObject() = default;
        virtual void load(GLWindow *win) = 0;
        virtual void draw(GLWindow *win) = 0;
        virtual void event(GLWindow *window, SDL_Event &e) = 0;
        virtual void resize(gl::GLWindow *win, int w, int h) {}
    };

    GLuint loadTexture(const std::string &filename);
    GLuint loadTexture(const std::string &filename, int &w, int &h);
    void updateTexture(GLuint texture, SDL_Surface *surface, bool flip);    
    GLuint createTexture(SDL_Surface *surface, bool flip);
    SDL_Surface *createSurface(int w, int h);    
}
#endif
#endif