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


const char *anifSource = R"(#version 300 es
        precision mediump float;
        out vec4 FragColor;
        in vec2 TexCoord;
        uniform sampler2D textTexture;
        uniform float time_f;
        float alpha = 1.0;
        uniform vec2 iResolution;
        float pingPong(float x, float length) {
            float modVal = mod(x, length * 2.0);
            return modVal <= length ? modVal : length * 2.0 - modVal;
        }
        void main(void) {
            vec2 uv = (TexCoord * iResolution - 0.5 * iResolution) / iResolution.y;
            float t = time_f * 0.7;
            float radius = length(uv);
            float angle = atan(uv.y, uv.x);
            float swirlAmount = sin(t) * 2.0;
            angle += swirlAmount * exp(-radius * 4.0);
            uv = vec2(cos(angle), sin(angle)) * radius;
            float wave = sin(radius * 10.0 + t * 6.0) * 0.05;
            uv += vec2(cos(angle), sin(angle)) * wave;
            vec3 texColor = texture(textTexture, uv * 0.5 + 0.5).rgb;
            float modAngle = angle + t * 0.5;
            vec3 swirlColor = vec3(
                sin(modAngle * 3.0),
                sin(modAngle * 2.0 + 1.0),
                sin(modAngle * 1.0 + 2.0)
            ) * 0.5 + 0.5;
            vec3 finalColor = mix(swirlColor, texColor, 0.7);
            float pulse = abs(sin(time_f * 3.14159)) * 0.2 + 0.8;
            finalColor *= pulse;
            FragColor = vec4(finalColor, 1.0);
        }
)";

const char *anifSource2 = R"(#version 300 es
    precision mediump float;
    out vec4 FragColor;
    in vec2 TexCoord;

    uniform sampler2D textTexture;
    uniform float time_f;
    uniform vec2 iResolution;


    float pingPong(float x, float length) {
        float modVal = mod(x, length * 2.0);
        return modVal <= length ? modVal : length * 2.0 - modVal;
    }

    void main(void) {
        vec2 uv = (TexCoord * iResolution - 0.5 * iResolution) / iResolution.y;
        float t = time_f * 0.5;
        float radius = length(uv);
        float angle = atan(uv.y, uv.x);
        angle += t;
        float radMod = pingPong(radius + t * 0.5, 0.5);
        float wave = sin(radius * 10.0 - t * 5.0) * 0.5 + 0.5;
        float r = sin(angle * 3.0 + radMod * 10.0 + wave * 6.2831);
        float g = sin(angle * 4.0 - radMod * 8.0  + wave * 4.1230);
        float b = sin(angle * 5.0 + radMod * 12.0 - wave * 3.4560);
        vec3 col = vec3(r, g, b) * 0.5 + 0.5;
        vec3 texColor = texture(textTexture, TexCoord).rgb;
        col = mix(col, texColor, 0.3);
        FragColor = vec4(col, 1.0);
    }
    )";

const char *anifSource3 = R"(#version 300 es
    precision mediump float;
    in vec2 TexCoord;
    out vec4 FragColor;
    uniform sampler2D textTexture;
    uniform float time_f;
    uniform vec2 iResolution;

    void main(void) {
        vec2 tc = TexCoord;
        vec2 res = iResolution;
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
        FragColor = twistedRippleColor;
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
 
const char *anifSource = R"(#version 330 core
        out vec4 FragColor;
        in vec2 TexCoord;
        uniform sampler2D textTexture;
        uniform float time_f;

        float alpha = 1.0;

        uniform vec2 iResolution;
        float pingPong(float x, float length) {
            float modVal = mod(x, length * 2.0);
            return modVal <= length ? modVal : length * 2.0 - modVal;
        }
        void main(void) {
            vec2 uv = (TexCoord * iResolution - 0.5 * iResolution) / iResolution.y;
            float t = time_f * 0.7;
            float radius = length(uv);
            float angle = atan(uv.y, uv.x);
            float swirlAmount = sin(t) * 2.0;
            angle += swirlAmount * exp(-radius * 4.0);
            uv = vec2(cos(angle), sin(angle)) * radius;
            float wave = sin(radius * 10.0 + t * 6.0) * 0.05;
            uv += vec2(cos(angle), sin(angle)) * wave;
            vec3 texColor = texture(textTexture, uv * 0.5 + 0.5).rgb;
            float modAngle = angle + t * 0.5;
            vec3 swirlColor = vec3(
                sin(modAngle * 3.0),
                sin(modAngle * 2.0 + 1.0),
                sin(modAngle * 1.0 + 2.0)
            ) * 0.5 + 0.5;
            vec3 finalColor = mix(swirlColor, texColor, 0.7);
            float pulse = abs(sin(time_f * 3.14159)) * 0.2 + 0.8;
            finalColor *= pulse;
            FragColor = vec4(finalColor, 1.0);
        }
)";

