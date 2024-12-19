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

class Game : public gl::GLObject {
public:
    
    Game() = default;
    ~Game() override {
        if(surface) {
            SDL_FreeSurface(surface);
        }
        if(texture) {
            glDeleteTextures(1, &texture);
        } 
    }
    void load(gl::GLWindow *win) override {
        font.loadFont(win->util.getFilePath("data/font.ttf"), 16);
        if(!shader.loadProgramFromText(gl::vSource, gl::fSource)) {
            throw mx::Exception("Could not load shader");
        }
        surface = gl::createSurface(640, 480);
        if(!surface) {
            throw mx::Exception("Error creating surface");
        }
        SDL_FillRect(surface, 0, SDL_MapRGBA(surface->format,255, 255, 255, 255));
        texture = gl::createTexture(surface, true);
        sprite.initSize(win->w, win->h);
        sprite.initWithTexture(&shader, texture, 0, 0, win->w, win->h);
    }

    Uint8 value = 0;

    void draw(gl::GLWindow *win) override {
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; // Convert to seconds
        lastUpdateTime = currentTime;
        if(deltaTime > 0.005) {
            value += 1;
            if(value >= 254) value = 0;
        }
        update(deltaTime);
        SDL_FillRect(surface, 0, SDL_MapRGBA(surface->format, value, value, value, 255));
        sprite.updateTexture(surface);
        sprite.draw();
        win->text.printText_Solid(font, 5.0f, 5.0f, "Hello, World!");
        win->text.printText_Solid(font, 250.0f, 250.0f, "OpenGL Text");
    }
    void event(gl::GLWindow *win, SDL_Event &e) override {}
    void update(float deltaTime) {}
private:
    Uint32 lastUpdateTime = SDL_GetTicks();
    mx::Font font;
    gl::GLSprite sprite;
    GLuint texture;
    SDL_Surface *surface = nullptr;
    gl::ShaderProgram shader;
};

#ifdef __EMSCRIPTEN__
    const char *vSource = R"(#version 300 es
            precision mediump float;
            layout (location = 0) in vec3 aPos;
            layout (location = 1) in vec2 aTexCoord;

            out vec2 TexCoord;

            void main() {
                gl_Position = vec4(aPos, 1.0); 
                TexCoord = aTexCoord;         
            }
    )";
    const char *fSource = R"(#version 300 es
        precision mediump float;
        in vec2 TexCoord;
        out vec4 FragColor;
        uniform sampler2D textTexture; 
        uniform float alpha;
        void main() {
            vec4 fcolor = texture(textTexture, TexCoord);
            FragColor = mix(fcolor, vec4(0.0, 0.0, 0.0, fcolor.a), alpha);
        }
    )";
#else
    const char *vSource = R"(#version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec2 aTexCoord;
        out vec2 TexCoord;
        void main() {
            gl_Position = vec4(aPos, 1.0); 
            TexCoord = aTexCoord;        
        }
    )";
    const char *fSource = R"(#version 330 core
        in vec2 TexCoord;
        out vec4 FragColor;
        uniform sampler2D textTexture; 
        uniform float alpha;
        void main() {
            vec4 fcolor = texture(textTexture, TexCoord);
            FragColor = mix(fcolor, vec4(0.0, 0.0, 0.0, fcolor.a), alpha);
        }
    )";
#endif

class IntroScreen : public gl::GLObject {
    float fade = 0.0f;
public:
    IntroScreen() = default;
    ~IntroScreen() override {

    }

    virtual void load(gl::GLWindow *win) override {
        if(!program.loadProgramFromText(vSource, fSource)) {
            throw mx::Exception("Error loading shader program");
        }
        logo.initSize(win->w, win->h);
        logo.loadTexture(&program, win->util.getFilePath("data/logo.png"), 0, 0, win->w, win->h);
    }

    void draw(gl::GLWindow *win) override {
#ifndef __EMSCRIPTEN__
        Uint32 currentTime = SDL_GetTicks();
#else
        double currentTime = emscripten_get_now();
#endif
        program.useProgram();
        program.setUniform("alpha", fade);
        logo.draw();
        if((currentTime - lastUpdateTime) > 25) {
            lastUpdateTime = currentTime;
            fade += 0.01;
        }
        if(fade >= 1.0) {
            win->setObject(new Game());
            win->object->load(win);
            return;
        }
    }
    void event(gl::GLWindow *win, SDL_Event &e) override {}

private:
    Uint32 lastUpdateTime;
    gl::GLSprite logo;
    gl::ShaderProgram program;
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("GL Window", tw, th) {
        setPath(path);
        setObject(new IntroScreen());
        object->load(this);
    }
    
    ~MainWindow() override {}
    
    virtual void event(SDL_Event &e) override {
        object->event(this, e);
    }
    
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
    Argz<std::string> parser(argc, argv);
    parser.addOptionSingle('h', "Display help message")
        .addOptionSingleValue('p', "assets path")
        .addOptionDoubleValue('P', "path", "assets path")
        .addOptionSingleValue('r',"Resolution WidthxHeight")
        .addOptionDoubleValue('R',"resolution", "Resolution WidthxHeight");
    Argument<std::string> arg;
    std::string path;
    int value = 0;
    int tw = 960, th = 720;
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
                    std::string left, right;
                    left = arg.arg_value.substr(0, pos);
                    right = arg.arg_value.substr(pos+1);
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
        mx::system_out << "mx: No path provided trying default current directory.\n";
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
