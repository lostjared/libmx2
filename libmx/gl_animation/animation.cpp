#include"mx.hpp"
#include"argz.hpp"
#ifdef __EMSCRIPTEN__
#include"SDL_image.h"
#include <emscripten/emscripten.h>
#include <GLES3/gl3.h>
#endif
#include"animation.hpp"

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
            precision highp float;
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
        out vec4 FragColor;
        uniform sampler2D textTexture; 
        uniform float alpha;
        void main() {
            vec4 fcolor = texture(textTexture, TexCoord);
            FragColor = mix(fcolor, vec4(0.0, 0.0, 0.0, fcolor.a), alpha);
        }
    )";


const char *anifSource = R"(#version 300 es
        precision highp float;
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
    precision highp float;
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
    precision highp float;
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

const char *anifSource4 = R"(#version 300 es
precision highp float;
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;
float alpha = 1.0f;



float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

void main(void) {
    vec2 tc = TexCoord;
    vec2 uv = (tc * iResolution - 0.5 * iResolution) / iResolution.y;
    
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
    FragColor = vec4(sin(col * pingPong(time_f, 8.0) + 2.0), alpha);
})";

const char *anifSource5 = R"(#version 300 es
precision highp float;
out vec4 FragColor;
in vec2 TexCoord;
uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;

vec2 rotate(vec2 pos, float angle) {
    float s = sin(angle);
    float c = cos(angle);
    pos -= vec2(0.5);
    pos = mat2(c, -s, s, c) * pos;
    pos += vec2(0.5);
    return pos;
}

void main(void) {
    vec2 tc = TexCoord;
    vec2 pos = tc;
    float aspectRatio = iResolution.x / iResolution.y;
    pos.x *= aspectRatio;

    float spinSpeed = time_f * 0.5;
    pos = rotate(pos, spinSpeed);
    float dist = distance(pos, vec2(0.5 * aspectRatio, 0.5));
    float scale = 1.0 + 0.2 * sin(dist * 15.0 - time_f * 2.0);
    pos = (pos - vec2(0.5)) * scale + vec2(0.5);
    FragColor = texture(textTexture, pos);
}
)";

const char *anifSource6 = R"(#version 300 es
precision highp float;
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}


vec4 adjustHue(vec4 color, float angle) {
    float U = cos(angle);
    float W = sin(angle);
    mat3 rotationMatrix = mat3(
        0.299, 0.587, 0.114,
        0.299, 0.587, 0.114,
        0.299, 0.587, 0.114
    ) + mat3(
        0.701, -0.587, -0.114,
        -0.299, 0.413, -0.114,
        -0.3, -0.588, 0.886
    ) * U + mat3(
        0.168, 0.330, -0.497,
        -0.328, 0.035, 0.292,
        1.25, -1.05, -0.203
    ) * W;
    return vec4(rotationMatrix * color.rgb, color.a);
}

void main() {
    vec2 tc = TexCoord;
    vec4 color = FragColor;
    vec2 uv = (tc - 0.5) * iResolution / min(iResolution.x, iResolution.y);
    float dist = length(uv);
    float ripple = sin(dist * 12.0 - pingPong(time_f, 10.0) * 10.0) * exp(-dist * 4.0);
    vec4 sampledColor = texture(textTexture, tc + ripple * 0.01);
    float hueShift = pingPong(time_f, 10.0) * ripple * 2.0;
    color = adjustHue(sampledColor, hueShift);
    FragColor = color;
}

)";

const char *anifSource7 = R"(#version 300 es
precision highp float;
out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;

mat3 rotationMatrixX(float angle) {
    return mat3(
        1.0, 0.0, 0.0,
        0.0, cos(angle), -sin(angle),
        0.0, sin(angle), cos(angle)
    );
}

mat3 rotationMatrixY(float angle) {
    return mat3(
        cos(angle), 0.0, sin(angle),
        0.0,       1.0, 0.0,
       -sin(angle), 0.0, cos(angle)
    );
}

mat2 rotationMatrixZ(float angle) {
    return mat2(
        cos(angle), -sin(angle),
        sin(angle),  cos(angle)
    );
}

vec2 vortexDistortion(vec2 uv, float time) {
    vec2 center = vec2(0.5, 0.5);
    vec2 offset = uv - center;
    float distance = length(offset);
    float angle = atan(offset.y, offset.x) + sin(time + distance * 10.0) * 0.5;
    return center + vec2(cos(angle), sin(angle)) * distance;
}

