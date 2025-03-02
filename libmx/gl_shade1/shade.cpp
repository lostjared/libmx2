// Phong shading model

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

#if defined(__EMSCRIPTEN__) || defined(__ANDOIRD__)
const char *g_vSource = R"(#version 300 es
precision highp float;
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform vec3 objectColor;

out vec3 vertexColor;
out vec2 TexCoords;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    gl_Position = projection * view * worldPos;
    vec3 norm = normalize(mat3(transpose(inverse(model))) * aNormal);
    vec3 lightDir = normalize(lightPos - vec3(worldPos));
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    float specularStrength = 1.0;    // Increase from 0.5 to 1.0 for more shine
    float shininess = 64.0;          // Increase this value for a tighter, shinier highlight
    vec3 viewDir = normalize(viewPos - vec3(worldPos));
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;
    vec3 finalColor = (ambient + diffuse + specular) * objectColor;
    vertexColor = finalColor;
    TexCoords = aTexCoords;
}
)";

const char *g_fSource = R"(#version 300 es
precision highp float;
in vec3 vertexColor;
in vec2 TexCoords;
uniform sampler2D texture1;  
out vec4 FragColor;

void main()
{
    FragColor = vec4(vertexColor, 1.0) * texture(texture1, TexCoords);
}
)";
#else
const char *g_vSource = R"(#version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;
    layout (location = 2) in vec2 aTexCoords;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    uniform vec3 lightPos;
    uniform vec3 viewPos;
    uniform vec3 lightColor;
    uniform vec3 objectColor;

    out vec3 vertexColor;
    out vec2 TexCoords;

    void main()
    {
        vec4 worldPos = model * vec4(aPos, 1.0);
        gl_Position = projection * view * worldPos;
        vec3 norm = normalize(mat3(transpose(inverse(model))) * aNormal);
        vec3 lightDir = normalize(lightPos - vec3(worldPos));
        float ambientStrength = 0.1;
        vec3 ambient = ambientStrength * lightColor;
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lightColor;
        float specularStrength = 1.0;    // Increase from 0.5 to 1.0 for more shine
        float shininess = 64.0;          // Increase this value for a tighter, shinier highlight
        vec3 viewDir = normalize(viewPos - vec3(worldPos));
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
        vec3 specular = specularStrength * spec * lightColor;
        vec3 finalColor = (ambient + diffuse + specular) * objectColor;
        vertexColor = finalColor;
        TexCoords = aTexCoords;
    }

)";
const char *g_fSource = R"(#version 330 core
        in vec3 vertexColor;
        in vec2 TexCoords;
        uniform sampler2D texture1;  
        out vec4 FragColor;

        void main()
        {
            FragColor = vec4(vertexColor, 1.0) * texture(texture1, TexCoords);
        }
)";
#endif

class Game : public gl::GLObject {
public:
    Game() = default;
    virtual ~Game() override {}

    void load(gl::GLWindow *win) override {
        if(!shade_program.loadProgramFromText(g_vSource, g_fSource)) {
            throw mx::Exception("Failed to load shader program");
        }
        if(!obj.openModel(win->util.getFilePath("data/saturn.mxmod.z"), true)) {
            throw mx::Exception("Error loading model...");
        }   
        obj.setTextures(win, win->util.getFilePath("data/planet.tex"), win->util.getFilePath("data"));
    }

    void draw(gl::GLWindow *win) override {
        glEnable(GL_DEPTH_TEST);
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; 
        lastUpdateTime = currentTime;

        
        static float rotationAngle = 0.0f;
        rotationAngle += deltaTime * 30.0f; 

        
        cameraPosition = glm::vec3(0.0f, 2.0f, 5.0f);
        glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);

        
        modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationAngle), glm::vec3(1.0f, 1.0f, 0.0f));
        
        viewMatrix = glm::lookAt(cameraPosition, cameraTarget, upVector);
        
        float aspectRatio = static_cast<float>(win->w) / static_cast<float>(win->h);
        projectionMatrix = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);

        lightPos = glm::vec3(0.0f, 5.0f, 0.0f); 

        obj.setShaderProgram(&shade_program, "texture1");
        shade_program.useProgram();

        
        glUniformMatrix4fv(glGetUniformLocation(shade_program.id(), "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shade_program.id(), "view"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shade_program.id(), "projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));

        
        glUniform3f(glGetUniformLocation(shade_program.id(), "lightPos"), lightPos.x, lightPos.y, lightPos.z);
        glUniform3f(glGetUniformLocation(shade_program.id(), "viewPos"), cameraPosition.x, cameraPosition.y, cameraPosition.z);
        glUniform3f(glGetUniformLocation(shade_program.id(), "lightColor"), 1.0f, 1.0f, 1.0f);
        glUniform3f(glGetUniformLocation(shade_program.id(), "objectColor"), 1.0f, 1.0f, 1.0f); 

        obj.drawArrays();
        update(deltaTime);
    }
    
    void event(gl::GLWindow *win, SDL_Event &e) override {}
    void update(float deltaTime) {}
private:
    gl::ShaderProgram shade_program;
    mx::Model obj;
    Uint32 lastUpdateTime = SDL_GetTicks();
    glm::mat4 modelMatrix{1.0f};
    glm::mat4 viewMatrix{1.0f};
    glm::mat4 projectionMatrix{1.0f};
    glm::vec3 cameraPosition{0.0f, 2.0f, 5.0f};
    glm::vec3 lightPos{2.0f, 2.0f, 2.0f};
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
