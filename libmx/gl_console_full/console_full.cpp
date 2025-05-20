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
    virtual ~Game() override {}

    void load(gl::GLWindow *win) override {
        font.loadFont(win->util.getFilePath("data/font.ttf"), 36);
        win->console.print("Console Skeleton Example\nLostSideDead Software\nhttps://lostsidedead.biz\n");
        win->console.setPrompt("mx> ");
#if defined(__EMSCRIPTEN__) 
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
    precision highp float;
    in vec2 TexCoord;
    out vec4 color;
    uniform sampler2D textTexture;
    uniform float time_f;
    uniform float alpha;

    void main(void) {
        vec2 tc = TexCoord;
        float rippleSpeed = 5.0;
        float rippleAmplitude = 0.03;
        float rippleWavelength = 10.0;
        float twistStrength = 1.0;
        float radius = length(tc - vec2(0.5, 0.5));
        float ripple = sin(tc.x * rippleWavelength + time_f * rippleSpeed) * rippleAmplitude;
        ripple += sin(tc.y * rippleWavelength + time_f * rippleSpeed) * rippleAmplitude;
        vec2 rippleTC = tc + vec2(ripple, ripple);
        float angle = twistStrength * (radius - 1.0) + time_f;
        float cosA = cos(angle);
        float sinA = sin(angle);
        mat2 rotationMatrix = mat2(cosA, -sinA, sinA, cosA);
        vec2 twistedTC = (rotationMatrix * (tc - vec2(0.5, 0.5))) + vec2(0.5, 0.5);
        vec4 originalColor = texture(textTexture, tc);
        vec4 twistedRippleColor = texture(textTexture, mix(rippleTC, twistedTC, 0.5));
        color = twistedRippleColor * alpha;
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
    out vec4 color;
    uniform sampler2D textTexture;
    uniform float time_f;
    uniform float alpha;

    void main(void) {
        vec2 tc = TexCoord;
        float rippleSpeed = 5.0;
        float rippleAmplitude = 0.03;
        float rippleWavelength = 10.0;
        float twistStrength = 1.0;
        float radius = length(tc - vec2(0.5, 0.5));
        float ripple = sin(tc.x * rippleWavelength + time_f * rippleSpeed) * rippleAmplitude;
        ripple += sin(tc.y * rippleWavelength + time_f * rippleSpeed) * rippleAmplitude;
        vec2 rippleTC = tc + vec2(ripple, ripple);
        float angle = twistStrength * (radius - 1.0) + time_f;
        float cosA = cos(angle);
        float sinA = sin(angle);
        mat2 rotationMatrix = mat2(cosA, -sinA, sinA, cosA);
        vec2 twistedTC = (rotationMatrix * (tc - vec2(0.5, 0.5))) + vec2(0.5, 0.5);
        vec4 originalColor = texture(textTexture, tc);
        vec4 twistedRippleColor = texture(textTexture, mix(rippleTC, twistedTC, 0.5));
        color = twistedRippleColor * alpha;
    }
)";
#endif
        if (!program.loadProgramFromText(vSource, fSource)) {
            throw mx::Exception("Failed to load shader program");
        }
        program.useProgram();
        program.setUniform("textTexture", 0);
        program.setUniform("time_f", 0.0f);
        program.setUniform("alpha", 1.0f);
        logo.initSize(win->w, win->h);
        logo.loadTexture(&program, win->util.getFilePath("data/crystal_red.png"), 0.0f, 0.0f, win->w, win->h);
        win->console.setCallback(win, [&](gl::GLWindow *window, const std::vector<std::string> &args) -> bool {
            if (args.size() == 1 && args[0] == "exit") {
                window->quit();
                return true;
            } else if(args.size() == 1 && args[0] == "author") {
                window->console.print("Coded by Jared Bruni\nLostSideDead Software\n");
                window->console.print("https://lostsidedead.biz\n");
                return true;
            } else if(args.size() == 2 && args[0] == "stretch" && args[1] == "on") {
                window->console.setStretch(true);
                window->console.print("Stretching is now ON\n");
                return true;
            } else if(args.size() == 2 && args[0] == "stretch" && args[1] == "off") {
                window->console.setStretch(false);
                window->console.print("Stretching is now OFF\n");
                return true;
            } else if(args.size() == 3 && args[0] == "size") {
                int ww = std::stoi(args[1]);
                int hh = std::stoi(args[2]);
                window->console.resize(window, ww, hh);
                window->console.printf("Resizing to %d x %d\n", ww, hh);
            } else if(args.size() == 2 && args[0] == "stretch_height") {
                if(args[1] == "1") {
                    window->console.setStretchHeight(1);
                    window->console.print("Stretch height set to 1\n");
                } else if(args[1] == "0") {
                    window->console.setStretchHeight(0);
                    window->console.print("Stretch height set to 0\n");
                }
                return true;
            }
            return window->console.procDefaultCommands(args);
        });
    }

    void draw(gl::GLWindow *win) override {
        glDisable(GL_DEPTH_TEST);
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; 
        lastUpdateTime = currentTime;
        logo.draw();
        update(deltaTime);
    }
    
    void event(gl::GLWindow *win, SDL_Event &e) override {}
    void update(float deltaTime) {

        static float time_f = 0.0f;
        time_f += deltaTime;
        program.setUniform("time_f", time_f);

    }
private:
    mx::Font font;
    gl::GLSprite logo;
    gl::ShaderProgram program;
    Uint32 lastUpdateTime = SDL_GetTicks();
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("Console Skeleton", tw, th) {
        setPath(path);
        setObject(new Game());
        object->load(this);
        activateConsole({25, 25, tw-50, th-50}, util.getFilePath("data/font.ttf"), 16, {255, 255, 255, 255});
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
    try {
        MainWindow main_window("/", 1920, 1080);
        main_w = &main_window;
        emscripten_set_main_loop(eventProc, 0, 1);
    } catch(const mx::Excpetion &e) {
        SDL_Log("mx: Exception: %s\n", e.text().c_str());
        SDL_LogFlush();
    } catch(std::exception &e) {
        std::cerr << e.what() << "\n";
    }
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
