/* 
Spinning Kaleidoscope Star 
written by Jared Bruni
2025 
*/

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
        float ambientStrength = 0.8; 
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
    const char *fSource = R"(#version 300 es
precision highp float;
in vec2 TexCoords;
out vec4 color;

uniform float time_f;
uniform sampler2D texture1;
uniform vec2 iResolution;

float pi = 3.14159265358979323846264;

float mirror1(float x) {
    return abs(fract(x * 0.5) * 2.0 - 1.0);
}
vec2 mirror2(vec2 uv) {
    return vec2(mirror1(uv.x), mirror1(uv.y));
}

void main(void) {
    vec2 tc = TexCoords;
    vec2 p = tc * 2.0 - 1.0;
    p.x *= iResolution.x / iResolution.y;

    float n = 10.0 + 6.0 * sin(time_f * 0.25);
    float seg = 2.0 * pi / n;

    float a = atan(p.y, p.x) + time_f * 0.15;
    a = mod(a, seg);
    a = abs(a - seg * 0.5);

    float r = length(p);
    r += 0.08 * sin(8.0 * a + time_f) + 0.05 * sin(6.0 * r - time_f * 1.2);

    vec2 q = vec2(cos(a), sin(a)) * r;
    q.x /= iResolution.x / iResolution.y;

    vec2 uvM = q * 0.5 + 0.5;

    float s = 0.3 * sin(time_f * 0.6);
    vec2 c = vec2(0.5);
    vec2 d = uvM - c;
    float rr = length(d);
    float th = atan(d.y, d.x) + s * exp(-rr * 3.0);
    vec2 swirl = c + vec2(cos(th), sin(th)) * rr;

    vec2 uvMand = mirror2(swirl);
    vec2 uvOrig = mirror2(tc);

    vec4 mand = texture(texture1, uvMand);
    vec4 orig = texture(texture1, uvOrig);
    color = mix(orig, mand, 0.85);
}

    )";

class Game : public gl::GLObject {
public:
    Game() = default;
    virtual ~Game() override {}

    void load(gl::GLWindow *win) override {
        font.loadFont(win->util.getFilePath("data/font.ttf"), 36);
        if(!shader.loadProgramFromText(vSource, fSource)) {
            throw mx::Exception("Failed to load shader program");
        }
        if(!model.openModel(win->util.getFilePath("data/star.mxmod"))) {
            throw mx::Exception("Failed to load model");
        }
        model.setTextures(win, win->util.getFilePath("data/metal.tex"), win->util.getFilePath("data"));
        model.setShaderProgram(&shader, "texture1");
        shader.useProgram();
    }
    glm::mat4 modelMatrix{1.0f};
    glm::mat4 viewMatrix{1.0f};
    glm::mat4 projectionMatrix{1.0f};
    glm::vec3 cameraPosition{0.0f, 2.0f, 5.0f};
    glm::vec3 lightPos{0.0f, 5.0f, 0.0f};
    float rot[3] = {1.0f, 0.0f, 0.f};    

    void draw(gl::GLWindow *win) override {
        glEnable(GL_DEPTH_TEST);
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; 
        lastUpdateTime = currentTime;
        static float rotationAngle = 0.0f;
        rotationAngle += deltaTime * 720.0f;  
        model.setShaderProgram(&shader, "texture1");
        shader.useProgram();
        
        cameraPosition = glm::vec3(0.0f, 2.0f, zoom);
        glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);
        
        modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::scale(modelMatrix, glm::vec3(2.0f, 2.0f, 2.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationAngle), glm::vec3(rot[0], rot[1], rot[2]));
        viewMatrix = glm::lookAt(cameraPosition, cameraTarget, upVector);
        
        float aspectRatio = 1.0f; 
        if (win->h > 0) {
            aspectRatio = static_cast<float>(win->w) / static_cast<float>(win->h);
        }
        projectionMatrix = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);      
        
        glUniformMatrix4fv(glGetUniformLocation(shader.id(), "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shader.id(), "view"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shader.id(), "projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
        glUniform3f(glGetUniformLocation(shader.id(), "lightPos"), lightPos.x, lightPos.y, lightPos.z);
        glUniform3f(glGetUniformLocation(shader.id(), "viewPos"), cameraPosition.x, cameraPosition.y, cameraPosition.z);
        glUniform3f(glGetUniformLocation(shader.id(), "lightColor"), 2.0f, 2.0f, 2.0f);
        glUniform3f(glGetUniformLocation(shader.id(), "objectColor"), 1.0f, 1.0f, 1.0f); 
        glUniform2f(glGetUniformLocation(shader.id(), "iResolution"), 
                    static_cast<float>(win->w), static_cast<float>(win->h));
        glUniform1f(glGetUniformLocation(shader.id(), "time_f"), 
                static_cast<float>(currentTime) / 1000.0f);

    
        CHECK_GL_ERROR();
        glDisable(GL_BLEND);
        model.drawArrays();
        update(deltaTime);
    }

    float zoom = 5.0f;
    
    void event(gl::GLWindow *win, SDL_Event &e) override {
        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_LEFT:
                    rot[0]= 0.0f;
                    break;
                case SDLK_RIGHT:
                    rot[0] = 1.0f;
                    break;
                case SDLK_a:
                    rot[1] = 0.0f;
                    break;
                case SDLK_s:
                    rot[1] = 1.0f;
                    break;
                case SDLK_w:
                    rot[2] = 0.0f;
                    break;
                case SDLK_d:
                    rot[2] = 1.0f;
                    break;
                case SDLK_UP:
                    zoom -= 0.5f;
                    if (zoom < 0.5f) zoom = 0.5f;
                    break;
                case SDLK_DOWN:
                    zoom += 0.5f;
                    if (zoom > 20.0f) zoom = 20.0f;
                    break;
                default:
                    break;
            }
        }
        if (e.type == SDL_FINGERDOWN || e.type == SDL_FINGERMOTION) {
            static float lastY = 0.0f;
            if (e.type == SDL_FINGERDOWN) {
                lastY = e.tfinger.y;
            } else if (e.type == SDL_FINGERMOTION) {
                float dy = e.tfinger.y - lastY;
                lastY = e.tfinger.y;
                zoom -= dy * 10.0f; 
                if (zoom < 0.5f) zoom = 0.5f;
                if (zoom > 20.0f) zoom = 20.0f;
            }
        }
    }
    void update(float deltaTime) {}
private:
    mx::Font font;
    Uint32 lastUpdateTime = SDL_GetTicks();
    gl::ShaderProgram shader;
    mx::Model  model;
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("Diamond", tw, th) {
        setPath(path);
        setObject(new Game());
        object->load(this);
    }
    
    ~MainWindow() override {}
    
    virtual void event(SDL_Event &e) override {}
    
    const Uint32 FPS = 60;
    const Uint32 frameDelay = 1000 / FPS;

    virtual void draw() override {
        Uint32 frameStart = SDL_GetTicks();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glViewport(0, 0, w, h);
        object->draw(this);
        swap();
        Uint32 frameTime = SDL_GetTicks() - frameStart;
        if (frameTime < frameDelay) {
            SDL_Delay(frameDelay - frameTime);
        }
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
