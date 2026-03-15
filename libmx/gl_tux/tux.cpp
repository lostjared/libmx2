#include"mx.hpp"
#include"argz.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <GLES3/gl3.h>
#endif

#include"gl.hpp"
#include"loadpng.hpp"
#include"model.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define CHECK_GL_ERROR() \
{ GLenum err = glGetError(); \
if (err != GL_NO_ERROR) \
printf("OpenGL Error: %d at %s:%d\n", err, __FILE__, __LINE__); }


#if defined(__EMSCRIPTEN__) || defined(__ANDROID__)
const char *g_vSource = R"(#version 300 es
precision highp float;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = vec3(worldPos);
    Normal  = mat3(transpose(inverse(model))) * aNormal;
    TexCoords = aTexCoords;
    gl_Position = projection * view * worldPos;
}
)";


const char *g_fSource = R"(#version 300 es
precision highp float;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 FragColor;

uniform sampler2D texture1;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;

void main() {
    // Ambient
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular (Phong)
    float specularStrength = 0.6;
    float shininess = 32.0;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;

    vec4 texColor = texture(texture1, TexCoords);
    vec3 result = (ambient + diffuse + specular) * texColor.rgb;
    FragColor = vec4(result, texColor.a);
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

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = vec3(worldPos);
    Normal  = mat3(transpose(inverse(model))) * aNormal;
    TexCoords = aTexCoords;
    gl_Position = projection * view * worldPos;
}
)";


const char *g_fSource = R"(#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 FragColor;

uniform sampler2D texture1;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;

void main() {
    // Ambient
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular (Phong)
    float specularStrength = 0.6;
    float shininess = 32.0;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;

    vec4 texColor = texture(texture1, TexCoords);
    vec3 result = (ambient + diffuse + specular) * texColor.rgb;
    FragColor = vec4(result, texColor.a);
}
)";
#endif


#if defined(__EMSCRIPTEN__) || defined(__ANDROID__)
const char *snowVertSource = R"(#version 300 es
precision highp float;

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 attributes; // intensity, size, rotation

uniform mat4 viewProj;
uniform float time;

out float intensity;
out float rotation;

void main() {
    intensity = attributes.x;
    rotation = attributes.z;
    vec3 pos = position;
    pos.x += sin(time * 0.5 + position.y * 0.1) * 0.1 * position.z;
    float size = attributes.y * 20.0;
    float depth = (viewProj * vec4(pos, 1.0)).w;
    float pointSize = size * (400.0 / depth);
    gl_Position = viewProj * vec4(pos, 1.0);
    gl_PointSize = pointSize;
}
)";

const char *snowFragSource = R"(#version 300 es
precision highp float;

in float intensity;
in float rotation;

uniform sampler2D snowflakeTexture;
uniform float time;

out vec4 fragColor;

void main() {
    vec2 coord = gl_PointCoord - vec2(0.5);
    float angle = rotation + time * 0.5;
    float s = sin(angle);
    float c = cos(angle);
    vec2 rotatedCoord = vec2(
        coord.x * c - coord.y * s,
        coord.x * s + coord.y * c
    );
    rotatedCoord += vec2(0.5);
    vec4 texColor = texture(snowflakeTexture, rotatedCoord);
    if(texColor.r < 0.1) {
        discard;
    }
    float dist = length(gl_PointCoord - vec2(0.5));
    float alpha = smoothstep(0.5, 0.4, dist) * intensity;
    fragColor = vec4(vec3(1.0), texColor.a * alpha);
}
)";
#else
const char *snowVertSource = R"(#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 attributes; // intensity, size, rotation

uniform mat4 viewProj;
uniform float time;

out float intensity;
out float rotation;

void main() {
    intensity = attributes.x;
    rotation = attributes.z;
    vec3 pos = position;
    pos.x += sin(time * 0.5 + position.y * 0.1) * 0.1 * position.z;
    float size = attributes.y * 20.0;
    float depth = (viewProj * vec4(pos, 1.0)).w;
    float pointSize = size * (400.0 / depth);
    gl_Position = viewProj * vec4(pos, 1.0);
    gl_PointSize = pointSize;
}
)";

const char *snowFragSource = R"(#version 330 core

in float intensity;
in float rotation;

uniform sampler2D snowflakeTexture;
uniform float time;

out vec4 fragColor;

void main() {
    vec2 coord = gl_PointCoord - vec2(0.5);
    float angle = rotation + time * 0.5;
    float s = sin(angle);
    float c = cos(angle);
    vec2 rotatedCoord = vec2(
        coord.x * c - coord.y * s,
        coord.x * s + coord.y * c
    );
    rotatedCoord += vec2(0.5);
    vec4 texColor = texture(snowflakeTexture, rotatedCoord);
    if(texColor.r < 0.1) {
        discard;
    }
    float dist = length(gl_PointCoord - vec2(0.5));
    float alpha = smoothstep(0.5, 0.4, dist) * intensity;
    fragColor = vec4(vec3(1.0), texColor.a * alpha);
}
)";
#endif

struct SnowFlake {
    float x, y, z;
    float intensity;
    float size;
    float rotation;
    float speed;
    float rotationSpeed;
};