void main(void) {
    vec2 tc = TexCoord;
    vec4 color = FragColor;
    vec2 uv = tc;
    float phase = mod(time_f, 3.0);

    if (phase < 1.0) {
        float angle = time_f * 2.0;
        mat3 rotation = rotationMatrixX(angle);
        uv = (rotation * vec3(uv - 0.5, 1.0)).xy + 0.5;
    } else if (phase < 2.0) {
        float angle = time_f * 2.0;
        mat3 rotation = rotationMatrixY(angle);
        uv = (rotation * vec3(uv - 0.5, 1.0)).xy + 0.5;
    } else {
        float angle = time_f * 2.0;
        mat2 rotation = rotationMatrixZ(angle);
        uv = (rotation * (uv - 0.5)) + 0.5;
    }

    uv.x = mod(uv.x + sin(time_f) * 0.2, 1.0);
    uv.y = mod(uv.y + cos(time_f) * 0.2, 1.0);

    uv = vortexDistortion(uv, time_f);
    vec4 texColor = texture(textTexture, uv);
    color = texColor;
    FragColor = color;
})";

const char *anifSource8 = R"(#version 300 es
precision highp float;
uniform sampler2D textTexture;
uniform vec2 iResolution;
uniform float time_f;
in vec2 TexCoord;
out vec4 FragColor;

void main() {
    vec4 color = FragColor;
    vec2 tc = TexCoord;
    vec2 uv = gl_FragCoord.xy / iResolution;
    vec2 centeredUV = uv * 2.0 - 1.0;
    float angle = time_f * 0.5;
    mat2 rotation = mat2(cos(angle), -sin(angle), sin(angle), cos(angle));
    if (centeredUV.x < 0.0) {
        centeredUV.x = -centeredUV.x;
        centeredUV = rotation * centeredUV;
    } else {
        centeredUV = rotation * centeredUV;
    }
    centeredUV = mod(centeredUV, 1.0);
    vec2 mirroredUV = centeredUV * 0.5 + 0.5;
    vec4 texColor = texture(textTexture, mirroredUV);
    color = texColor;
    FragColor = color;
})";

const char *anifSource9 = R"(#version 300 es
precision highp float;
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D textTexture;
uniform float time_f;
uniform vec2 iResolution;

void main() {
    vec4 color = FragColor;
    vec2 tc = TexCoord;
    vec2 uv = tc * 2.0 - 1.0;
    uv *= iResolution.y / iResolution.x;

    vec2 center = vec2(0.0, 0.0);
    vec2 offset = uv - center;
    float dist = length(offset);
    float angle = atan(offset.y, offset.x);

    float pulse = sin(time_f * 2.0 + dist * 10.0) * 0.15;
    float spiral = sin(angle * 5.0 + time_f * 2.0) * 0.15;
    float stretch = cos(time_f * 3.0 - dist * 15.0) * 0.2;

    vec2 spiralOffset = vec2(cos(angle), sin(angle)) * spiral;
    vec2 stretchOffset = normalize(offset) * (pulse + stretch);
    vec2 morphUV = uv + spiralOffset + stretchOffset;

    vec4 texColor = texture(textTexture, morphUV * 0.5 + 0.5);

    vec3 blended = vec3(0.0);
    float weight = 0.0;

    for (float x = -2.0; x <= 2.0; x++) {
        for (float y = -2.0; y <= 2.0; y++) {
            vec2 sampleUV = morphUV + vec2(x, y) * 0.005;
            vec4 sampleColor = texture(textTexture, sampleUV * 0.5 + 0.5);
            blended += sampleColor.rgb;
            weight += 1.0;
        }
    }
    blended /= weight;
    blended = mix(texColor.rgb, blended, 0.5);
    float shimmer = sin(dist * 20.0 - time_f * 5.0) * 0.1 + 1.0;
    color = vec4(blended * shimmer, texColor.a);
    FragColor = color;
})";

const char *anifSource10 = R"(#version 300 es
precision highp float;
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
    vec4 color = FragColor;
    vec2 tc = TexCoord;
    vec2 center = vec2(0.5, 0.5);
    vec2 tc_centered = tc - center;
    float dist = length(tc_centered);
   vec2 dir = tc_centered / max(dist, 1e-6);
    float waveLength = 0.05;
    float amplitude = 0.02;
    float speed = 2.0;

    float ripple = sin((dist / waveLength - time_f * speed) * 6.2831853); // 2 * PI
    vec2 tc_displaced = tc + dir * ripple * amplitude;
    vec4 texColor = texture(textTexture, tc_displaced);
    color = texColor;
    FragColor = color;
})";

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

