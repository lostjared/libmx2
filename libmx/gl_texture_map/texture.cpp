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

const char *fSource = R"(#version 300 es
precision highp float;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform sampler2D texture1;


uniform vec3 viewPos = vec3(0.0, 0.0, 5.0); // Camera position
uniform vec3 lightPos = vec3(3.0, 3.0, 5.0); // Light position
uniform vec3 lightColor = vec3(1.0, 1.0, 1.0); // White light


uniform float ambientStrength = 0.3;
uniform float diffuseStrength = 0.7;
uniform float specularStrength = 0.5;
uniform float shininess = 32.0;

out vec4 FragColor;

void main() {

    vec4 texColor = texture(texture1, TexCoord);
    vec3 ambient = ambientStrength * lightColor;
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diffuseStrength * diff * lightColor;
    
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;
    
    float distance = length(lightPos - FragPos);
    float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
    
    vec3 lighting = (ambient + diffuse + specular) * attenuation;
    FragColor = vec4(lighting * texColor.rgb, texColor.a);
}
)";

const char *vSource = R"(#version 300 es
precision highp float;

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;  // Properly transform normals
    TexCoord = aTexCoord;
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

#else

const char *fSource = R"(#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform sampler2D texture1;
uniform vec2 iResolution;
uniform float time;
uniform vec3 viewPos = vec3(0.0, 0.0, 5.0); // Camera position
uniform vec3 lightPos = vec3(3.0, 3.0, 5.0); // Light position
uniform vec3 lightColor = vec3(1.0, 1.0, 1.0); // White light

uniform float ambientStrength = 0.3;
uniform float diffuseStrength = 0.7;
uniform float specularStrength = 0.5;
uniform float shininess = 32.0;


out vec4 FragColor;

vec4 distort(vec2 tc, sampler2D samp, float time_f) {
    vec2 normPos = (gl_FragCoord.xy / iResolution.xy) * 2.0 - 1.0;
    float dist = length(normPos);
    float phase = sin(dist * 10.0 - time_f * 4.0);
    vec2 tcAdjusted = tc + (normPos * 0.305 * phase);
    float dispersionScale = 0.02;
    vec2 dispersionOffset = normPos * dist * dispersionScale;
    vec2 tcAdjustedR = tcAdjusted + dispersionOffset * (-1.0);
    vec2 tcAdjustedG = tcAdjusted;
    vec2 tcAdjustedB = tcAdjusted + dispersionOffset * 1.0;
    float r = texture(samp, tcAdjustedR).r;
    float g = texture(samp, tcAdjustedG).g;
    float b = texture(samp, tcAdjustedB).b;
    vec4 color;
    color = vec4(r, g, b, 1.0);
    return color;
}
    
vec4 distort2(vec2 tc, sampler2D samp, float time_f) {
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
    vec4 originalColor = texture(samp, tc);
    return originalColor;
}

void main() {
    vec4 texColor = texture(texture1, TexCoord);
    vec3 ambient = ambientStrength * lightColor;
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diffuseStrength * diff * lightColor;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;
    float distance = length(lightPos - FragPos);
    float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
    vec3 lighting = (ambient + diffuse + specular) * attenuation;
    
    vec4 ctx = distort(TexCoord, texture1, time);
    
    vec4 litTexture = vec4(lighting * texColor.rgb, texColor.a);
    FragColor = mix(ctx, litTexture, 0.5);
    vec4 secondDistortion = distort2(TexCoord, texture1, time);
    FragColor = FragColor + secondDistortion * 0.3; // Reduced effect strength
}
)";
const char *vSource = R"(#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;  // Properly transform normals
    TexCoord = aTexCoord;
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

#endif

class Game : public gl::GLObject {
public:
    Game() = default;
    virtual ~Game() override {}

