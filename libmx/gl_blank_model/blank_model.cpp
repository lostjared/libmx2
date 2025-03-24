#include"mx.hpp"
#include"argz.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <GLES3/gl3.h>
#endif

#include"gl.hpp"
#include"loadpng.hpp"
#include"model.hpp"

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
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char *fSource = R"(#version 300 es
precision highp float;

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;

uniform sampler2D texture1;

void main()
{
    vec4 texColor = texture(texture1, TexCoord);
    
    // Basic lighting
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);
    vec3 ambient = vec3(0.3, 0.3, 0.3);
    
    vec3 result = (ambient + diffuse) * texColor.rgb;
    FragColor = vec4(result, texColor.a);
}
)";
#else
const char *vSource = R"(#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char *fSource = R"(#version 330 core

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;

uniform sampler2D texture1;

void main()
{
    vec4 texColor = texture(texture1, TexCoord);
    
    // Basic lighting
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);
    vec3 ambient = vec3(0.3, 0.3, 0.3);
    
    vec3 result = (ambient + diffuse) * texColor.rgb;
    FragColor = vec4(result, texColor.a);
}
)";
#endif
class Game : public gl::GLObject {
public:
    Game() = default;
    virtual ~Game() override {}

    void load(gl::GLWindow *win) override {
        font.loadFont(win->util.getFilePath("data/font.ttf"), 20);
        if(!shader_program.loadProgramFromText(vSource, fSource)) {
            throw mx::Exception("Failed to load shader program");
        }
        if(!object_file.openModel(win->util.getFilePath("data/cube.mxmod.z"))) {
            throw mx::Exception("Failed to load model");
        }

        object_file.setTextures(win, win->util.getFilePath("data/bg.tex"), win->util.getFilePath("data"));
        object_file.setShaderProgram(&shader_program, "texture1");
        shader_program.useProgram();
    }

    void draw(gl::GLWindow *win) override {
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; 
        lastUpdateTime = currentTime;
        
        glEnable(GL_DEPTH_TEST);
        shader_program.useProgram();
        glm::mat4 projection = glm::perspective(45.0f, (float)win->w / (float)win->h, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(2.0f, 2.0f, 2.0f));
        model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 1.0f, 0.0f));
        shader_program.setUniform("projection", projection);            
        shader_program.setUniform("view", view);
        shader_program.setUniform("model", model);
        shader_program.setUniform("texture1", 0);
        object_file.drawArrays();
        glDisable(GL_DEPTH_TEST);
        win->text.setColor({255, 255, 255, 255});
        win->text.printText_Solid(font, 25.0f, 25.0f, "FPS: " + calculateFPS());
        update(deltaTime);
    }
    
    void event(gl::GLWindow *win, SDL_Event &e) override {}
    void update(float deltaTime) {
        angle += 25.0f * deltaTime;
        if(angle > 360) 
            angle -= 360;
    }
private:
    float angle = 45.0f;
    mx::Font font;
    Uint32 lastUpdateTime = SDL_GetTicks();
    mx::Model object_file;
    gl::ShaderProgram shader_program;

    std::string calculateFPS() {
        static int frameCount = 0;
        static float timeElapsed = 0.0f;
        static float current_FPS = 0.0f;
        static Uint32 last_update_time = SDL_GetTicks();
        frameCount++;
        Uint32 currentTime = SDL_GetTicks();
        timeElapsed = (currentTime - last_update_time) / 1000.0f;
        
        if (timeElapsed >= 1.0f) {
            current_FPS = frameCount / timeElapsed;
            frameCount = 0;
            last_update_time = currentTime;
        }
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << current_FPS;
        return ss.str();
    }
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("Blank Model", tw, th) {
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