const char *aniSource4 = "";
const char *anifSource5 = "";
const char *anifSource6 = "";
const char *anifSource7 = "";
const char *anifSource8 = "";
const char *anifSource9 = "";
const char *anifSource10 = "";
#endif

bool done = false;
float the_time = 1.0f;
size_t shader_index = 0;
bool playing = false;

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
            done = true;
            return;
        }
    }
    void event(gl::GLWindow *win, SDL_Event &e) override {}

private:
    Uint32 lastUpdateTime;
    gl::GLSprite logo;
    gl::ShaderProgram program;
};

int new_width = 1920, new_height = 1080;

class MainWindow : public gl::GLWindow {
public:
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("Acid Cam Shader Animation", tw, th) {
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
        glViewport(0, 0, new_width, new_height);
        object->draw(this);
        swap();
        delay();
    }

    void setTime(float time) {
        if(done == true) {
            the_time = time;
        }
    }
    bool checkDone() { return done; }
    void inc() {
        if(shader_index < MAX_SHADER-1)
            shader_index ++;
    }
    void dec() {
        if(shader_index > 0) 
            shader_index --;
    }
    void play() {
        playing = true;
    }
    void stop() {
        playing = false;
    }
};

MainWindow *main_w = nullptr;
gl::GLSprite sprite;
gl::ShaderProgram shader[MAX_SHADER];

#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_BINDINGS(main_window_bindings) {
        class_<MainWindow>("MainWindow")
            .constructor<std::string, int, int>()
            .function("checkDone", &MainWindow::checkDone)
            .function("setTime", &MainWindow::setTime)
            .function("inc", &MainWindow::inc)
            .function("dec", &MainWindow::dec)
            .function("play", &MainWindow::play)
            .function("stop", &MainWindow::stop);
    }

    void loadImage(const std::vector<uint8_t>& imageData) {
        std::cout << "Received image data of size: " << imageData.size() << " bytes\n";
        std::ofstream outFile("image.png", std::ios::binary);
        outFile.write(reinterpret_cast<const char*>(imageData.data()), imageData.size());
        outFile.close();
        std::cout << "Saved image as 'image.png'.\n";
        SDL_Surface *surface = png::LoadPNG("image.png");
        int imgWidth = surface->w;
        int imgHeight = surface->h;
        main_w->setWindowSize(imgWidth, imgHeight);
        emscripten_set_canvas_element_size("#canvas", imgWidth, imgHeight);
        int canvasWidth = imgWidth, canvasHeight = imgHeight;
        emscripten_get_canvas_element_size("#canvas", &canvasWidth, &canvasHeight);
        new_width = canvasWidth;
        new_height = canvasHeight;
        sprite.initSize(canvasWidth, canvasHeight);
        main_w->text.init(canvasWidth, canvasHeight);
        sprite.loadTexture(&shader[shader_index], "image.png", 0, 0, canvasWidth, canvasHeight);
        SDL_FreeSurface(surface);
    }

    void loadImageJPG(const std::vector<uint8_t>& imageData) {
        std::ofstream outFile("image.jpg", std::ios::binary);
        outFile.write(reinterpret_cast<const char*>(imageData.data()), imageData.size());
        outFile.close();
        SDL_Surface *surface = IMG_Load("image.jpg");
        int imgWidth = surface->w;
        int imgHeight = surface->h;
        main_w->setWindowSize(imgWidth, imgHeight);
        emscripten_set_canvas_element_size("#canvas", imgWidth, imgHeight);
        int canvasWidth = imgWidth, canvasHeight = imgHeight;
        emscripten_get_canvas_element_size("#canvas", &canvasWidth, &canvasHeight);
        new_width = canvasWidth;
        new_height = canvasHeight;
        sprite.initSize(canvasWidth, canvasHeight);
        main_w->text.init(canvasWidth, canvasHeight);
        GLuint text = gl::createTexture(surface, true);
        sprite.initWithTexture(&shader[shader_index], text, 0, 0, canvasWidth, canvasHeight);
        SDL_FreeSurface(surface);
    }

EMSCRIPTEN_BINDINGS(image_loader) {
    emscripten::function("loadImage", &loadImage);
    emscripten::function("loadImageJPG",  &loadImageJPG);
    emscripten::register_vector<uint8_t>("VectorU8");
}
#endif

void eventProc() {
    main_w->proc();
}

int main(int argc, char **argv) {
#ifdef __EMSCRIPTEN__
    IMG_Init(IMG_INIT_JPG);
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
