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
    FragColor = texColor;
}
)";

class Game : public gl::GLObject {
    GLuint texture = 0;
    bool fill_polygon = false;
public:
    Game() = default;
    virtual ~Game() override {
        glDeleteTextures(1, &texture);
    }

    void load(gl::GLWindow *win) override {
        font.loadFont(win->util.getFilePath("data/font.ttf"), 36);
        
        if(!program.loadProgramFromText(vSource, fSource)) {
            throw mx::Exception("Error loading shader program...");
        }
        
        if(!model.openModel(win->util.getFilePath("data/model.mxmod.z"))) {
            throw mx::Exception("Error loading model: data/model.mxmod.z");
        }
        
        texture = gl::loadTexture(win->util.getFilePath("data/logo.png"));
        std::vector<GLuint> tex;
        tex.push_back(texture);
        model.setTextures(tex);
        model.setShaderProgram(&program, "texture1");
        model.saveOriginal();
        program.useProgram();
    }

    void draw(gl::GLWindow *win) override {
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; 
        lastUpdateTime = currentTime;
        program.useProgram();
        model.resetToOriginal();
        static float value = 1.0f;
        static int dir = 1;
        if(dir == 1) {
            value += 0.05f;
            if (value > 40.0f) {
                dir = 0;
                value = 1.0f;
            }
        } else {
            value -= 0.05f;
            if(value <= 1.0) {
                dir = 1;
                value = 1.0f;
            }
        }
        model.wave(mx::DeformAxis::X, value * 0.8f, 1.5);
        model.wave(mx::DeformAxis::Y, value * 0.6f, 1.8);
        model.wave(mx::DeformAxis::Z, value * 0.7f, 1.2);
        model.recalculateNormals();
        model.updateBuffers();
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)win->w / (float)win->h, 0.1f, 1000.0f);
        glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 90.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 model_mat = glm::mat4(1.0f);
        model_mat = glm::scale(model_mat, glm::vec3(0.8f, 0.8f, 0.8f));
        model_mat = glm::rotate(model_mat, (float)currentTime * 0.001f, glm::vec3(0.0f, 1.0f, 0.0f));
        program.setUniform("projection", projection);
        program.setUniform("view", view);
        program.setUniform("model", model_mat);
        program.setUniform("texture1", 0);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        if(fill_polygon)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        model.drawArrays();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        win->text.setColor({255, 255, 255, 255});
        win->text.printText_Solid(font, 25.0f, 25.0f, "Hello, World!");
        update(deltaTime);
        CHECK_GL_ERROR();
    }
    void event(gl::GLWindow *win, SDL_Event &e) override {

        if(e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_SPACE) {
            fill_polygon = !fill_polygon;
        }

    }
    void update(float deltaTime) {}
private:
    float angle = 45.0f;
    mx::Font font;
    Uint32 lastUpdateTime = SDL_GetTicks();
    mx::Model model;
    gl::ShaderProgram program;
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("Skeleton", tw, th, true, gl::GLMode::ES) {
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
    try {
        MainWindow main_window("/", 960, 720);
        main_w =&main_window;
        emscripten_set_main_loop(eventProc, 0, 1);
    } catch(const mx::Exception &e) {
        mx::system_err << "mx: Exception: " << e.text()  << "\n";
        mx::system_err.flush();
        exit(EXIT_FAILURE);
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