class SnowEmiter {
public:
    static const int MAX_SNOWFLAKES = 3000;

    SnowEmiter() = default;
    ~SnowEmiter() {
        releaseSnowFlakes();
    }

    void initPointSprites(gl::GLWindow *win) {
        SDL_Surface* snowflakeSurface = png::LoadPNG(win->util.getFilePath("data/snowflake.png").c_str());
        if (!snowflakeSurface) {
            throw mx::Exception("Failed to load snowflake texture");
        }

        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, snowflakeSurface->w, snowflakeSurface->h,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, snowflakeSurface->pixels);
        SDL_FreeSurface(snowflakeSurface);

        if (!snowShader.loadProgramFromText(snowVertSource, snowFragSource)) {
            throw mx::Exception("Failed to load snow shaders");
        }

        snowflakes.reserve(MAX_SNOWFLAKES);
        for (int i = 0; i < MAX_SNOWFLAKES; ++i) {
            SnowFlake flake;
            flake.x = static_cast<float>(rand() % 2000 - 1000) / 100.0f;
            flake.y = static_cast<float>(rand() % 2000 - 1000) / 100.0f;
            flake.z = static_cast<float>(rand() % 1000) / 1000.0f - 2.0f;
            flake.intensity = static_cast<float>(rand() % 100) / 100.0f;
            flake.size = 0.005f + static_cast<float>(rand() % 100) / 3000.0f;
            flake.speed = 0.4f + static_cast<float>(rand() % 100) / 500.0f;
            flake.rotation = static_cast<float>(rand() % 360) * (M_PI / 180.0f);
            flake.rotationSpeed = (static_cast<float>(rand() % 100) / 1000.0f) - 0.05f;
            snowflakes.push_back(flake);
        }

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, snowflakes.size() * sizeof(SnowFlake), snowflakes.data(), GL_DYNAMIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SnowFlake), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(SnowFlake), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
        CHECK_GL_ERROR();
    }

    void update(float deltaTime) {
        windTime += deltaTime * 1.2f;
        float windOffsetX = std::sin(windTime) * 0.3f;
        float windOffsetZ = std::cos(windTime * 0.7f) * 0.2f;

        for (auto& flake : snowflakes) {
            flake.y -= flake.speed * deltaTime;
            flake.x += windOffsetX * deltaTime;
            flake.z += windOffsetZ * deltaTime;
            flake.rotation += flake.rotationSpeed * deltaTime;

            if (flake.y < -10.0f) {
                flake.y = 10.0f;
                flake.x = static_cast<float>(rand() % 2000 - 1000) / 100.0f;
                flake.z = static_cast<float>(rand() % 1000) / 1000.0f - 2.0f;
            }
        }

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, snowflakes.size() * sizeof(SnowFlake), snowflakes.data());
    }

    void draw(const glm::mat4 &projection, const glm::mat4 &view) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#ifndef __EMSCRIPTEN__
        glEnable(GL_PROGRAM_POINT_SIZE);
#endif
        glDepthMask(GL_FALSE);

        snowShader.useProgram();
        GLint viewProjLoc = glGetUniformLocation(snowShader.id(), "viewProj");
        glUniformMatrix4fv(viewProjLoc, 1, GL_FALSE, glm::value_ptr(projection * view));

        GLint texLoc = glGetUniformLocation(snowShader.id(), "snowflakeTexture");
        glUniform1i(texLoc, 0);

        GLint timeLoc = glGetUniformLocation(snowShader.id(), "time");
        glUniform1f(timeLoc, SDL_GetTicks() / 1000.0f);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, snowflakes.size());
        glBindVertexArray(0);

#ifndef __EMSCRIPTEN__
        glDisable(GL_PROGRAM_POINT_SIZE);
#endif
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        CHECK_GL_ERROR();
    }

    void releaseSnowFlakes() {
        if (vbo != 0) {
            glDeleteBuffers(1, &vbo);
            vbo = 0;
        }

        if (vao != 0) {
            glDeleteVertexArrays(1, &vao);
            vao = 0;
        }

        if (textureID != 0) {
            glDeleteTextures(1, &textureID);
            textureID = 0;
        }
    }

private:
    std::vector<SnowFlake> snowflakes;
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint textureID = 0;
    gl::ShaderProgram snowShader;
    float windTime = 0.0f;
};

class Game : public gl::GLObject {
public:
    Game() = default;
    virtual ~Game() override {}

    void load(gl::GLWindow *win) override {
        font.loadFont(win->util.getFilePath("data/font.ttf"), 18);

        if (!shader.loadProgramFromText(g_vSource, g_fSource)) {
            throw mx::Exception("Failed to load Phong shader program");
        }

        if (!tuxModel.openModel(win->util.getFilePath("data/tux.obj"))) {
            throw mx::Exception("Failed to load tux.obj model");
        }

        if(!bg_shader.loadProgramFromText(gl::vSource, gl::fSource)) {
            throw mx::Exception("Failed to load background shader");
        }

        background.initSize(win->w, win->h);
        background.loadTexture(&bg_shader, win->util.getFilePath("data/background.png"), 0.0f, 0.0f, win->w, win->h);
    
        shader.useProgram();
        tuxModel.setShaderProgram(&shader, "texture1");
        tuxModel.setTextures(win, win->util.getFilePath("data/tux.tex"),
                             win->util.getFilePath("data"));

        snowEmitter.initPointSprites(win);
    }

