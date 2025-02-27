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

const char *fragShader1 = R"(#version 330 core
        out vec4 color;
        in vec2 TexCoord;
        uniform sampler2D textTexture;
        uniform vec2 iResolution;
        uniform float time_f;
        void main(void) {
            vec2 normCoord = (TexCoord * 2.0 - 1.0) * vec2(iResolution.x / iResolution.y, 1.0);
            float waveStrength = 0.05;
            float waveFrequency = 3.0;
            vec2 wave = vec2(sin(normCoord.y * waveFrequency + time_f) * waveStrength,
                            cos(normCoord.x * waveFrequency + time_f) * waveStrength);

            normCoord += wave;
            float dist = length(normCoord);
            float angle = atan(normCoord.y, normCoord.x);\
            float spiralAmount = tan(time_f) * 3.0;
            angle += dist * spiralAmount;
            vec2 spiralCoord = vec2(cos(angle), sin(angle)) * dist;
            spiralCoord = (spiralCoord / vec2(iResolution.x / iResolution.y, 1.0) + 1.0) / 2.0;
            color = texture(textTexture, spiralCoord);
            color = vec4(color.rgb, 0.8);
        }
)";
const char *fragShader2 = R"(#version 300 es
    precision highp float;
    out vec4 FragColor;
    in vec2 TexCoord;
    
    uniform sampler2D textTexture;
    uniform float time_f;
    float alpha = 1.0;
    
    void main(void) {
        vec2 uv = TexCoord * 2.0 - 1.0;
        float len = length(uv);
        float bubble = smoothstep(0.8, 1.0, 1.0 - len);
        vec2 distort = uv * (1.0 + 0.1 * sin(time_f + len * 20.0));
        vec4 texColor = texture(textTexture, distort * 0.5 + 0.5);
        FragColor = mix(texColor, vec4(1.0, 1.0, 1.0, 1.0), bubble);
        FragColor = FragColor * alpha;
        FragColor = vec4(FragColor.rgb, 0.5);
    }
)";

class Game : public gl::GLObject {
public:
    Game() = default;
    virtual ~Game() override {}

    void load(gl::GLWindow *win) override {
        font.loadFont(win->util.getFilePath("data/font.ttf"), 36);
        if(shader1.loadProgramFromText(gl::vSource, fragShader1) == false) {
            throw mx::Exception("Failed to load shader program shader1");
        }
        if(shader2.loadProgramFromText(gl::vSource, fragShader2) == false) {
            throw mx::Exception("Failed to load shader program shader2");
        }

        shader1.useProgram();
        glUniform2f(glGetUniformLocation(shader1.id(), "iResolution"), win->w, win->h);

        glUseProgram(0);

        image1.initSize(win->w, win->h);
        image2.initSize(win->w, win->h);
        image1.loadTexture(&shader1, win->util.getFilePath("data/cm_back.png"), 0.0f, 0.0f, win->w, win->h);
        image2.loadTexture(&shader2, win->util.getFilePath("data/cm_bottom.png"), 0.0f, 0.0f, win->w, win->h);
    }

    void draw(gl::GLWindow *win) override {
        glDisable(GL_DEPTH_TEST);
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; 
        lastUpdateTime = currentTime;
        static float time_f =  0.0f;
        time_f += deltaTime;
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBlendEquation(GL_FUNC_ADD);
        shader1.useProgram();
        shader1.setUniform("time_f", time_f);
        image1.draw();
        shader2.useProgram();
        shader2.setUniform("time_f", time_f);
        image2.draw();
        glDisable(GL_BLEND);
        win->text.setColor({255, 255, 255, 255});
        win->text.printText_Solid(font, 25.0f, 25.0f, "Hello, World! from OpenGL: " + std::to_string(currentTime));
        update(deltaTime);
    }
    
    void event(gl::GLWindow *win, SDL_Event &e) override {}
    void update(float deltaTime) {}
private:
    mx::Font font;
    gl::GLSprite image1,  image2;
    gl::ShaderProgram shader1, shader2;
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
