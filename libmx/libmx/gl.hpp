#ifndef GL_H__
#define GL_H__

#ifdef __EMSCRIPTEN__
#include"config.hpp"
#endif

#ifdef WITH_GL
#include "mx.hpp"
#include<string>
#include<memory>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <GLES3/gl3.h>
#include "glm.hpp"
#include "gtc/type_ptr.hpp"
#else
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>
#endif

namespace gl {

    class GLObject;
   
    class GLWindow {
    public:
        GLWindow(const std::string &text, int width, int height) : glContext{nullptr}, window{nullptr} { 
            initGL(text, width, height);
        };
        virtual ~GLWindow();
        void initGL(const std::string &title, int width, int height);
        void swap();

        virtual void event(SDL_Event &e) = 0;
        virtual void draw() = 0;

        void setObject(GLObject *o);

        void proc();
        void loop();
        void delay();
        void setPath(const std::string &path) { util.path = path; }
        void setWindowTitle(const std::string &title);
        std::unique_ptr<gl::GLObject> object;
        mx::mxUtil util;
        int w = 0, h = 0;
#ifdef WITH_MIXER
        mx::Mixer mixer;
#endif
        
    private:
        SDL_GLContext glContext;
        SDL_Window *window;
        bool active = false;
        SDL_Event e;
    };

    class GLObject {
    public:
        GLObject() = default;
        virtual ~GLObject() = default;
        virtual void load(GLWindow *win) = 0;
        virtual void draw(GLWindow *win) = 0;
        virtual void event(GLWindow *window, SDL_Event &e) = 0;
    };

    class ShaderProgram {
    public:
        ShaderProgram();
        ShaderProgram(GLuint id);
        ShaderProgram &operator=(const ShaderProgram &p);        
        int printShaderLog(GLuint err);
        void printProgramLog(int p);
        bool checkError();
        GLuint createProgram(const char *vert, const char *frag);
        GLuint createProgramFromFile(const std::string &vert, const std::string &frag);
        
        bool loadProgram(const std::string &text1, const std::string &text2);
        int id() const { return shader_id; }
        void useProgram();
        void setName(const std::string &n);
        std::string name() const { return name_; }

        void setUniform(const std::string &name, int value);
        void setUniform(const std::string &name, float value);
        void setUniform(const std::string &name, const glm::vec3 &value);
        void setUniform(const std::string &name, const glm::vec4 &value);
        void setUniform(const std::string &name, const glm::mat4 &value);

    private:
        GLuint shader_id;
        std::string name_;
    };
    

}
#endif
#endif
