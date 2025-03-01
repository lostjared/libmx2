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

#if defined(__EMSCRIPTEN__) || defined(__ANDROID__)
const char *fSource = R"(#version 300 es
precision highp float;
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
#else
const char *fSource = R"(#version 330 core
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
#endif

class Game : public gl::GLObject {
public:
    Game() = default;
    virtual ~Game() override {}
    int width = 0;
    int height = 0;
    bool snap = false;

    void load(gl::GLWindow *win) override {
        std::cout << "Press Return to Save Screenshot\n";
        program.loadProgramFromText(gl::vSource, fSource);
        font.loadFont(win->util.getFilePath("data/font.ttf"), 36);
        image.initSize(win->w, win->h);
        image.loadTexture(&program, win->util.getFilePath("data/bg.png"), 0.0f, 0.0f, win->w, win->h);
        program.useProgram();
        glUniform2f(glGetUniformLocation(program.id(), "iResolution"), win->w, win->h);

    }

    void flipPixelData(unsigned char* data, int width, int height, int channels) {
        int rowSize = width * channels;
        unsigned char* tempRow = new unsigned char[rowSize];
        for (int y = 0; y < height / 2; ++y) {
            unsigned char* topRow = data + y * rowSize;
            unsigned char* bottomRow = data + (height - 1 - y) * rowSize;
            memcpy(tempRow, topRow, rowSize);
            memcpy(topRow, bottomRow, rowSize);
            memcpy(bottomRow, tempRow, rowSize);
        }
        delete[] tempRow;
    }
    
    void draw(gl::GLWindow *win) override {
        glDisable(GL_DEPTH_TEST);
        width = win->w;
        height = win->h;
        GLuint fbo, texture;
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Error: Framebuffer is not complete!" << std::endl;
            return;
        }
        glViewport(0, 0, width, height);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; 
        lastUpdateTime = currentTime;
        program.useProgram();
        static float time_f = 0.0f;
        time_f += deltaTime;
        program.setUniform("time_f", time_f);
        image.draw();        
        win->text.setColor({255, 255, 255, 255});
        win->text.printText_Solid(font, 25.0f, 25.0f, "Hello, World! from OpenGL: " + std::to_string(currentTime));
        update(deltaTime);
        if (snap == true) {
            static int index = 1;
            unsigned char* pixelData = new unsigned char[width * height * 4]; 
            glBindTexture(GL_TEXTURE_2D, texture);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
            flipPixelData(pixelData, width, height, 4);
            png::SavePNG_RGBA(std::string("screenshot-" + std::to_string(index++) + ".png").c_str(), pixelData, width, height);
            mx::system_out << "Saved snapshot...\n";
            delete[] pixelData;
            snap = false;
        }
        
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteTextures(1, &texture);
        glDeleteFramebuffers(1, &fbo);
    }
    
    
    void event(gl::GLWindow *win, SDL_Event &e) override {
        switch(e.type) {
            case SDL_KEYUP:
            if(e.key.keysym.sym == SDLK_RETURN)
                snap = true;
            break;
        }


    }
    void update(float deltaTime) {}
private:
    mx::Font font;
    gl::ShaderProgram program;
    gl::GLSprite image;
    Uint32 lastUpdateTime = SDL_GetTicks();
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("Skeleton [ Press Enter to take Screenshot ]", tw, th) {
        setPath(path);
        setObject(new Game());
        object->load(this);
    }
    
    ~MainWindow() override {}
    
    virtual void event(SDL_Event &e) override {}
    
    virtual void draw() override {
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