const char *anifSource2 = R"(#version 330 core

out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;


float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

void main(void) {
    vec2 uv = (TexCoord * iResolution - 0.5 * iResolution) / iResolution.y;
    float t = time_f * 0.5;
    float radius = length(uv);
    float angle = atan(uv.y, uv.x);
    angle += t;
    float radMod = pingPong(radius + t * 0.5, 0.5);
    float wave = sin(radius * 10.0 - t * 5.0) * 0.5 + 0.5;
    float r = sin(angle * 3.0 + radMod * 10.0 + wave * 6.2831);
    float g = sin(angle * 4.0 - radMod * 8.0  + wave * 4.1230);
    float b = sin(angle * 5.0 + radMod * 12.0 - wave * 3.4560);
    vec3 col = vec3(r, g, b) * 0.5 + 0.5;
    vec3 texColor = texture(textTexture, TexCoord).rgb;
    col = mix(col, texColor, 0.3);
    FragColor = vec4(col, 1.0);
}
)";

const char *anifSource3 = R"(#version 330
    in vec2 TexCoord;
    out vec4 FragColor;
    uniform sampler2D textTexture;
    uniform float time_f;
    uniform vec2 iResolution;

    void main(void) {
        vec2 tc = TexCoord;
        vec2 res = iResolution;
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
        FragColor = twistedRippleColor;
}
)";

#endif

class Animation : public gl::GLObject {
public:
    
     Animation() = default;
    ~Animation() override {}

    void load(gl::GLWindow *win) override {
        font.loadFont(win->util.getFilePath("data/font.ttf"), 24);
        if(!shader[0].loadProgramFromText(vSource, anifSource)) {
            throw mx::Exception("Could not load shader");
        }
        if(!shader[1].loadProgramFromText(vSource, anifSource2)) {
            throw mx::Exception("Could not load shader");
        }
        if(!shader[2].loadProgramFromText(vSource, anifSource3)) {
            throw mx::Exception("Could not load shader");
        }
        for(int i = 0; i < MAX_SHADER; ++i) {
            shader[i].useProgram();
            shader[i].setUniform("iResolution", glm::vec2(win->w, win->h));
            shader[i].setUniform("time_f", static_cast<float>(SDL_GetTicks()));
        }
        sprite.initSize(win->w, win->h);
        sprite.loadTexture(&shader[0], win->util.getFilePath("data/bg.png"), 0, 0, win->w, win->h);
    }

    void draw(gl::GLWindow *win) override {
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; // Convert to seconds
        lastUpdateTime = currentTime;
        update(deltaTime);
        sprite.setShader(&shader[index]);
        shader[index].useProgram();
        shader[index].setUniform("time_f", static_cast<float>(currentTime / 1000.0f));
        sprite.draw();
        win->text.setColor({255,255,255,255});
        win->text.printText_Solid(font, 25, 25, "Press Up and Down Arrows Or Tap to Switch Shaders");
    }
    void event(gl::GLWindow *win, SDL_Event &e) override {
        if(e.type == SDL_KEYDOWN) {
            switch(e.key.keysym.sym) {
                case SDLK_UP:
                if(index > 0) index--;
                break;
                case SDLK_DOWN:
                if(index < MAX_SHADER-1) index++;
                break;
            }
        } else if(e.type == SDL_FINGERUP || e.type == SDL_MOUSEBUTTONUP) {
            index ++;
            if(index > MAX_SHADER-1)
                index = 0;
        }
    }
    void update(float deltaTime) {}
private:
    Uint32 lastUpdateTime = SDL_GetTicks();
    mx::Font font;
    gl::GLSprite sprite;
    static constexpr int MAX_SHADER = 3;
    gl::ShaderProgram shader[MAX_SHADER];
    size_t index = 0;
};



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
            win->setObject(new Animation());
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
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("GL Shader Animation", tw, th) {
        setPath(path);
        setObject(new IntroScreen());
        object->load(this);
    }
    
    ~MainWindow() override {}
    
    virtual void event(SDL_Event &e) override {
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
    MainWindow main_window("", 1920, 1080);
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
    int tw = 1920, th = 1080;
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
