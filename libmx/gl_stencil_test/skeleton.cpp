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

#ifdef __EMSCRIPTEN__
    const char *fxSource = R"(#version 300 es
        precision highp float;
        in vec2 TexCoord;
        out vec4 FragColor;
        uniform sampler2D textTexture;
        uniform float time_f;
        uniform vec2 iResolution;

        vec3 rainbow(float t) {
            t = fract(t);
            float r = abs(t * 6.0 - 3.0) - 1.0;
            float g = 2.0 - abs(t * 6.0 - 2.0);
            float b = 2.0 - abs(t * 6.0 - 4.0);
            return clamp(vec3(r, g, b), 0.0, 1.0);
        }

        void main(void) {
            vec2 tc = TexCoord;
            vec2 uv = tc * 2.0 - 1.0;
            uv.y *= iResolution.y / iResolution.x;
            float wave = sin(uv.x * 10.0 + time_f * 2.0) * 0.1;
            float angle = atan(uv.y + wave, uv.x) + time_f * 2.0;
            vec3 rainbow_color = rainbow(angle / (2.0 * 3.14159));
            vec4 original_color = texture(textTexture, tc);
            vec3 blended_color = mix(original_color.rgb, rainbow_color, 0.5);
            FragColor = vec4(sin(blended_color * time_f), original_color.a);
            vec4 color = texture(textTexture, TexCoord);
            FragColor += color + 0.5;
        }
    )";
#else
const char *fxSource = R"(#version 330
        in vec2 TexCoord;
        out vec4 FragColor;
        uniform sampler2D textTexture;
        uniform float time_f;
        uniform vec2 iResolution;

        vec3 rainbow(float t) {
            t = fract(t);
            float r = abs(t * 6.0 - 3.0) - 1.0;
            float g = 2.0 - abs(t * 6.0 - 2.0);
            float b = 2.0 - abs(t * 6.0 - 4.0);
            return clamp(vec3(r, g, b), 0.0, 1.0);
        }

        void main(void) {
            vec2 tc = TexCoord;
            vec2 uv = tc * 2.0 - 1.0;
            uv.y *= iResolution.y / iResolution.x;
            float wave = sin(uv.x * 10.0 + time_f * 2.0) * 0.1;
            float angle = atan(uv.y + wave, uv.x) + time_f * 2.0;
            vec3 rainbow_color = rainbow(angle / (2.0 * 3.14159));
            vec4 original_color = texture(textTexture, tc);
            vec3 blended_color = mix(original_color.rgb, rainbow_color, 0.5);
            FragColor = vec4(sin(blended_color * time_f), original_color.a);
            vec4 color = texture(textTexture, TexCoord);
            FragColor += color + 0.5;
        }
    )";
#endif

class Game : public gl::GLObject {
public:
    Game() = default;
    virtual ~Game() override {}

    void load(gl::GLWindow *win) override {
        if(shader.loadProgramFromText(gl::vSource, fxSource) == false) {
            throw mx::Exception("Failed to load shader program");
        }
        font.loadFont(win->util.getFilePath("data/font.ttf"), 76);
        img.initSize(win->w, win->h);
        shader.useProgram();
        glUniform2f(glGetUniformLocation(shader.id(), "iResolution"), win->w, win->h);
        img.loadTexture(&shader, win->util.getFilePath("data/logo.png"), 0, 0, win->w, win->h);
    }

    void draw(gl::GLWindow *win) override {
        glEnable(GL_STENCIL_TEST);
        glStencilFunc(GL_ALWAYS, 1, 0xFF);  
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);  
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        win->text.setColor({255,255,255,255});
        win->text.printText_Solid(font, 25.0f, 25.0f, "Hello World, with OpenGL!");
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glStencilFunc(GL_EQUAL, 1, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        shader.useProgram();
        static float time_f = 0.0f;
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; // Convert to seconds
        lastUpdateTime = currentTime;
        time_f += deltaTime;
        shader.setUniform("time_f", time_f);
        img.draw();
        glDisable(GL_STENCIL_TEST);
        update(deltaTime);
    }
    
    void event(gl::GLWindow *win, SDL_Event &e) override {}
    void update(float deltaTime) {}
    
private:
    mx::Font font;
    gl::GLSprite img;
    gl::ShaderProgram shader;
    Uint32 lastUpdateTime = SDL_GetTicks();
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("Skeleton", tw, th) {
        setPath(path);
        setObject(new Game());
        object->load(this);
    }
    
    ~MainWindow() override {}
    
    virtual void event(SDL_Event &e) override {
    }
    
    virtual void draw() override {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClearStencil(0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
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
