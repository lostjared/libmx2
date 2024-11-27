#ifndef GL_H__
#define GL_H__
#ifdef WITH_GL
#include <GL/glew.h>
#include "mx.hpp"
#include<string>
#include<memory>
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
        
        void setPath(const std::string &path) { util.path = path; }

        std::unique_ptr<gl::GLObject> object;
        mx::mxUtil util;
    private:
        SDL_GLContext glContext;
        SDL_Window *window;
        int w = 0, h = 0;
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
    private:
        GLuint shader_id;
        std::string name_;
    };
    

}
#endif
#endif