    void load(gl::GLWindow *win) override {
        font.loadFont(win->util.getFilePath("data/font.ttf"), 36);
        if(!shader_program.loadProgramFromText(vSource, fSource)) {
            throw mx::Exception("Failed to load shader program");
        }
        if(!model.openModel(win->util.getFilePath("data/star.mxmod.z"))) {
            throw mx::Exception("Failed to load model");
        }
        
        GLuint textureID = gl::loadTexture(win->util.getFilePath("data/crystal1.png"));
        if (textureID == 0) {
            throw mx::Exception("Failed to load texture");
        }
        
        std::vector<GLuint> textures{textureID};
        model.setTextures(textures);
        model.setShaderProgram(&shader_program, "texture1");
        shader_program.useProgram();
        glUniform2f(glGetUniformLocation(shader_program.id(), "iResolution"), win->w, win->h);
    }

    void draw(gl::GLWindow *win) override {
        glEnable(GL_DEPTH_TEST); 
        
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; 
        lastUpdateTime = currentTime;
        
        static float time_f = 0.0f;
        time_f += deltaTime;

        
        glm::mat4 model_matrix = glm::mat4(1.0f);
        model_matrix = glm::scale(model_matrix, glm::vec3(0.5f, 0.5f, 0.5f));
        model_matrix = glm::rotate(model_matrix, (float)(currentTime / 1000.0f), glm::vec3(1.0f, 1.0f, 0.0f));
        
        glm::mat4 view_matrix = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 5.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        
        float aspectRatio = static_cast<float>(win->w) / static_cast<float>(win->h);
        glm::mat4 projection_matrix = glm::perspective(glm::radians(zoom), aspectRatio, 0.1f, 100.0f);
        
        shader_program.useProgram();
        shader_program.setUniform("model", model_matrix);
        shader_program.setUniform("view", view_matrix);
        shader_program.setUniform("projection", projection_matrix);

        shader_program.setUniform("ambientStrength", 0.4f);    
        shader_program.setUniform("diffuseStrength", 0.9f);    
        shader_program.setUniform("specularStrength", 1.2f);   
        shader_program.setUniform("shininess", 128.0f);
        

        float lightX = 3.0f * sin(currentTime / 1000.0f);
        float lightZ = 3.0f * cos(currentTime / 1000.0f);
        shader_program.setUniform("lightPos", glm::vec3(lightX, 2.0f, lightZ + 2.0f));

        
        shader_program.setUniform("lightColor", glm::vec3(1.2f, 1.2f, 1.2f)); 
        shader_program.setUniform("time", time_f);
        model.drawArrays();

        win->text.setColor({255,255,255,255});
        win->text.printText_Solid(font, 10, 10, "Up/Down: Zoom: " + std::to_string(zoom));
        win->text.printText_Solid(font, 10, 40, "S/D: Shininess: " + std::to_string(shininess));
        win->text.printText_Solid(font, 10, 70, "B/V: Specular Strength: " + std::to_string(specularStrength));

    }
    
    void event(gl::GLWindow *win, SDL_Event &e) override {
        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_UP:
                case SDLK_PLUS:
                case SDLK_KP_PLUS:
                    zoom -= 2.0f;
                    if (zoom < 2.0f) zoom = 2.0f; 
                    break;

                    
                case SDLK_DOWN:
                case SDLK_MINUS:
                case SDLK_KP_MINUS:
                    zoom += 2.0f;
                    if (zoom > 120.0f) zoom = 120.0f; 
                    break;
                    
                case SDLK_RETURN:
                    zoom = 45.0f;
                    break;

                
                case SDLK_s:
                    shininess *= 1.5f;
                    if (shininess > 2048.0f) shininess = 2048.0f;
                    break;
                case SDLK_d:
                    shininess /= 1.5f;
                    if (shininess < 8.0f) shininess = 8.0f;
                    break;
                    
                
                case SDLK_b:
                    specularStrength += 0.1f;
                    if (specularStrength > 5.0f) specularStrength = 5.0f;
                    break;
                case SDLK_v:
                    specularStrength -= 0.1f;
                    if (specularStrength < 0.1f) specularStrength = 0.1f;
                    break;
            }
        }
    }
    
    void update(float deltaTime) {
        if(deltaTime > 0.1f) {
            deltaTime = 0.1f;
        }
    }
private:
    mx::Font font;
    mx::Model model;
    gl::ShaderProgram shader_program;
    float zoom = 45.0f; 
    Uint32 lastUpdateTime = SDL_GetTicks();
    float shininess = 128.0f;
    float specularStrength = 1.2f; 
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("Texture", tw, th) {
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
