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

#include<random>

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
#if !defined(__EMSCRIPTEN__) && !defined(__ANDROID__)
const char* vertSource = R"(#version 330 core

layout (location = 0) in vec3 inPosition; 
layout (location = 1) in float inSize;
layout (location = 2) in vec4 inColor;

uniform mat4 MVP;  

out vec4 fragColor;

void main() {
    gl_Position = MVP * vec4(inPosition, 1.0);
    gl_PointSize = inSize;
    fragColor = inColor;
}
)";

const char* fragSource = R"(#version 330 core

in vec4 fragColor;
out vec4 FragColor;

uniform sampler2D spriteTexture;

void main() {
    float dist = length(gl_PointCoord - vec2(0.5));
    if (dist > 0.5) {
        discard;
    }
    vec4 texColor = texture(spriteTexture, gl_PointCoord);
    FragColor = texColor * fragColor;
}
)";

#else
const char* vertSource = R"(#version 300 es

precision highp float;

layout (location = 0) in vec3 inPosition; 
layout (location = 1) in float inSize;
layout (location = 2) in vec4 inColor;

uniform mat4 MVP;

out vec4 fragColor;

void main() {
    gl_Position = MVP * vec4(inPosition, 1.0);
    gl_PointSize = inSize;
    fragColor = inColor;
}
)";

const char* fragSource = R"(#version 300 es

precision highp float;

in vec4 fragColor;
out vec4 FragColor;

uniform sampler2D spriteTexture;

void main() {
    float dist = length(gl_PointCoord - vec2(0.5));
    if (dist > 0.5) {
        discard;
    }

    vec4 texColor = texture(spriteTexture, gl_PointCoord);
    FragColor = texColor * fragColor;
}
)";
#endif

float generateRandomFloat(float min, float max) {
    static std::random_device rd; 
    static std::default_random_engine eng(rd()); 
    std::uniform_real_distribution<float> dist(min, max);
    return dist(eng);
}

class StarField : public gl::GLObject {
public:
    struct Particle {
        float x, y, z;   
        float vx, vy, vz; 
        float life;
        float twinkle;
    };

    static constexpr int NUM_PARTICLES = 2500;
    gl::ShaderProgram program;
    GLuint VAO, VBO[3];
    GLuint texture;
    std::vector<Particle> particles;
    Uint32 lastUpdateTime = 0;
    float cameraZoom = 3.0f;   
    float cameraRotation = 0.0f; 

    StarField() : particles(NUM_PARTICLES) {}

    ~StarField() override {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(3, VBO);
        glDeleteTextures(1, &texture);
    }

    void load(gl::GLWindow *win) override {
        if(!program.loadProgramFromText(vertSource, fragSource)) {
            throw mx::Exception("Error loading shader");
        }

        for (auto& p : particles) {
            p.x = generateRandomFloat(-1.0f, 1.0f);
            p.y = generateRandomFloat(-1.0f, 1.0f);
            p.z = generateRandomFloat(-5.0f, -2.0f); 
            p.vx = generateRandomFloat(-0.02f, 0.02f); 
            p.vy = generateRandomFloat(-0.02f, 0.02f); 
            p.vz = generateRandomFloat(0.2f, 0.5f);    
            p.life = generateRandomFloat(0.6f, 1.0f);
            p.twinkle = generateRandomFloat(1.0f, 5.0f);
        }
        glGenVertexArrays(1, &VAO);
        glGenBuffers(3, VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
        glBufferData(GL_ARRAY_BUFFER, NUM_PARTICLES * 3 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);\
        glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
        glBufferData(GL_ARRAY_BUFFER, NUM_PARTICLES * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
        glBufferData(GL_ARRAY_BUFFER, NUM_PARTICLES * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
        texture = gl::loadTexture(win->util.getFilePath("data/star.png"));
        cameraRotation = 356.0f;
        cameraZoom = 0.09f;
        lastUpdateTime = SDL_GetTicks();
    }

    void event(gl::GLWindow *win, SDL_Event &e) override {

    }

    void draw(gl::GLWindow *win) override {
    #ifndef __EMSCRIPTEN__
        glEnable(GL_PROGRAM_POINT_SIZE);
    #endif
        glDisable(GL_DEPTH_TEST);

        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; // seconds
        lastUpdateTime = currentTime;

        update(deltaTime);

        CHECK_GL_ERROR();

        program.useProgram();

        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f), 
            (float)win->w / (float)win->h, 
            0.1f, 
            100.0f
        );

        glm::vec3 cameraPos(
            cameraZoom * sin(glm::radians(cameraRotation)), 
            0.0f,                                           
            cameraZoom * cos(glm::radians(cameraRotation))  
        );
        glm::vec3 target(0.0f, 0.0f, 0.0f); 
        glm::vec3 up(0.0f, 1.0f, 0.0f);     \
        glm::mat4 view = glm::lookAt(cameraPos, target, up);
        glm::mat4 model = glm::mat4(1.0f);  
        glm::mat4 MVP = projection * view * model;
        program.setUniform("MVP", MVP);
        program.setUniform("spriteTexture", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindVertexArray(VAO);
        glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);
        CHECK_GL_ERROR();
    }

    void update(float deltaTime) {
        if(deltaTime > 0.1f) 
            deltaTime = 0.1f;
        std::vector<float> positions;
        std::vector<float> sizes;
        std::vector<float> colors;
        positions.reserve(NUM_PARTICLES * 3);
        sizes.reserve(NUM_PARTICLES);
        colors.reserve(NUM_PARTICLES * 4);
        for (auto& p : particles) {
            p.x += p.vx * deltaTime;
            p.y += p.vy * deltaTime;
            p.z += p.vz * deltaTime;
            if (p.z > 0.0f) {
                p.x = generateRandomFloat(-1.0f, 1.0f);
                p.y = generateRandomFloat(-1.0f, 1.0f);
                p.z = generateRandomFloat(-5.0f, -2.0f); 
                p.vx = generateRandomFloat(-0.02f, 0.02f); 
                p.vy = generateRandomFloat(-0.02f, 0.02f); 
                p.vz = generateRandomFloat(0.2f, 0.5f);    
                p.life = generateRandomFloat(0.6f, 1.0f);
                p.twinkle = generateRandomFloat(1.0f, 5.0f);
            }
            float twinkleFactor = 0.5f * (1.0f + sin(SDL_GetTicks() * 0.001f * p.twinkle));
            float brightness = p.life * twinkleFactor;
            positions.push_back(p.x);
            positions.push_back(p.y);
            positions.push_back(p.z);
            float size = 10.0f * p.life;
            sizes.push_back(size);
            float alpha = p.life;
            colors.push_back(brightness);
            colors.push_back(brightness);
            colors.push_back(brightness);
            colors.push_back(alpha);
        }

        glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, positions.size() * sizeof(float), positions.data());

        glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizes.size() * sizeof(float), sizes.data());

        glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, colors.size() * sizeof(float), colors.data());
    }
};
    

class Game : public gl::GLObject {
public:
    Game() = default;
    virtual ~Game() override {}

    void load(gl::GLWindow *win) override {
        if(!shade_program.loadProgramFromText(g_vSource, g_fSource)) {
            throw mx::Exception("Failed to load shader program");
        }
        if(!obj.openModel(win->util.getFilePath("data/torus.mxmod.z"), true)) {
            throw mx::Exception("Error loading model...");
        }   
        obj.setTextures(win, win->util.getFilePath("data/torus.tex"), win->util.getFilePath("data"));
        starField.load(win);
    }

    void draw(gl::GLWindow *win) override {
        glEnable(GL_DEPTH_TEST);
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; 
        lastUpdateTime = currentTime;
       // starField.draw(win);
        glEnable(GL_DEPTH_TEST);
        
        static float rotationAngle = 0.0f;
        rotationAngle += deltaTime * 30.0f; 
        
        cameraPosition = glm::vec3(0.0f, 2.0f, 5.0f);
        glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);

        modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::scale(modelMatrix, glm::vec3(0.3f, 0.3f, 0.3f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationAngle), glm::vec3(1.0f, 1.0f, 0.0f));
        
        viewMatrix = glm::lookAt(cameraPosition, cameraTarget, upVector);
        
        float aspectRatio = static_cast<float>(win->w) / static_cast<float>(win->h);
        projectionMatrix = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);


        //lightPos = glm::vec3(0.0f, 5.0f, 0.0f); 

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
    
    void event(gl::GLWindow *win, SDL_Event &e) override {
        if (e.type == SDL_KEYDOWN) {
            float moveSpeed = 0.05f; 
            switch (e.key.keysym.sym) {
                case SDLK_LEFT:
                    lightPos.x -= moveSpeed;
                    break;
                case SDLK_RIGHT:
                    lightPos.x += moveSpeed;
                    break;
                case SDLK_UP:
                    lightPos.z -= moveSpeed;
                    break;
                case SDLK_DOWN:
                    lightPos.z += moveSpeed;
                    break;
                case SDLK_PAGEUP:
                    lightPos.y += moveSpeed;
                    break;
                case SDLK_PAGEDOWN:
                    lightPos.y -= moveSpeed;
                    break;
                case SDLK_RETURN:
                    lightPos = glm::vec3(0.0f, 5.0f, 0.0f);
                    break;
            }
            std::cout << "Light Position: " << lightPos.x << ", " << lightPos.y << ", " << lightPos.z << "\n";
        }
    }

    void update(float deltaTime) {
        if(deltaTime > 0.1f) {
            deltaTime =  0.1f;
        }
        starField.update(deltaTime);
    }
private:
    gl::ShaderProgram shade_program;
    mx::Model obj;
    Uint32 lastUpdateTime = SDL_GetTicks();
    glm::mat4 modelMatrix{1.0f};
    glm::mat4 viewMatrix{1.0f};
    glm::mat4 projectionMatrix{1.0f};
    glm::vec3 cameraPosition{0.0f, 2.0f, 5.0f};
    glm::vec3 lightPos{0.0f, 5.0f, 0.0f};
    StarField starField;
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
