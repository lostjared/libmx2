#include"mx.hpp"
#include"argz.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <GLES3/gl3.h>
#endif

#include"gl.hpp"
#include"loadpng.hpp"
#include"model.hpp"
#ifdef DEBUG_MODE
#define CHECK_GL_ERROR() \
{ GLenum err = glGetError(); \
if (err != GL_NO_ERROR) \
printf("OpenGL Error: %d at %s:%d\n", err, __FILE__, __LINE__); }
#else
#define CHECK_GL_ERROR()
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef __EMSCRIPTEN__
const char *vertSource = R"(#version 300 es
precision highp float;

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";
#else
const char *vertSource = R"(#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";
#endif
#ifdef __EMSCRIPTEN__
const char *fragSource = R"(#version 300 es
precision highp float;
in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;

uniform sampler2D texture1;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform float time_f;
void main() {
    // Ambient lighting
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * vec3(1.0, 1.0, 1.0);
    
    // Diffuse lighting
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0) - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);
    
    // Specular lighting
    float specularStrength = 0.5;
    vec3 viewDir = normalize(vec3(0.0, 0.0, 3.0) - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = specularStrength * spec * vec3(1.0, 1.0, 1.0);
    
    // Texture sampling
    
    vec2 uv = TexCoord * 2.0 - 1.0;
    float len = length(uv);
    float bubble = smoothstep(0.8, 1.0, 1.0 - len);
    vec2 distort = uv * (1.0 + 0.1 * sin(time_f + len * 20.0));
    vec4 texColor = texture(texture1, distort * 0.5 + 0.5);
    FragColor = mix(texColor, vec4(1.0, 1.0, 1.0, 1.0), bubble);
}
)";
#else
const char *fragSource = R"(#version 330 core
in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;

uniform sampler2D texture1;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform float time_f;
void main() {
    // Ambient lighting
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * vec3(1.0, 1.0, 1.0);
    
    // Diffuse lighting
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0) - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);
    
    // Specular lighting
    float specularStrength = 0.5;
    vec3 viewDir = normalize(vec3(0.0, 0.0, 3.0) - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = specularStrength * spec * vec3(1.0, 1.0, 1.0);
    
    // Texture sampling
    
    vec2 uv = TexCoord * 2.0 - 1.0;
    float len = length(uv);
    float bubble = smoothstep(0.8, 1.0, 1.0 - len);
    vec2 distort = uv * (1.0 + 0.1 * sin(time_f + len * 20.0));
    vec4 texColor = texture(texture1, distort * 0.5 + 0.5);
    FragColor = mix(texColor, vec4(1.0, 1.0, 1.0, 1.0), bubble);
}
)";
#endif

class Intro : public gl::GLObject {
public:
    Intro() = default;
    virtual ~Intro() override {}

    void load(gl::GLWindow *win) override {
        font.loadFont(win->util.getFilePath("data/font.ttf"), 36);
        if(!cube.openModel(win->util.getFilePath("data/star.mxmod.z"))) {
            throw mx::Exception("Error loading model");
        }
        if(!shader_program.loadProgramFromText(vertSource, fragSource)) {
            throw mx::Exception("Error loading shader");
        }
        cube.setTextures(win, win->util.getFilePath("data/cube.tex"), win->util.getFilePath("data"));
        cube.setShaderProgram(&shader_program, "texture1");
    }

    void draw(gl::GLWindow *win) override {
        glEnable(GL_DEPTH_TEST);
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; 
        lastUpdateTime = currentTime;
        update(deltaTime);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f));
        glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, depth));
        float aspectRatio = static_cast<float>(win->w) / static_cast<float>(win->h);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);
        shader_program.useProgram();
        shader_program.setUniform("model", model);
        shader_program.setUniform("view", view);
        shader_program.setUniform("projection", projection);
        static float time_f = 0.0f;
        time_f += deltaTime;
        shader_program.setUniform("time_f", time_f);
        cube.drawArrays();
        glDisable(GL_DEPTH_TEST);
        win->text.setColor({255, 255, 255, 255});
        win->text.printText_Solid(font, 15.0f, 15.0f, "https://lostsidedead.biz");
    }
    
    void event(gl::GLWindow *win, SDL_Event &e) override {
        if(e.type == SDL_KEYDOWN) {
            if(e.key.keysym.sym == SDLK_LEFT) {
                star_dir = false;
            } else if(e.key.keysym.sym == SDLK_RIGHT) {
                star_dir = true;
            }
        }
    }
    void update(float deltaTime) {  

        if(deltaTime > 0.1f)
            deltaTime = 0.1f;

        if(star_dir == false)
            angle -= (45.0f) * 16 * deltaTime; 
        else
            angle += (45.0f) * 16 * deltaTime;

        if (angle >= 360.0f) 
            angle -= 360.0f;
        if(angle <= 0.0f)
            angle += 360.0f;

        static bool dir = true;
        if(dir == true) {
            depth += 2.0f * deltaTime;
            if(depth > 0.0f) {
                dir = false;
            }
        } else {
            depth -= 2.0f * deltaTime;
            if(depth < -10.0f) {
                dir = true;
            }
        }
    }
private:
    mx::Font font;
    mx::Model cube;
    gl::ShaderProgram shader_program;
    Uint32 lastUpdateTime = SDL_GetTicks();
    float angle = 0.0f;
    float depth = -10.0f;
    bool star_dir = false;

};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("Skeleton", tw, th) {
        setPath(path);
        setObject(new Intro());
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