    void draw(gl::GLWindow *win) override {
#ifdef __EMSCRIPTEN__
        float currentTime = emscripten_get_now();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;
#else
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;
#endif

        glDisable(GL_DEPTH_TEST);
        bg_shader.setUniform("textTexture", 0);
        background.draw();
        glEnable(GL_DEPTH_TEST);
        if(!paused) {
            rotationAngle += deltaTime * 50.0f;
            if (rotationAngle >= 360.0f) rotationAngle -= 360.0f;
        }
        shader.useProgram();

        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, modelYOffset, 0.0f));
        modelMatrix = glm::scale(modelMatrix, glm::vec3(0.25f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationAngle),
                                  glm::vec3(0.0f, 1.0f, 0.0f));

        glm::mat4 viewMatrix = glm::lookAt(cameraPos, cameraTarget, cameraUp);

        
        float aspect = static_cast<float>(win->w) / static_cast<float>(win->h);
        glm::mat4 projMatrix = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);

        shader.setUniform("model", modelMatrix);
        shader.setUniform("view", viewMatrix);
        shader.setUniform("projection", projMatrix);
        shader.setUniform("lightPos", lightPos);
        shader.setUniform("viewPos", cameraPos);
        shader.setUniform("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));

        tuxModel.setShaderProgram(&shader, "texture1");
        tuxModel.drawArrays();

        snowEmitter.update(deltaTime);
        glDisable(GL_DEPTH_TEST);
        snowEmitter.draw(projMatrix, viewMatrix);
        win->text.setColor({255, 255, 255, 255});
        win->text.printText_Solid(font, 25.0f, 25.0f,
            "Arrow keys / PgUp PgDn: move light  Enter: reset  Space: pause rotation");
        glEnable(GL_DEPTH_TEST);
    }

    void event(gl::GLWindow *win, SDL_Event &e) override {
        if (e.type == SDL_KEYDOWN) {
            float step = 0.5f;
            switch (e.key.keysym.sym) {
                case SDLK_LEFT:    lightPos.x -= step; break;
                case SDLK_RIGHT:   lightPos.x += step; break;
                case SDLK_UP:      lightPos.z -= step; break;
                case SDLK_DOWN:    lightPos.z += step; break;
                case SDLK_PAGEUP:  lightPos.y += step; break;
                case SDLK_PAGEDOWN:lightPos.y -= step; break;
                case SDLK_RETURN:
                    lightPos = glm::vec3(5.0f, 10.0f, 5.0f);
                    break;
                case SDLK_SPACE:
                    paused = !paused;
                    break;
            }
        }
    }

private:
    mx::Font font;
    gl::ShaderProgram shader, bg_shader;
    mx::Model tuxModel;

    float rotationAngle = 0.0f;
#ifdef __EMSCRIPTEN__
    float lastTime = emscripten_get_now();
#else
    Uint32 lastTime = SDL_GetTicks();
#endif
    bool paused = false;

    glm::vec3 cameraPos    = glm::vec3(0.0f, 2.0f, 5.0f);
    glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 cameraUp     = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 lightPos     = glm::vec3(5.0f, 10.0f, 5.0f);
    float modelYOffset     = -0.8f;
    SnowEmiter snowEmitter;
    gl::GLSprite background;
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("Tux", tw, th) {
        setPath(path);
        setObject(new Game());
        object->load(this);
    }

    ~MainWindow() override {}

    virtual void event(SDL_Event &e) override {}

    virtual void draw() override {
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
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
    main_w = &main_window;
    emscripten_set_main_loop(eventProc, 0, 1);
#else
    Argz<std::string> parser(argc, argv);
    parser.addOptionSingle('h', "Display help message")
          .addOptionSingleValue('p', "assets path")
          .addOptionDoubleValue('P', "path", "assets path")
          .addOptionSingleValue('r', "Resolution WidthxHeight")
          .addOptionDoubleValue('R', "resolution", "Resolution WidthxHeight");
    Argument<std::string> arg;
    std::string path;
    int tw = 1280, th = 720;
    try {
        int value = 0;
        while ((value = parser.proc(arg)) != -1) {
            switch (value) {
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
                    if (pos == std::string::npos) {
                        mx::system_err << "Error invalid resolution use WidthxHeight\n";
                        mx::system_err.flush();
                        exit(EXIT_FAILURE);
                    }
                    tw = std::stoi(arg.arg_value.substr(0, pos));
                    th = std::stoi(arg.arg_value.substr(pos + 1));
                } break;
            }
        }
        MainWindow main_window(path, tw, th);
        main_window.loop();
    } catch (const mx::Exception &e) {
        mx::system_err << "mx: Exception: " << e.text() << "\n";
        mx::system_err.flush();
        exit(EXIT_FAILURE);
    }
#endif
    return 0;
}
