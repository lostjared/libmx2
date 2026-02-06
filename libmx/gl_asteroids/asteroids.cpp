#define ASTEROIDS_VERSION "v1.1"
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
#include"console.hpp"


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

using mx::generateRandomFloat;
using mx::generateRandomInt;

console::GLConsole *con = nullptr;


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
    vec3 norm = normalize(mat3(transpose(inverse(model)))* aNormal);
    vec3 lightDir = normalize(lightPos - vec3(worldPos));
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    float specularStrength = 1.0;    
    float shininess = 64.0;          
    vec3 viewDir = normalize(viewPos - vec3(worldPos));
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), float(shininess));
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

        uniform float time_f;
        uniform int effect_type;

        vec4 alphaXor(vec4 color) {
            ivec3 source;
            for (int i = 0; i < 3; ++i) {
                source[i] = int(255.0 * clamp(color[i], 0.0, 1.0));
            }
            
            float time_mod = mod(time_f, 10.0); 
            ivec3 time_factors = ivec3(
                int(255.0 * time_mod / 10.0), 
                int(127.0 * time_mod / 10.0), 
                int(63.0 * time_mod / 10.0)
            );
            
            
            ivec3 int_color;
            for (int i = 0; i < 3; ++i) {
                int_color[i] = int(255.0 * clamp(color[i], 0.0, 1.0));
                int_color[i] = int_color[i] ^ (source[i] + time_factors[i % 3]);
                int_color[i] = int_color[i] % 256; 
                color[i] = float(int_color[i]) / 255.0;
            }
            return color;
        }

        vec4 distort(vec2 tc, float time_t, sampler2D tex) {
            vec4 ctx;
            float xDistort = cos(tc.y * 10.0 + time_f) * 0.1;
                float yDistort = sin(tc.x * 10.0 + time_f) * 0.1;
                float tanDistortX = tan(tc.x * 5.0 + time_f) * 0.05;
                float tanDistortY = tan(tc.y * 5.0 + time_f) * 0.05;
                vec2 distortedTC = tc + vec2(xDistort + tanDistortX, yDistort + tanDistortY);
                distortedTC = fract(distortedTC);
                ctx = texture(tex, distortedTC);
                return ctx;
        }


        void main()
        {
            if(effect_type != 3) {
                FragColor = vec4(vertexColor, 1.0) * texture(texture1, TexCoords);
                vec4 ctx = distort(TexCoords, time_f, texture1);
                FragColor = alphaXor(ctx);
                FragColor.a = 1.0;
            } else {
                FragColor = texture(texture1, TexCoords); 
            }
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
        float specularStrength = 1.0;    
        float shininess = 64.0;          
        vec3 viewDir = normalize(viewPos - vec3(worldPos));
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), float(shininess));
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

        uniform float time_f;
        uniform int effect_type;

        vec4 alphaXor(vec4 color) {
            ivec3 source;
            for (int i = 0; i < 3; ++i) {
                source[i] = int(255.0 * clamp(color[i], 0.0, 1.0));
            }
            
            float time_mod = mod(time_f, 10.0); 
            ivec3 time_factors = ivec3(
                int(255.0 * time_mod / 10.0), 
                int(127.0 * time_mod / 10.0), 
                int(63.0 * time_mod / 10.0)
            );
            
            
            ivec3 int_color;
            for (int i = 0; i < 3; ++i) {
                int_color[i] = int(255.0 * clamp(color[i], 0.0, 1.0));
                int_color[i] = int_color[i] ^ (source[i] + time_factors[i % 3]);
                int_color[i] = int_color[i] % 256; 
                color[i] = float(int_color[i]) / 255.0;
            }
            return color;
        }

        vec4 distort(vec2 tc, float time_t, sampler2D tex) {
            vec4 ctx;
            float xDistort = cos(tc.y * 10.0 + time_f) * 0.1;
                float yDistort = sin(tc.x * 10.0 + time_f) * 0.1;
                float tanDistortX = tan(tc.x * 5.0 + time_f) * 0.05;
                float tanDistortY = tan(tc.y * 5.0 + time_f) * 0.05;
                vec2 distortedTC = tc + vec2(xDistort + tanDistortX, yDistort + tanDistortY);
                distortedTC = fract(distortedTC);
                ctx = texture(tex, distortedTC);
                return ctx;
        }


        void main() {
            if(effect_type != 3) {
                FragColor = vec4(vertexColor, 1.0) * texture(texture1, TexCoords);
                vec4 ctx = distort(TexCoords, time_f, texture1);
                FragColor = alphaXor(ctx);
                FragColor.a = 1.0;
            } else {
                FragColor = texture(texture1, TexCoords); 
            }
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
    vec2 coord = gl_PointCoord - vec2(0.5);
    float dist = length(coord);
    
    if (dist > 0.5) {
        discard;
    }
    
    vec4 texColor = texture(spriteTexture, gl_PointCoord);
    
    float glow = 1.0 - smoothstep(0.0, 0.5, dist);
    float core = 1.0 - smoothstep(0.0, 0.15, dist);
    
    vec4 glowColor = fragColor * glow;
    vec4 coreColor = vec4(1.0, 1.0, 1.0, 1.0) * core * fragColor.a;
    
    FragColor = texColor * fragColor * glow + coreColor * 0.3;
    FragColor.a = fragColor.a * glow;
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
    vec2 coord = gl_PointCoord - vec2(0.5);
    float dist = length(coord);
    
    if (dist > 0.5) {
        discard;
    }
    
    vec4 texColor = texture(spriteTexture, gl_PointCoord);
    
    float glow = 1.0 - smoothstep(0.0, 0.5, dist);
    float core = 1.0 - smoothstep(0.0, 0.15, dist);
    
    vec4 glowColor = fragColor * glow;
    vec4 coreColor = vec4(1.0, 1.0, 1.0, 1.0) * core * fragColor.a;
    
    FragColor = texColor * fragColor * glow + coreColor * 0.3;
    FragColor.a = fragColor.a * glow;
}
)";
#endif

class Intro : public gl::GLObject {
public:

    Intro() = default;

    virtual ~Intro() {

    }

    virtual void load(gl::GLWindow *win) {

        font.loadFont(win->util.getFilePath("data/font.ttf"), 25);

#if defined(__EMSCRIPTEN__) 
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
    precision highp float;
    out vec4 FragColor;
    in vec2 TexCoord;
    
    uniform sampler2D textTexture;
    uniform float time_f;
    uniform float alpha;
    
    void main(void) {
        vec2 uv = TexCoord * 2.0 - 1.0;
        float len = length(uv);
        float bubble = smoothstep(0.8, 1.0, 1.0 - len);
        vec2 distort = uv * (1.0 + 0.1 * sin(time_f + len * 20.0));
        vec4 texColor = texture(textTexture, distort * 0.5 + 0.5);
        FragColor = mix(texColor, vec4(1.0, 1.0, 1.0, 1.0), bubble);
        FragColor = FragColor * alpha;
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
        out vec4 FragColor;
        in vec2 TexCoord;
        uniform sampler2D textTexture;
        uniform float time_f;
        uniform float alpha;
        void main(void) {
            vec2 uv = TexCoord * 2.0 - 1.0;
            float len = length(uv);
            float bubble = smoothstep(0.8, 1.0, 1.0 - len);
            vec2 distort = uv * (1.0 + 0.1 * sin(time_f + len * 20.0));
            vec4 texColor = texture(textTexture, distort * 0.5 + 0.5);
            FragColor = mix(texColor, vec4(1.0, 1.0, 1.0, 1.0), bubble);
            FragColor = FragColor * alpha;
        }
    )";
#endif
        if(!shader.loadProgramFromText(vSource, fSource)) {
            throw mx::Exception("Error loading Intro shader");
        }   

        intro.initSize(win->w, win->h);
        intro.loadTexture(&shader, win->util.getFilePath("data/intro.png"), 0.0f, 0.0f, win->w, win->h);
    }

    virtual void draw(gl::GLWindow *win);
    virtual void event(gl::GLWindow *win, SDL_Event &e) {
        if((e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_SPACE) || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_RETURN)) {
            fade = 0.01f;
        }
    }

    void update(float deltaTime) {
        if (deltaTime > 0.1f) {
            deltaTime = 0.1f;
        }
    }
protected:
    gl::GLSprite intro;
    gl::ShaderProgram shader;
    Uint32 lastUpdateTime = 0;
    float fade = 1.0f;
    mx::Font font;
};

class StarField : public gl::GLObject {
public:
    enum class StarType { NORMAL, BRIGHT, BLUE, ORANGE, RED, YELLOW };

    struct Particle {
        float x, y, z;
        float vx, vy, vz;
        float life;
        float twinkle;
        float twinklePhase;
        StarType type;
        int layer;
        float pulseSpeed;
        float baseSize;
    };

    struct LayerConfig {
        float radiusMin, radiusMax;
        float speedMultiplier;
        float sizeMultiplier;
        int count;
    };

    static constexpr int NUM_PARTICLES = 80000;
    static constexpr int NUM_LAYERS = 3;

    gl::ShaderProgram program;
    GLuint VAO, VBO[3];
    GLuint texture;
    std::vector<Particle> particles;
    Uint32 lastUpdateTime = 0;
    float cameraZoom = 3.0f;
    float cameraRotation = 0.0f;
    float globalTime = 0.0f;

    glm::vec3 lastCameraPos{0.0f, 0.0f, 0.0f};
    float starFieldRadius = 30.0f;

    LayerConfig layers[NUM_LAYERS] = {
        { 20.0f, 30.0f, 0.3f, 0.5f, 48000 },   
        { 10.0f, 20.0f, 0.6f, 1.0f, 22000 },    
        { 4.0f,  10.0f, 1.0f, 1.5f, 10000 }     
    };

    glm::vec4 getStarColor(StarType type, float brightness) {
        switch(type) {
            case StarType::BLUE:
                return glm::vec4(0.6f * brightness, 0.8f * brightness, 1.0f * brightness, 1.0f);
            case StarType::ORANGE:
                return glm::vec4(1.0f * brightness, 0.7f * brightness, 0.3f * brightness, 1.0f);
            case StarType::RED:
                return glm::vec4(1.0f * brightness, 0.4f * brightness, 0.4f * brightness, 1.0f);
            case StarType::YELLOW:
                return glm::vec4(1.0f * brightness, 1.0f * brightness, 0.6f * brightness, 1.0f);
            case StarType::BRIGHT:
                return glm::vec4(1.0f * brightness, 1.0f * brightness, 1.0f * brightness, 1.0f);
            case StarType::NORMAL:
            default:
                return glm::vec4(0.9f * brightness, 0.9f * brightness, 1.0f * brightness, 1.0f);
        }
    }

    StarType randomStarType() {
        float r = generateRandomFloat(0.0f, 1.0f);
        if (r < 0.50f) return StarType::NORMAL;
        if (r < 0.65f) return StarType::BLUE;
        if (r < 0.75f) return StarType::YELLOW;
        if (r < 0.85f) return StarType::ORANGE;
        if (r < 0.92f) return StarType::RED;
        return StarType::BRIGHT;
    }

    StarField() : particles(NUM_PARTICLES) {}

    ~StarField() override {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(3, VBO);
        glDeleteTextures(1, &texture);
    }

    void initParticle(Particle& p, int layer, const glm::vec3& center) {
        const auto& cfg = layers[layer];
        p.layer = layer;
        float radius = generateRandomFloat(cfg.radiusMin, cfg.radiusMax);
        float theta = generateRandomFloat(0.0f, 2.0f * M_PI);
        float phi = generateRandomFloat(0.0f, M_PI);
        p.x = center.x + radius * sin(phi) * cos(theta);
        p.y = center.y + radius * sin(phi) * sin(theta);
        p.z = center.z + radius * cos(phi);
        p.vx = generateRandomFloat(-0.01f, 0.01f) * cfg.speedMultiplier;
        p.vy = generateRandomFloat(-0.01f, 0.01f) * cfg.speedMultiplier;
        p.vz = generateRandomFloat(-0.01f, 0.01f) * cfg.speedMultiplier;
        p.life = generateRandomFloat(0.7f, 1.0f);
        p.twinkle = generateRandomFloat(2.0f, 8.0f);
        p.twinklePhase = generateRandomFloat(0.0f, 6.28f);
        p.type = randomStarType();
        p.pulseSpeed = generateRandomFloat(0.5f, 3.0f);
        float typeMultiplier = (p.type == StarType::BRIGHT) ? 2.0f : 1.0f;
        p.baseSize = generateRandomFloat(8.0f, 22.0f) * cfg.sizeMultiplier * typeMultiplier;
    }

    void load(gl::GLWindow *win) override {
        if(!program.loadProgramFromText(vertSource, fragSource)) {
            throw mx::Exception("Error loading shader");
        }

        glm::vec3 center(0.0f);
        int particleIndex = 0;
        for (int layer = 0; layer < NUM_LAYERS; ++layer) {
            int count = layers[layer].count;
            for (int i = 0; i < count && particleIndex < NUM_PARTICLES; ++i, ++particleIndex) {
                initParticle(particles[particleIndex], layer, center);
            }
        }

        glGenVertexArrays(1, &VAO);
        glGenBuffers(3, VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
        glBufferData(GL_ARRAY_BUFFER, NUM_PARTICLES * 3 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
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
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f;
        lastUpdateTime = currentTime;
        globalTime += deltaTime;

        update(deltaTime);

        CHECK_GL_ERROR();

        program.useProgram();

        glm::mat4 viewRotOnly = glm::mat4(glm::mat3(viewMatrix));
        glm::mat4 MVP = projectionMatrix * viewRotOnly * glm::mat4(1.0f);
        program.setUniform("MVP", MVP);
        program.setUniform("spriteTexture", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindVertexArray(VAO);
        glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);
        CHECK_GL_ERROR();

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_BLEND);
    }

    void update(float deltaTime) {
        if(deltaTime > 0.1f)
            deltaTime = 0.1f;

        CHECK_GL_ERROR();

        std::vector<float> positions;
        std::vector<float> sizes;
        std::vector<float> colors;
        positions.reserve(NUM_PARTICLES * 3);
        sizes.reserve(NUM_PARTICLES);
        colors.reserve(NUM_PARTICLES * 4);

        float time = globalTime;
        glm::vec3 origin(0.0f);

        for (auto& p : particles) {
            
            p.x += p.vx * deltaTime;
            p.y += p.vy * deltaTime;
            p.z += p.vz * deltaTime;

            
            float distance = glm::length(glm::vec3(p.x, p.y, p.z));
            const auto& cfg = layers[p.layer];

            
            if (distance > cfg.radiusMax * 1.1f || distance < cfg.radiusMin * 0.5f) {
                initParticle(p, p.layer, origin);
            }

            
            float twinkle1 = 0.5f * (1.0f + sin(time * p.twinkle + p.twinklePhase));
            float twinkle2 = 0.3f * (1.0f + sin(time * p.pulseSpeed * 2.0f + p.twinklePhase * 1.5f));
            float twinkleFactor = 0.5f + 0.3f * twinkle1 + 0.2f * twinkle2;

            
            float depthFactor = 1.0f - (distance / (cfg.radiusMax * 1.1f));
            depthFactor = glm::clamp(depthFactor, 0.3f, 1.0f);

            float brightness = p.life * twinkleFactor * depthFactor;

            
            glm::vec4 color = getStarColor(p.type, brightness);

            positions.push_back(p.x);
            positions.push_back(p.y);
            positions.push_back(p.z);

            
            float sizePulse = 1.0f + 0.2f * sin(time * p.pulseSpeed + p.twinklePhase);
            float size = p.baseSize * sizePulse * depthFactor;
            sizes.push_back(size);

            float alpha = p.life * glm::clamp(depthFactor + 0.2f, 0.0f, 1.0f);
            colors.push_back(color.r);
            colors.push_back(color.g);
            colors.push_back(color.b);
            colors.push_back(alpha);
        }

        if (positions.empty() || sizes.empty() || colors.empty()) {
            return;
        }

        CHECK_GL_ERROR();
        size_t uploadSize = std::min(positions.size() * sizeof(float), size_t(NUM_PARTICLES * 3 * sizeof(float)));

        glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, uploadSize, positions.data());

        CHECK_GL_ERROR();

        glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizes.size() * sizeof(float), sizes.data());

        CHECK_GL_ERROR();

        glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, colors.size() * sizeof(float), colors.data());

        CHECK_GL_ERROR();
    }

    void repositionStarsAroundCamera(const glm::vec3& cameraPos) {
        int particleIndex = 0;
        for (int layer = 0; layer < NUM_LAYERS; ++layer) {
            int count = layers[layer].count;
            for (int i = 0; i < count && particleIndex < NUM_PARTICLES; ++i, ++particleIndex) {
                initParticle(particles[particleIndex], layer, cameraPos);
            }
        }
        lastCameraPos = cameraPos;
    }

    void setViewProjectionMatrices(const glm::mat4& view, const glm::mat4& projection) {
        this->viewMatrix = view;
        this->projectionMatrix = projection;
        
        glm::mat3 rotMat(view);
        glm::vec3 d(view[3]);
        glm::vec3 cameraPos = -d * rotMat;
        
        static bool firstTime = true;
        if (firstTime) {
            repositionStarsAroundCamera(cameraPos);
            firstTime = false;
        }
    }
    
private:
    glm::mat4 projectionMatrix{1.0f};
    glm::mat4 viewMatrix{1.0f};
};
    
class ExplodeEmiter {
public:
    static constexpr int MAX_PARTICLES = 40000; 
    static constexpr int MAX_EXPLOSIONS = 5;    

    
    struct Explosion {
        glm::vec3 position;
        float startTime;
        float lifeTime;
        bool active;
        
        Explosion() : position(0.0f), startTime(0.0f), lifeTime(3.0f), active(false) {}
    };
    
    std::vector<Explosion> explosions;
    
    ExplodeEmiter() {
        explosions.resize(MAX_EXPLOSIONS);
    }

    ~ExplodeEmiter() {
        if (vao != 0) glDeleteVertexArrays(1, &vao);
        if (vbo != 0) glDeleteBuffers(1, &vbo);
    }

    struct Particle {
        float x, y, z;         
        float vx, vy, vz;      
        float r, g, b;         
        float alpha;           
        float size;            
        float life;            
        float maxLife;         
        bool active;           
    };

    
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    
    void load(gl::GLWindow *win) {
        
#ifndef __EMSCRIPTEN__
        const char* particleVS = R"(#version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec4 aColor;
        layout (location = 2) in float aSize;
        
        out vec4 Color;
        
        uniform mat4 projection;
        uniform mat4 view;
        
        void main() {
            vec4 viewPos = view * vec4(aPos, 1.0);
            gl_Position = projection * viewPos;
            
            float distance = length(viewPos.xyz);
            gl_PointSize = aSize / (distance * 0.1); 
            Color = aColor;
        }
        )";

        const char* particleFS = R"(#version 330 core
        in vec4 Color;
        out vec4 FragColor;
        
        uniform sampler2D particleTexture;
        
        void main() {
            vec2 texCoord = gl_PointCoord;
            vec4 texColor = texture(particleTexture, texCoord);
            if(texColor.r < 0.1 && texColor.g < 0.1 && texColor.b < 0.1) {
                discard;
            }
            vec4 finalColor = texColor * Color;
            float dist = length(texCoord - vec2(0.5));
            float alpha = smoothstep(0.5, 0.4, dist) * finalColor.a;
            
            if (alpha < 0.01)
                discard;
                
            FragColor = vec4(finalColor.rgb, alpha);
        }
        )";
#else
        const char* particleVS = R"(#version 300 es
        precision highp float;
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec4 aColor;
        layout (location = 2) in float aSize;
        
        out vec4 Color;
        
        uniform mat4 projection;
        uniform mat4 view;
        
        void main() {
            vec4 viewPos = view * vec4(aPos, 1.0);
            gl_Position = projection * viewPos;
            
            float distance = length(viewPos.xyz);
            gl_PointSize = aSize / (distance * 0.1);
            Color = aColor;
        }
        )";

        const char* particleFS = R"(#version 300 es
        precision highp float;
        in vec4 Color;
        out vec4 FragColor;
        
        uniform sampler2D particleTexture;
        
        void main() {
            vec2 texCoord = gl_PointCoord;
            vec4 texColor = texture(particleTexture, texCoord);
            if(texColor.r < 0.1 && texColor.g < 0.1 && texColor.b < 0.1) {
                discard;
            }
            vec4 finalColor = texColor * Color;
            
            float dist = length(texCoord - vec2(0.5));
            float alpha = smoothstep(0.5, 0.4, dist) * finalColor.a;
            
            if (alpha < 0.01)
                discard;
                
            FragColor = vec4(finalColor.rgb, alpha);
        }
        )";
#endif
        if (!shader.loadProgramFromText(particleVS, particleFS)) {
            throw mx::Exception("Failed to load particle shader program");
        }

        
        particles.resize(MAX_PARTICLES); 
        for (auto& p : particles) {
            p.active = false;
        }

        
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        
        glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * 8 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(7 * sizeof(float)));
        glEnableVertexAttribArray(2);
        
        glBindVertexArray(0);
    }

    void draw(gl::GLWindow *win) {
        if (activeParticles == 0) return;
        
        
        std::vector<float> particleData;
        particleData.reserve(activeParticles * 8); 
        
        for (const auto& p : particles) {
            if (!p.active) continue;
            
            
            particleData.push_back(p.x);
            particleData.push_back(p.y);
            particleData.push_back(p.z);
            
            
            particleData.push_back(p.r);
            particleData.push_back(p.g);
            particleData.push_back(p.b);
            particleData.push_back(p.alpha);
            
            
            particleData.push_back(p.size);
        }
        
        if (particleData.empty() || particleData.size() % 8 != 0) {
            return;
        }

        int count = static_cast<int>(particleData.size() / 8);
        size_t maxBytes = MAX_PARTICLES * 8 * sizeof(float);
        size_t dataBytes = particleData.size() * sizeof(float);
        if (dataBytes > maxBytes) {
            dataBytes = maxBytes; 
            count = MAX_PARTICLES;
        }
        if (count >= MAX_PARTICLES) {
            count = MAX_PARTICLES;
            dataBytes = MAX_PARTICLES * 8 * sizeof(float);
        } else {
            dataBytes = count * 8 * sizeof(float);
        }
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE); 
        glDepthMask(GL_FALSE); 
        CHECK_GL_ERROR();
        shader.useProgram();
        CHECK_GL_ERROR();
        shader.setUniform("projection", projectionMatrix);
        shader.setUniform("view", viewMatrix);
        shader.setUniform("particleTexture", 0);
        CHECK_GL_ERROR();
        glActiveTexture(GL_TEXTURE0);
        CHECK_GL_ERROR();
        glBindTexture(GL_TEXTURE_2D, textureID);
        CHECK_GL_ERROR();
        glBindVertexArray(vao);
        CHECK_GL_ERROR();
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        CHECK_GL_ERROR();   
        glBufferSubData(GL_ARRAY_BUFFER, 0, dataBytes, particleData.data());
        CHECK_GL_ERROR();
#ifndef __EMSCRIPTEN__
        glEnable(GL_PROGRAM_POINT_SIZE);
#endif
        
        if(count > 0) {
            glDrawArrays(GL_POINTS, 0, count);
        }
        CHECK_GL_ERROR();
#ifndef __EMSCRIPTEN__
        glDisable(GL_PROGRAM_POINT_SIZE);
#endif
        
        
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    void explode(const glm::vec3& position) {
        
        for (auto& explosion : explosions) {
            if (!explosion.active) {
                explosion.position = position;
                explosion.startTime = 0.0f;
                explosion.active = true;
                
                createExplosionParticles(explosion);
                return;
            }
        }
        
        float oldestTime = 0.0f;
        Explosion* oldestExplosion = nullptr;
        
        for (auto& explosion : explosions) {
            if (oldestExplosion == nullptr || explosion.startTime > oldestTime) {
                oldestExplosion = &explosion;
                oldestTime = explosion.startTime;
            }
        }
        
        if (oldestExplosion) {
            oldestExplosion->position = position;
            oldestExplosion->startTime = 0.0f;
            oldestExplosion->active = true;
            createExplosionParticles(*oldestExplosion);
        }
    }

    void createExplosionParticles(Explosion& explosion) {
        const int WAVE_COUNT = 4;
        int particlesPerWave = MAX_PARTICLES / (WAVE_COUNT * MAX_EXPLOSIONS);
        struct WaveParams {
            float minSpeed, maxSpeed;
            float minSize, maxSize;
            float minLife, maxLife;
            glm::vec3 baseColor;
        };
        
        WaveParams waves[WAVE_COUNT] = {
            {40.0f, 60.0f, 25.0f, 40.0f, 1.5f, 2.5f, {1.0f, 0.9f, 0.3f}},
            {30.0f, 45.0f, 20.0f, 30.0f, 2.0f, 3.0f, {1.0f, 0.6f, 0.2f}},
            {20.0f, 35.0f, 15.0f, 25.0f, 2.5f, 3.5f, {1.0f, 0.3f, 0.1f}},
            {10.0f, 25.0f, 5.0f, 15.0f, 3.0f, 4.0f, {0.8f, 0.2f, 0.1f}}
        };
        
        int particleIndex = 0;
        
        for (int wave = 0; wave < WAVE_COUNT; wave++) {
            for (int i = 0; i < particlesPerWave && particleIndex < MAX_PARTICLES; i++) {
                float theta = generateRandomFloat(0.0f, 2.0f * M_PI);
                float phi = generateRandomFloat(0.0f, M_PI);
                float x = sin(phi) * cos(theta);
                float y = sin(phi) * sin(theta);
                float z = cos(phi);        

                if (particleIndex >= MAX_PARTICLES) 
                    break;

                auto& p = particles[particleIndex++];
                float offset = 0.8f + 0.2f * static_cast<float>(wave) / WAVE_COUNT;
                p.x = explosion.position.x + x * offset;
                p.y = explosion.position.y + y * offset;
                p.z = explosion.position.z + z * offset;
                float speed = generateRandomFloat(waves[wave].minSpeed, waves[wave].maxSpeed);
                p.vx = x * speed;
                p.vy = y * speed;
                p.vz = z * speed;
                p.vx += generateRandomFloat(-5.0f, 5.0f);
                p.vy += generateRandomFloat(-5.0f, 5.0f);
                p.vz += generateRandomFloat(-5.0f, 5.0f);
                const auto& baseColor = waves[wave].baseColor;
                p.r = baseColor.r * generateRandomFloat(0.9f, 1.1f);
                p.g = baseColor.g * generateRandomFloat(0.9f, 1.1f);
                p.b = baseColor.b * generateRandomFloat(0.9f, 1.1f);
                p.alpha = 0.1f; 
                p.size = generateRandomFloat(waves[wave].minSize, waves[wave].maxSize);
                p.maxLife = generateRandomFloat(waves[wave].minLife, waves[wave].maxLife);
                p.life = 0.0f; 
                p.active = true;
            }
        }
        activeParticles = particleIndex;
    }
    
    void update(float deltaTime) {
        for (auto& explosion : explosions) {
            if (explosion.active) {
                explosion.startTime += deltaTime;
                if (explosion.startTime >= explosion.lifeTime) {
                    explosion.active = false;
                }
            }
        }
        
        if (activeParticles == 0) return;
        
        int newActiveCount = 0;
        
        for (auto& p : particles) {
            if (!p.active) continue;
            
            p.x += p.vx * deltaTime;
            p.y += p.vy * deltaTime;
            p.z += p.vz * deltaTime;
            
            p.vx *= 0.98f;
            p.vy *= 0.98f;
            p.vz *= 0.98f;
            
            p.vy -= 0.5f * deltaTime;
            
            p.life += deltaTime;
            float lifeRatio = p.life / p.maxLife;
            
            if (lifeRatio < 0.2f) {
                p.alpha = lifeRatio / 0.2f;
            } else if (lifeRatio > 0.8f) {
                p.alpha = (1.0f - lifeRatio) / 0.2f;
            } else {
                p.alpha = 1.0f;
            }
            
            if (lifeRatio < 0.3f) {
                p.size *= 1.01f;
            } else {
                p.size *= 0.99f;
            }
            
            if (lifeRatio > 0.6f) {
                p.r = 1.0f - (lifeRatio - 0.6f) * 2.0f; 
                p.g = 0.6f * (1.0f - (lifeRatio - 0.6f) * 2.5f);
                p.b = 0.0f;
            }
        
            if (p.life >= p.maxLife || p.alpha < 0.01f) {
                p.active = false;
            } else {
                newActiveCount++;
            }
        }
        
        activeParticles = newActiveCount;
    }

    void reset() {
        for (auto& p : particles) {
            p.active = false;
        }
        activeParticles = 0;
    }
    
    void setProjectionMatrix(const glm::mat4& proj) { projectionMatrix = proj; }
    void setViewMatrix(const glm::mat4& view) { viewMatrix = view; }
    void setTextureID(GLuint id) { textureID = id; }
    
protected:
    gl::ShaderProgram shader;
    std::vector<Particle> particles;
    GLuint vao = 0, vbo = 0;
    int activeParticles = 0;
    glm::mat4 projectionMatrix{1.0f};
    glm::mat4 viewMatrix{1.0f};
    GLuint textureID = 0;
};

#ifndef __EMSCRIPTEN__
const char* star_vSource = R"(#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec3 lightPos;
uniform vec3 viewPos;

out vec3 Normal;
out vec3 FragPos;
out vec2 TexCoords;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoords = aTexCoords;
    
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char* star_fSource = R"(#version 330 core
in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform sampler2D starTexture;

void main() {
    float ambientStrength = 0.4;
    vec3 ambient = ambientStrength * vec3(1.0, 1.0, 1.0);

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);

    float specularStrength = 0.8;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * vec3(1.0, 1.0, 1.0);
    
    vec4 texColor = texture(starTexture, TexCoords);
    vec3 result = (ambient + diffuse + specular) * texColor.rgb;
    FragColor = vec4(result, 1.0);
}
)";
#else
const char* star_vSource = R"(#version 300 es
precision highp float;
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec3 lightPos;
uniform vec3 viewPos;

out vec3 Normal;
out vec3 FragPos;
out vec2 TexCoords;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoords = aTexCoords;
    
    gl_Position = projection * view * model * vec4(aPos, 1.0);
})";

const char* star_fSource = R"(#version 300 es
precision highp float;
in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform sampler2D starTexture;

void main() {
    
    float ambientStrength = 0.8;
    vec3 ambient = ambientStrength * vec3(1.0, 1.0, 1.0);
    
    
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);
    
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0); 
    vec3 specular = specularStrength * spec * vec3(1.0, 1.0, 1.0);
    
    vec4 texColor = texture(starTexture, TexCoords);
    vec3 result = (ambient + diffuse + specular) * texColor.rgb;
    FragColor = vec4(result, 1.0);
}
)";
#endif

class Exhaust {
public:
    struct ExhaustParticle {
        glm::vec3 pos;
        glm::vec3 velocity;
        float life;     
        float maxLife;  
    };

    Exhaust() = default;
    ~Exhaust() {
        glDeleteVertexArrays(1, &exhaustVAO);
        glDeleteBuffers(3, exhaustVBO);
    }

    std::vector<ExhaustParticle> exhaustParticles;
    
    const size_t maxExhaustParticles = 50;  

    void spawnExhaustParticle(const glm::vec3& shipPos, const glm::vec3& shipRotation) {
        glm::mat4 rotationMatrix(1.0f);
        rotationMatrix = glm::rotate(rotationMatrix, glm::radians(shipRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        rotationMatrix = glm::rotate(rotationMatrix, glm::radians(shipRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        rotationMatrix = glm::rotate(rotationMatrix, glm::radians(shipRotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        
        glm::vec4 leftOffset1(0.29f, -0.05f, 0.25f, 1.0f);   
        glm::vec4 leftOffset2(0.29f, -0.02f, 0.25f, 1.0f);   
        
        glm::vec4 rightOffset1(-0.29f, -0.05f, 0.25f, 1.0f); 
        glm::vec4 rightOffset2(-0.29f, -0.02f, 0.25f, 1.0f);
        glm::vec3 leftPos1 = shipPos + glm::vec3(rotationMatrix * leftOffset1);
        glm::vec3 leftPos2 = shipPos + glm::vec3(rotationMatrix * leftOffset2);
        glm::vec3 rightPos1 = shipPos + glm::vec3(rotationMatrix * rightOffset1);
        glm::vec3 rightPos2 = shipPos + glm::vec3(rotationMatrix * rightOffset2);
        glm::vec4 backVector(0.0f, 0.0f, 1.0f, 0.0f); 
        glm::vec3 backward = glm::normalize(glm::vec3(rotationMatrix * backVector));
        float randomX = generateRandomFloat(-0.05f, 0.05f);
        float randomY = generateRandomFloat(-0.05f, 0.05f);
        float randomZ = generateRandomFloat(-0.05f, 0.05f);
        ExhaustParticle p1;
        p1.pos = leftPos1;
        p1.velocity = backward * 5.0f + glm::vec3(randomX, randomY, randomZ);
        p1.life = 0.0f;
        p1.maxLife = generateRandomFloat(0.8f, 1.2f);
        exhaustParticles.push_back(p1);
        randomX = generateRandomFloat(-0.1f, 0.1f);
        randomY = generateRandomFloat(-0.1f, 0.1f);
        randomZ = generateRandomFloat(-0.1f, 0.1f);
        ExhaustParticle p2;
        p2.pos = leftPos2;
        p2.velocity = backward * 5.0f + glm::vec3(randomX, randomY, randomZ);
        p2.life = 0.0f;
        p2.maxLife = generateRandomFloat(0.8f, 1.2f);
        exhaustParticles.push_back(p2);
        randomX = generateRandomFloat(-0.1f, 0.1f);
        randomY = generateRandomFloat(-0.1f, 0.1f);
        randomZ = generateRandomFloat(-0.1f, 0.1f);
        ExhaustParticle p3;
        p3.pos = rightPos1;
        p3.velocity = backward * 5.0f + glm::vec3(randomX, randomY, randomZ);
        p3.life = 0.0f;
        p3.maxLife = generateRandomFloat(0.8f, 1.2f);
        exhaustParticles.push_back(p3);
        randomX = generateRandomFloat(-0.1f, 0.1f);
        randomY = generateRandomFloat(-0.1f, 0.1f);
        randomZ = generateRandomFloat(-0.1f, 0.1f);
        ExhaustParticle p4;
        p4.pos = rightPos2;
        p4.velocity = backward * 5.0f + glm::vec3(randomX, randomY, randomZ);
        p4.life = 0.0f;
        p4.maxLife = generateRandomFloat(0.8f, 1.2f);
        exhaustParticles.push_back(p4);
        while (exhaustParticles.size() > maxExhaustParticles) {
            exhaustParticles.erase(exhaustParticles.begin());
        }
    }

    void updateExhaustParticles(float deltaTime) {
        for(auto& p : exhaustParticles) {
            p.pos += p.velocity * deltaTime;
            p.life += deltaTime;
        }
        exhaustParticles.erase(
            std::remove_if(exhaustParticles.begin(), exhaustParticles.end(),
                [](const ExhaustParticle& p) { return p.life >= p.maxLife; }),
            exhaustParticles.end()
        );
    }


    GLuint exhaustVAO = 0;
    GLuint exhaustVBO[3];
    gl::ShaderProgram exhaustShader;


    void initializeExhaustBuffers() {
        glGenVertexArrays(1, &exhaustVAO);
        glGenBuffers(3, exhaustVBO);
        glBindVertexArray(exhaustVAO);
        glBindBuffer(GL_ARRAY_BUFFER, exhaustVBO[0]);
        glBufferData(GL_ARRAY_BUFFER, maxExhaustParticles * 3 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glBindBuffer(GL_ARRAY_BUFFER, exhaustVBO[1]);
        glBufferData(GL_ARRAY_BUFFER, maxExhaustParticles * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glBindBuffer(GL_ARRAY_BUFFER, exhaustVBO[2]);
        glBufferData(GL_ARRAY_BUFFER, maxExhaustParticles * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glBindVertexArray(0);
    }

    void drawExhaustParticles(const glm::mat4& MVP, GLuint exhaustTexture) {
        if (exhaustParticles.empty()) return; 
        
        std::vector<float> positions;
        std::vector<float> sizes;
        std::vector<float> colors;
        positions.reserve(exhaustParticles.size() * 3);
        sizes.reserve(exhaustParticles.size());
        colors.reserve(exhaustParticles.size() * 4);
        
        for(auto& p : exhaustParticles) {
            positions.push_back(p.pos.x);
            positions.push_back(p.pos.y);
            positions.push_back(p.pos.z);
            
            float lifeFactor = 1.0f - (p.life / p.maxLife);
            sizes.push_back(20.0f * lifeFactor);  
            
            colors.push_back(1.0f);
            colors.push_back(0.6f + (lifeFactor * 0.3f)); 
            colors.push_back(0.1f);                      
            colors.push_back(lifeFactor * 1.2f);         
        }
        
        
        exhaustShader.useProgram();
        
        glBindBuffer(GL_ARRAY_BUFFER, exhaustVBO[0]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, positions.size() * sizeof(float), positions.data());
        
        glBindBuffer(GL_ARRAY_BUFFER, exhaustVBO[1]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizes.size() * sizeof(float), sizes.data());
        
        glBindBuffer(GL_ARRAY_BUFFER, exhaustVBO[2]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, colors.size() * sizeof(float), colors.data());
        
        
        exhaustShader.setUniform("MVP", MVP);
        exhaustShader.setUniform("spriteTexture", 0);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, exhaustTexture);
        
        
        glBindVertexArray(exhaustVAO);
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(exhaustParticles.size()));
    }
};
class Projectiles {
    struct Projectile {
        glm::vec3 position;
        glm::vec3  prevPosition;
        glm::vec3 velocity;
        float life;
        float maxLife;
        float radius = 0.5f; 
    };

    std::vector<Projectile> projectiles;
    GLuint projectileVAO = 0;
    GLuint projectileVBO[3];
    gl::ShaderProgram projectileShader;
    glm::mat4 viewMatrix{1.0f};
    glm::mat4 projectionMatrix{1.0f};

public:
    GLuint texture = 0;

    ~Projectiles() {
        if(projectileVAO != 0) glDeleteVertexArrays(1, &projectileVAO);
        if(projectileVBO[0] != 0) glDeleteBuffers(3, projectileVBO);
        if(texture != 0) glDeleteTextures(1, &texture);
    }

    void load(gl::GLWindow *win) {
        const char* fullProjFrag = 
#ifdef __EMSCRIPTEN__
        R"(#version 300 es
        precision highp float;
        in float intensity;
        out vec4 FragColor;
        uniform sampler2D particleTexture;
            
        void main() {
            vec2 texCoord = gl_PointCoord;
            vec4 texColor = texture(particleTexture, texCoord);
            if(texColor.r < 0.7 && texColor.g < 0.7 && texColor.b < 0.7) discard;
            FragColor = vec4(texColor.rgb * intensity, texColor.a * intensity);
        })";
#else
        R"(#version 330 core
        in float intensity;
        out vec4 FragColor;
        uniform sampler2D particleTexture;
            
        void main() {
            vec2 texCoord = gl_PointCoord;
            vec4 texColor = texture(particleTexture, texCoord);
            if(texColor.r < 0.7 && texColor.g < 0.7 && texColor.b < 0.7) discard;
            FragColor = vec4(texColor.rgb * intensity, texColor.a * intensity);
        })";
#endif
const char* fullProjVert =
#ifdef __EMSCRIPTEN__
        R"(#version 300 es
        precision highp float;
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in float aSize;
        layout (location = 2) in float aIntensity;

        uniform mat4 MVP;
        out float intensity;

        void main() {
            gl_Position = MVP * vec4(aPos, 1.0);
            gl_PointSize = aSize * 2.0;
            intensity = aIntensity;
        })";
#else
        R"(#version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in float aSize;
        layout (location = 2) in float aIntensity;

        uniform mat4 MVP;
        out float intensity;

        void main() {
            gl_Position = MVP * vec4(aPos, 1.0);
            gl_PointSize = aSize * 2.0;
            intensity = aIntensity;
        })";
#endif

        if (!projectileShader.loadProgramFromText(fullProjVert, fullProjFrag)) {
            throw mx::Exception("Failed to load projectile shader program");
        }

        texture = gl::loadTexture(win->util.getFilePath("data/fire.png"));

        glGenVertexArrays(1, &projectileVAO);
        glGenBuffers(3, projectileVBO);
        glBindVertexArray(projectileVAO);
        glBindBuffer(GL_ARRAY_BUFFER, projectileVBO[0]);
        glBufferData(GL_ARRAY_BUFFER, 100 * 3 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glBindBuffer(GL_ARRAY_BUFFER, projectileVBO[1]);
        glBufferData(GL_ARRAY_BUFFER, 100 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glBindBuffer(GL_ARRAY_BUFFER, projectileVBO[2]);
        glBufferData(GL_ARRAY_BUFFER, 100 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glBindVertexArray(0);
    }

    void fire(const glm::vec3& position, const glm::vec3& direction, float speed = 50.0f) {
        Projectile projectile;
        projectile.position = position;
        projectile.velocity = direction * speed;
        projectile.life = 0.0f;
        projectile.maxLife = 3.0f; 
        projectiles.push_back(projectile);
        
        if (projectiles.size() > 100) {
            projectiles.erase(projectiles.begin());
        }
    }

    void setMatrices(const glm::mat4& view, const glm::mat4& projection) {
        viewMatrix = view;
        projectionMatrix = projection;
    }

    void draw(gl::GLWindow *win) {
        if (projectiles.empty()) return;
        
        std::vector<float> positions;
        std::vector<float> sizes;
        std::vector<float> intensities;
        
        positions.reserve(projectiles.size() * 3);
        sizes.reserve(projectiles.size());
        intensities.reserve(projectiles.size());
        
        for (const auto& projectile : projectiles) {
            positions.push_back(projectile.position.x);
            positions.push_back(projectile.position.y);
            positions.push_back(projectile.position.z);
            
        
            float sizeMultiplier = 1.0f + (projectile.life / projectile.maxLife) * 0.5f;
            sizes.push_back(32.0f * sizeMultiplier); 
            
        
            float lifeFactor = 1.0f - (projectile.life / projectile.maxLife);
            intensities.push_back(lifeFactor * lifeFactor); 
        }
        
        
        if (!positions.empty() && !sizes.empty() && !intensities.empty()) {
            glBindBuffer(GL_ARRAY_BUFFER, projectileVBO[0]);
            glBufferSubData(GL_ARRAY_BUFFER, 0, positions.size() * sizeof(float), positions.data());
            
            glBindBuffer(GL_ARRAY_BUFFER, projectileVBO[1]);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizes.size() * sizeof(float), sizes.data());
            
            glBindBuffer(GL_ARRAY_BUFFER, projectileVBO[2]);
            glBufferSubData(GL_ARRAY_BUFFER, 0, intensities.size() * sizeof(float), intensities.data());
            
            projectileShader.useProgram();
            
            glm::mat4 MVP = projectionMatrix * viewMatrix;
            projectileShader.setUniform("MVP", MVP);
            projectileShader.setUniform("particleTexture", 0);
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture);

#ifndef __EMSCRIPTEN__
            glEnable(GL_PROGRAM_POINT_SIZE);
#endif
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glDepthMask(GL_FALSE);
            
            glBindVertexArray(projectileVAO);
            glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(projectiles.size()));
        
#ifndef __EMSCRIPTEN__
            glDisable(GL_PROGRAM_POINT_SIZE);
#endif
            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
        }
    }

    void update(float deltaTime) {
        for (auto& projectile : projectiles) {
            projectile.prevPosition = projectile.position;
            projectile.position += projectile.velocity * deltaTime;
            projectile.life += deltaTime;
        }
        
        projectiles.erase(
            std::remove_if(projectiles.begin(), projectiles.end(),
                [](const Projectile& p) { return p.life >= p.maxLife; }),
            projectiles.end()
        );
    }
    
    std::vector<size_t> checkCollisions(const glm::vec3& asteroidPosition, float asteroidRadius, float deltaTime) {
        std::vector<size_t> hitIndices;
        
        for (size_t i = 0; i < projectiles.size(); ++i) {
            const auto& proj = projectiles[i];
            
            
            const float MAX_COLLISION_DISTANCE = 20.0f;
            if (glm::length(proj.position - asteroidPosition) > 
                MAX_COLLISION_DISTANCE + asteroidRadius + proj.radius) {
                continue; 
            }

            
            glm::vec3 prevPos = proj.prevPosition;
            glm::vec3 rayDir = glm::normalize(proj.position - prevPos);
            glm::vec3 toSphere = asteroidPosition - prevPos;
            
            float rayDirDotToSphere = glm::dot(rayDir, toSphere);
            
            
            if (rayDirDotToSphere < 0.0f)
                continue;
                
            
            float d = glm::length(toSphere - rayDir * rayDirDotToSphere);
            
            
            if (d > (asteroidRadius + proj.radius))
                continue;
                
            
            float distToClosest = rayDirDotToSphere;
            
            
            float discriminant = pow(asteroidRadius + proj.radius, 2) - pow(d, 2);
            if (discriminant < 0)
                continue;
                
            
            float distToIntersection = distToClosest - sqrt(discriminant);
            
            
            float rayLength = glm::length(proj.position - prevPos);
            if (distToIntersection <= rayLength) {
                hitIndices.push_back(i);
            }
        }
        
        
        std::sort(hitIndices.begin(), hitIndices.end(), std::greater<size_t>());
        for (auto idx : hitIndices) {
            projectiles.erase(projectiles.begin() + idx);
        }
        
        return hitIndices;
    }
    
    void clear() {
        projectiles.clear();
    }
};

class StarFighter {
public:
    Exhaust exhaust;
    Projectiles projectiles;
    GLuint exhaustTexture = 0;
    
    float minSpeed = 0.1f;
    float currentSpeed = 1.0f;
    bool visible = true;

    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 rotation{0.0f, 0.0f, 0.0f};
    glm::vec3 velocity{0.0f, 0.0f, 0.0f};
    
    float speed = 10.0f;         
    float turnSpeed = 100.0f;    
    float maxSpeed = 20.0f;      
    float drag = 0.95f;
    
    float cameraDistance = 5.0f;
    float cameraHeight = 1.5f;
    float cameraLag = 0.1f;
    glm::vec3 cameraPosition{0.0f, 0.0f, 0.0f};
    
    void load(gl::GLWindow *win) {
        if(!model.openModel(win->util.getFilePath("data/bird.mxmod.z"))) {
            throw mx::Exception("Failed to load model");
        }
        if(!shader.loadProgramFromText(star_vSource, star_fSource)) {
            throw mx::Exception("Failed to load shader program");
        }
        model.setShaderProgram(&shader, "starTexture");
        model.setTextures(win, win->util.getFilePath("data/bird.tex"), win->util.getFilePath("data"));
        
        position = glm::vec3(0.0f, 0.0f, 0.0f);
        rotation = glm::vec3(0.0f, 0.0f, 0.0f);
        velocity = glm::vec3(0.0f, 0.0f, 0.0f);
        
        exhaust.initializeExhaustBuffers();
        if(!exhaust.exhaustShader.loadProgramFromText(vertSource, fragSource)) {
            throw mx::Exception("Failed to load exhaust shader program");
        }
        
        exhaustTexture = gl::loadTexture(win->util.getFilePath("data/ember.png"));
        if (exhaustTexture == 0) {
            throw mx::Exception("Failed to load exhaust texture");
        }
        projectiles.load(win);
    }

    void moveForward(float deltaTime) {
        glm::mat4 rotationMatrix(1.0f);
        rotationMatrix = glm::rotate(rotationMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f)); 
        rotationMatrix = glm::rotate(rotationMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        
        glm::vec3 forward = glm::normalize(glm::vec3(
            -sin(glm::radians(rotation.y)),
            sin(glm::radians(rotation.x)),
            -cos(glm::radians(rotation.y)) * cos(glm::radians(rotation.x))
        ));
        
        velocity += forward * speed * deltaTime;
        float currentSpeed = glm::length(velocity);
        if (currentSpeed > maxSpeed) {
            velocity = velocity * (maxSpeed / currentSpeed);
        }
    }
    
    void moveBackward(float deltaTime) {
        glm::vec3 forward = glm::normalize(glm::vec3(
            -sin(glm::radians(rotation.y)),
            sin(glm::radians(rotation.x)),
            -cos(glm::radians(rotation.y)) * cos(glm::radians(rotation.x))
        ));
        
        velocity -= forward * speed * deltaTime;
        
        float currentSpeed = glm::length(velocity);
        if (currentSpeed > maxSpeed) {
            velocity = velocity * (maxSpeed / currentSpeed);
        }
    }
    
    void yaw(float amount, float deltaTime) {
        float absPitch = fabs(rotation.x);
        if (absPitch >= 75.0f) {
             float rollDirection = (rotation.x > 0) ? -1.0f : 1.0f;
             roll(amount * rollDirection * turnSpeed * deltaTime, deltaTime);
        } else {
             rotation.y += amount * turnSpeed * deltaTime;
        }
        
        while (rotation.y >= 360.0f) rotation.y -= 360.0f;
        while (rotation.y < 0.0f) rotation.y += 360.0f;
    }
    
    
    void pitch(float amount, float deltaTime) {
        rotation.x += amount * turnSpeed * deltaTime;
        rotation.x = glm::clamp(rotation.x, -80.0f, 80.0f);
    }
    
    void roll(float amount, float deltaTime) {
        rotation.z += amount * turnSpeed * deltaTime;
        while (rotation.z > 360.0f) rotation.z -= 360.0f;
        while (rotation.z < -360.0f) rotation.z += 360.0f;
    }
    
    bool pause = false;

    void setPause(bool pause1) {
        pause = pause1;
    }

    void update(float deltaTime) {
        glm::vec3 forward = glm::normalize(glm::vec3(
            -sin(glm::radians(rotation.y)),
            sin(glm::radians(rotation.x)),
            -cos(glm::radians(rotation.y)) * cos(glm::radians(rotation.x))
        ));

        if(pause == false) {
            velocity = forward * currentSpeed;
            position += velocity * deltaTime;
        }
        if (fabs(rotation.z) > 3.0f) {  
            float bankFactor = rotation.z / 45.0f;  
            float turnRate = 15.0f * bankFactor * (currentSpeed / maxSpeed);
            rotation.y -= turnRate * deltaTime;
            while (rotation.y > 360.0f) rotation.y -= 360.0f;
            while (rotation.y < 0.0f) rotation.y += 360.0f;
        }
        
        updateCamera(deltaTime);
        
        static float exhaustTimer = 0.0f;
        exhaustTimer += deltaTime;
        
        float spawnInterval = 0.05f - (currentSpeed / maxSpeed) * 0.04f; 
        
        if (exhaustTimer >= spawnInterval && currentSpeed > minSpeed * 1.2f) {
            exhaust.spawnExhaustParticle(position, rotation);
            exhaustTimer = 0.0f;
        }
        
        exhaust.updateExhaustParticles(deltaTime);
        projectiles.update(deltaTime);      
    }

    void increaseSpeed(float deltaTime) {
        currentSpeed += speed * deltaTime * 2.0f;
        if (currentSpeed > maxSpeed) {
            currentSpeed = maxSpeed;
        }
    }

    void decreaseSpeed(float deltaTime) {
        currentSpeed -= speed * deltaTime * 2.0f;
        if (currentSpeed < minSpeed) {
            currentSpeed = minSpeed;
        }
    }
    
    void updateCamera(float deltaTime) {
        
        glm::vec3 forward = glm::normalize(glm::vec3(
            -sin(glm::radians(rotation.y)),
            sin(glm::radians(rotation.x)),
            -cos(glm::radians(rotation.y)) * cos(glm::radians(rotation.x))
        ));
        
        glm::vec3 idealCameraPos = position - forward * cameraDistance + glm::vec3(0.0f, cameraHeight, 0.0f);
        
        
        cameraPosition = glm::mix(cameraPosition, idealCameraPos, 1.0f - exp(-deltaTime * 10.0f));
    }
    
    glm::mat4 getViewMatrix() {
        return glm::lookAt(
            cameraPosition,                      
            position,                            
            glm::vec3(0.0f, 1.0f, 0.0f)          
        );
    }

    void draw(gl::GLWindow *win, const glm::mat4& projection, const glm::vec3& lightPos) {
        if (!visible) {
            glm::mat4 viewMatrix = getViewMatrix();
            projectiles.setMatrices(viewMatrix, projection);
            projectiles.draw(win);
            return;
        }
        glm::mat4 viewMatrix = getViewMatrix();     
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);
#ifndef __EMSCRIPTEN__
        glEnable(GL_PROGRAM_POINT_SIZE);
#endif
        
        glm::mat4 MVP = projection * viewMatrix;
        exhaust.drawExhaustParticles(MVP, exhaustTexture);
        
#ifndef __EMSCRIPTEN__
        glDisable(GL_PROGRAM_POINT_SIZE);
#endif
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        
        shader.useProgram();
        
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, position);
        
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        
        modelMatrix = glm::scale(modelMatrix, glm::vec3(0.5f, 0.5f, 0.5f));
        
        shader.setUniform("model", modelMatrix);
        shader.setUniform("view", viewMatrix);
        shader.setUniform("projection", projection);
        shader.setUniform("lightPos", lightPos);
        shader.setUniform("viewPos", cameraPosition);
        model.drawArrays();
        projectiles.setMatrices(viewMatrix, projection);
        projectiles.draw(win);
    }

    void fireProjectile() {
        glm::vec3 forward = glm::normalize(glm::vec3(
            -sin(glm::radians(rotation.y)),
            sin(glm::radians(rotation.x)),
            -cos(glm::radians(rotation.y)) * cos(glm::radians(rotation.x))
        ));
        
        glm::vec3 projectilePos = position + forward * 0.6f;
        projectiles.fire(projectilePos, forward, 80.0f + currentSpeed);
    }

protected:
    mx::Model model;
    gl::ShaderProgram shader;
};

const float BOUNDARY_X_MIN = -150.0f;
const float BOUNDARY_X_MAX = 150.0f;
const float BOUNDARY_Y_MIN = -100.0f;
const float BOUNDARY_Y_MAX = 100.0f;
const float BOUNDARY_Z_MIN = -150.0f;
const float BOUNDARY_Z_MAX = 150.0f;
const float BOUNDARY_BOUNCE_FACTOR = 1.2f;
constexpr int MAX_GENERATIONS = 2;
constexpr int CHILDREN_PER_SPAWN = 2;
constexpr int MAX_PLANETS = 7 * (1 + CHILDREN_PER_SPAWN + CHILDREN_PER_SPAWN * CHILDREN_PER_SPAWN);



class Planet {
public:
    Planet() = default;
    
    Planet(const Planet&) = delete;
    Planet& operator=(const Planet&) = delete;
    
    Planet(Planet&& other) noexcept 
        : position(other.position), 
          rotationSpeed(other.rotationSpeed),
          rotationAngle(other.rotationAngle),
          scale(other.scale),
          time_f(other.time_f),
          isDestroyed(other.isDestroyed),
          radius(other.radius),
          generation(other.generation), 
          velocity(other.velocity) {}
          
    Planet& operator=(Planet&& other) noexcept {
        if (this != &other) {
            position = other.position;
            rotationSpeed = other.rotationSpeed;
            rotationAngle = other.rotationAngle;
            scale = other.scale;
            time_f = other.time_f;
            isDestroyed = other.isDestroyed;
            radius = other.radius;
            generation = other.generation; 
            velocity = other.velocity;     
        }
        return *this;
    }
    
    ~Planet() = default;

    glm::vec3 position{0.0f, 0.0f, -30.0f}; 
    float rotationSpeed = 75.0f;             
    float rotationAngle = 0.0f;
    float scale = 5.0f;                     
    float time_f = 0.0f;  
    int planet_type = 0;
    bool isDestroyed = true;
    bool isActive = false;
    float radius = 5.0f;
    int generation = 0;
    int asteroid_type = 0;
    glm::vec3 velocity{0.0f, 0.0f, 0.0f};  
    

    float getRadius() const {
         return radius;
    }

    void load(gl::GLWindow *win) {
        static bool loaded = false;
        if(loaded == false) {
            models[0] = std::make_unique<mx::Model>();
            if(!models[0]->openModel(win->util.getFilePath("data/asteroid.mxmod.z"))) {
                    throw mx::Exception("Failed to load planet model");
            }
            int rnd_color = generateRandomInt(0, 1);
            if(rnd_color == 0) {
                models[0]->setTextures(win, win->util.getFilePath("data/rock.tex"), win->util.getFilePath("data"));
            } else {
                models[0]->setTextures(win, win->util.getFilePath("data/rock2.tex"), win->util.getFilePath("data"));
            }
            models[1] = std::make_unique<mx::Model>();
            if(!models[1]->openModel(win->util.getFilePath("data/asteroid2.mxmod.z"))) {
                    throw mx::Exception("Failed to load planet model");
            }
            rnd_color = generateRandomInt(0, 1);
            if(rnd_color == 0) {
                models[1]->setTextures(win, win->util.getFilePath("data/rock.tex"), win->util.getFilePath("data"));
            } else {
                models[1]->setTextures(win, win->util.getFilePath("data/rock2.tex"), win->util.getFilePath("data"));
            }
            models[2] = std::make_unique<mx::Model>();
            if(!models[2]->openModel(win->util.getFilePath("data/asteroid3.mxmod.z"))) {
                    throw mx::Exception("Failed to load planet model");
            }
            rnd_color = generateRandomInt(0, 1);
            if(rnd_color == 0) {
                models[2]->setTextures(win, win->util.getFilePath("data/rock.tex"), win->util.getFilePath("data"));
            } else {
                models[2]->setTextures(win, win->util.getFilePath("data/rock2.tex"), win->util.getFilePath("data"));
            }
            
            shader = std::make_unique<gl::ShaderProgram>();
            if(!shader->loadProgramFromText(g_vSource, g_fSource)) {
                throw mx::Exception("Failed to load planet shader program");
            }
            models[0]->setShaderProgram(shader.get(), "texture1");
            models[1]->setShaderProgram(shader.get(), "texture1");
            models[2]->setShaderProgram(shader.get(), "texture1");
            loaded = true;
            
        }
        planet_type = 3;
        shader->useProgram();
    }

    void update(float deltaTime) {
        rotationAngle += rotationSpeed * deltaTime;
        if(rotationAngle > 360.0f) rotationAngle -= 360.0f;

        position += velocity * deltaTime;
        

        bool bounced = false;
        

        if (position.x < BOUNDARY_X_MIN) {
            position.x = BOUNDARY_X_MIN;
            velocity.x = -velocity.x * BOUNDARY_BOUNCE_FACTOR;
            bounced = true;
        }
        else if (position.x > BOUNDARY_X_MAX) {
            position.x = BOUNDARY_X_MAX;
            velocity.x = -velocity.x * BOUNDARY_BOUNCE_FACTOR;
            bounced = true;
        }
        

        if (position.y < BOUNDARY_Y_MIN) {
            position.y = BOUNDARY_Y_MIN;
            velocity.y = -velocity.y * BOUNDARY_BOUNCE_FACTOR;
            bounced = true;
        }
        else if (position.y > BOUNDARY_Y_MAX) {
            position.y = BOUNDARY_Y_MAX;
            velocity.y = -velocity.y * BOUNDARY_BOUNCE_FACTOR;
            bounced = true;
        }
        

        if (position.z < BOUNDARY_Z_MIN) {
            position.z = BOUNDARY_Z_MIN;
            velocity.z = -velocity.z * BOUNDARY_BOUNCE_FACTOR;
            bounced = true;
        }
        else if (position.z > BOUNDARY_Z_MAX) {
            position.z = BOUNDARY_Z_MAX;
            velocity.z = -velocity.z * BOUNDARY_BOUNCE_FACTOR;
            bounced = true;
        }
    
        if (bounced) {
            velocity.x += generateRandomFloat(-0.5f, 0.5f);
            velocity.y += generateRandomFloat(-0.5f, 0.5f);
            velocity.z += generateRandomFloat(-0.5f, 0.5f);
            float speed = glm::length(velocity);
            if (speed < 2.0f) {
                velocity = glm::normalize(velocity) * 2.0f;
            }
        }
    
        if (glm::length(velocity) > 0.01f) {
            velocity *= 0.995f; 
        }
        
        time_f += deltaTime;
        if(time_f > 10000.0f) time_f = 0.0f;
    }

    void draw(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& lightPos, const glm::vec3& viewPos) {
        if (isDestroyed) return;

        float distance = glm::length(position - viewPos);
        if (distance > 280.0f) return;
        float apparentScale = scale;
        if (distance > 100.0f) {
            apparentScale = scale * (1.0f + (distance - 100.0f) * 0.01f);
        }

        planet_type = 3;

        float modelScale = (planet_type == 2) ? (apparentScale / 6.0f) : apparentScale;
        
        shader->useProgram();
        
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, position);
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrix = glm::scale(modelMatrix, glm::vec3(modelScale, modelScale, modelScale));
        
        shader->setUniform("model", modelMatrix);
        shader->setUniform("view", view);
        shader->setUniform("projection", projection);
        shader->setUniform("time_f", time_f);
        shader->setUniform("effect_type", planet_type);
        models[asteroid_type]->drawArrays();
    }

    void reset() {
        isDestroyed = true;
        isActive = false;
    }

    

    void spawnChildAsteroids(std::vector<Planet>& planets) {
        if (generation >= MAX_GENERATIONS) return;

        con->printf("Asteroid splitting: gen %d -> %d\n", generation, generation + 1);
        con->printf("Asteroid Position: %f. %f. %f\n", position.x, position.y, position.z);

        for (int i = 0; i < CHILDREN_PER_SPAWN; ++i) {
            int availCount = 0;
            for (const auto& p : planets) {
                if (!p.isActive && p.isDestroyed) availCount++;
            }
            con->printf("Available asteroids: %d/%d\n", availCount, planets.size());
            auto it = std::find_if(planets.begin(), planets.end(),
                [](const Planet& p) { return !p.isActive && p.isDestroyed; });
            
            if (it == planets.end()) {
                con->println("Astertoid pool exhausted! Increase MAX_PLANETS.");
                return;
            }
            
            Planet& child = *it;
            child.isDestroyed = false;
            child.isActive = true;
            child.planet_type = 3;
            child.position = position;
            float offset = radius * 0.5f;
            child.position.x += generateRandomFloat(-offset, offset);
            child.position.y += generateRandomFloat(-offset, offset);
            child.position.z += generateRandomFloat(-offset, offset);
            child.scale = scale * 0.4f;
            child.radius = radius * 0.4f;
            child.rotationSpeed = rotationSpeed * generateRandomFloat(0.8f, 1.5f);
            child.rotationAngle = generateRandomFloat(0.0f, 360.0f);
            child.asteroid_type = generateRandomInt(0, 2);
            child.generation = generation + 1;
            glm::vec3 directionFromParent = glm::normalize(child.position - position);
            child.velocity = directionFromParent * generateRandomFloat(2.0f, 5.0f);
        }
    }
    static std::unique_ptr<mx::Model> models[3];
    static std::unique_ptr<gl::ShaderProgram> shader;
};

std::unique_ptr<mx::Model> Planet::models[3] = {nullptr,nullptr,nullptr};
std::unique_ptr<gl::ShaderProgram> Planet::shader = nullptr;

class Game : public gl::GLObject {
    ExplodeEmiter emiter;
    StarField field;
    StarFighter ship;  
    std::vector<Planet> planets;    
    static const int NUM_PLANETS = 7;
    GLuint texture = 0;
    mx::Controller controller;
    bool shipExploding = false;
    float shipExplosionTimer = 0.0f;
    const float SHIP_EXPLOSION_DURATION = 2.5f;
    int lives = 5;
    int score = 0;
    bool inverted_controls = true;
public:
    Game() = default;
    virtual ~Game() override {
        planets.clear();
        for (int i = 0; i < 3; ++i) {
            if (Planet::models[i]) {
                Planet::models[i].reset();
            }
        }
        if (Planet::shader) {
            Planet::shader.reset();
        }
        if (texture)
            glDeleteTextures(1, &texture);
    }
    void load(gl::GLWindow *win) override {
        font.loadFont(win->util.getFilePath("data/font.ttf"), 18);
        win->console.println("Asteroids MX2");
        win->console.println("written by LostSideDead Software\nhttps://lostsidedead.biz");
        con = &win->console;
        emiter.load(win);
        emiter.setTextureID(gl::loadTexture(win->util.getFilePath("data/star.png")));        
        field.load(win);
        ship.load(win);  
        planets.resize(MAX_PLANETS);
        for (auto& planet : planets) {
            planet.load(win);
            planet.isActive = false;
            planet.isDestroyed = true;
        }
        randomizePlanetPositions();
        field.repositionStarsAroundCamera(ship.cameraPosition);
        win->console.setCallback(win, [&](gl::GLWindow *window, const std::vector<std::string> &args) -> bool {
            if(args.size() == 0) {
                return false;
            }
            if(args.size() == 1 && args[0] == "randomize") {
                randomizePlanetPositions();
                return true;
            } else if(args.size() == 4 && args[0] == "setpos") {
                float x = std::stof(args[1]);
                float y = std::stof(args[2]);
                float z = std::stof(args[3]);
                ship.position = glm::vec3(x, y, z);
                ship.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
                ship.velocity = glm::vec3(0.0f);
                window->console.printf("Ship position set to: %f, %f, %f\n", x, y, z);
                return true;
            } else if(args.size() == 1 && args[0] == "toggle") {
                window->console_visible = !window->console_visible;
                if(window->console_visible)
                    window->console.show();
                else
                    window->console.hide();
                return true;
            } 
            return window->console.procDefaultCommands(args);
        });
    }
    
    void randomizePlanetPositions() {
        con->println("Randomizing asteroid positions...");
        lives = 5;
        score = 0;
        const float minX = -100.0f, maxX = 100.0f;
        const float minY = -50.0f, maxY = 50.0f;
        const float minZ = -100.0f, maxZ = 100.0f;
        const float minDistanceBetweenPlanets = 20.0f;
        
        for(int i = 0; i < static_cast<int>(planets.size()); i++) {
            planets[i].reset();
            planets[i].isActive = false;
            planets[i].isDestroyed = true;
        }

        for (int i = 0; i < 7; i++) {
            bool validPosition = false;
            glm::vec3 newPos;
        
            planets[i].planet_type = 3;
         
            while (!validPosition) {
                newPos = glm::vec3(
                    generateRandomFloat(minX, maxX),
                    generateRandomFloat(minY, maxY),
                    generateRandomFloat(minZ, maxZ)
                );
                
                if (glm::length(newPos - ship.position) < 30.0f) {
                    continue;
                }
                
                validPosition = true;
                for (int j = 0; j < i; j++) {
                    if (glm::length(newPos - planets[j].position) < minDistanceBetweenPlanets) {
                        validPosition = false;
                        break;
                    }
                }
            }

            planets[i].position = newPos;
            planets[i].radius = generateRandomFloat(3.0f, 7.0f);
            planets[i].scale = planets[i].radius;
            planets[i].rotationSpeed = generateRandomFloat(1.0f, 5.0f);
            planets[i].rotationAngle = generateRandomFloat(0.0f, 360.0f);
            planets[i].isDestroyed = false;
            planets[i].isActive = true;       
            planets[i].generation = 0; 
            planets[i].velocity = glm::vec3(0.0f);
            planets[i].asteroid_type = generateRandomInt(0, 2);
            int availablePlanets = 0;
            for (const auto& p : planets) {
                if (!p.isActive && p.isDestroyed) availablePlanets++;
            }
            con->printf("Available asteroids after initialization: %d/%d\n", availablePlanets, planets.size());
        }
    }

    void draw (gl::GLWindow *win) override {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; 
        lastUpdateTime = currentTime;
        
        if (deltaTime > 0.1f) deltaTime = 0.1f;
        
        handleInput(win, deltaTime);
        
        if(win->console_visible == true)
            ship.setPause(true);
        else 
            ship.setPause(false);

             ship.update(deltaTime);
        
        for (auto& planet : planets) {
            planet.update(deltaTime);
        }
        
        float aspectRatio = static_cast<float>(win->w) / static_cast<float>(win->h);
        projectionMatrix = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 300.0f);
        
        viewMatrix = ship.getViewMatrix();
        
        field.setViewProjectionMatrices(viewMatrix, projectionMatrix);  
        emiter.setProjectionMatrix(projectionMatrix);
        emiter.setViewMatrix(viewMatrix);
        
        field.draw(win);
        field.update(deltaTime);
        glEnable(GL_DEPTH_TEST);
        for (auto& planet : planets) {
            planet.draw(viewMatrix, projectionMatrix, lightPos, ship.cameraPosition);
        }
        
        ship.draw(win, projectionMatrix, lightPos);
        
        for (auto& planet : planets) {
            if (!planet.isDestroyed) {
                auto hits = ship.projectiles.checkCollisions(planet.position, planet.getRadius(), deltaTime);
                if (!hits.empty()) {
                    
                    emiter.position = planet.position;
                    emiter.explode(planet.position);                       
                    if (planet.generation >= MAX_GENERATIONS) {
                        emiter.explode(planet.position);
                        planet.isDestroyed = true;                                                                                                                      
                        score += 100 * hits.size();  
                    } else {
                        emiter.explode(planet.position);
                        planet.spawnChildAsteroids(planets);
                        planet.isDestroyed = true;
                        score += 10 * hits.size();
                    }
                }
                
                float distance = glm::length(ship.position - planet.position);
                if (distance < planet.getRadius() + 1.0f) {
                    emiter.position = ship.position;
                    emiter.explode(ship.position);
                    shipExploding = true;
                    shipExplosionTimer = 0.0f;
                    ship.visible = false;
                }
            }
        }

        if (shipExploding) {
            shipExplosionTimer += deltaTime;
            if (shipExplosionTimer >= SHIP_EXPLOSION_DURATION) {
                shipExploding = false;
                ship.position = glm::vec3(0.0f, 0.0f, 0.0f);
                ship.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
                ship.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
                ship.currentSpeed = ship.minSpeed;
                ship.visible = true;
                ship.cameraPosition = glm::vec3(0.0f, ship.cameraHeight, ship.cameraDistance);
                lives--;
                if (lives <= 0) {
                    con->printf("Game Over! Final Score: %d\n", score);
                    ship.visible = false;
                } else {
                    con->printf("Ship destroyed! Lives left: %d\n", lives); 
                }           
                if(lives <= 0) {
                    randomizePlanetPositions();
                }
                
            }
        }

        emiter.update(deltaTime);
        emiter.draw(win);

        win->text.setColor({0xBD, 0, 0, 255});
        win->text.printText_Solid(font, win->w-250.0f, 25.0f, "MX2 Asteroids " + std::string(ASTEROIDS_VERSION));
        win->text.setColor({255,255,255,255});
        win->text.printText_Solid(font, win->w-250.0f, 50.0f, "Score: " + std::to_string(score));
        win->text.printText_Solid(font, win->w-250.0f, 75.0f, "Lives: " + std::to_string(lives));
        
         int numPlanets = 0;
         for(auto &p : planets) {
             if(!p.isDestroyed) numPlanets++;
         }

         if(numPlanets == 0) {
            randomizePlanetPositions();
         }

         win->text.printText_Solid(font, win->w-250.0f, 100.0f, "Asteroids: " + std::to_string(numPlanets));
         win->text.printText_Solid(font, win->w-250.0f, 125.0f, "[F1 for Debug]");
         if(inverted_controls) {
            win->text.printText_Solid(font, win->w-250.0f, 150.0f, "[F2 invert controls]");
         } else {
            win->text.printText_Solid(font, win->w-250.0f, 150.0f, "[F2 restore controls]");
         }
         win->text.printText_Solid(font, win->w-250.0f, 175.0f, "[F3 for Console]");

        if(debug_menu) {
            win->text.setColor({255,255,255,255});
            win->text.printText_Solid(font,25.0f,25.0f, "Ship X,Y,Z: " + std::to_string(ship.position.x) + ", " + std::to_string(ship.position.y) + ", " + std::to_string(ship.position.z));
            win->text.printText_Solid(font,25.0f,50.0f, "Velocity X,Y,Z: " + std::to_string(ship.velocity.x) + ", " + std::to_string(ship.velocity.y) + ", " + std::to_string(ship.velocity.z)); 
            win->text.printText_Solid(font,25.0f,75.0f, "FPS: " + std::to_string(1.0f / deltaTime));
            int destroyedCount = 0;
            for (const auto& planet : planets) {
                if (planet.isDestroyed) destroyedCount++;
            }
            win->text.printText_Solid(font,25.0f,100.0f, "Aseroids destroyed: " + std::to_string(destroyedCount));
            win->text.printText_Solid(font,25.0f,125.0f, "Controls: Arrows to Move, W,S Tilt Up/Down - SPACE to shoot");
            
            if (!planets.empty()) {
                float closestPlanet = 999999.0f;
                float farthestPlanet = 0.0f;
                
                for (const auto& planet : planets) {
                    float dist = glm::length(ship.position - planet.position);
                    closestPlanet = std::min(closestPlanet, dist);
                    farthestPlanet = std::max(farthestPlanet, dist);
                }
                
                win->text.printText_Solid(font,25.0f,150.0f, "Nearest Object: " + std::to_string(closestPlanet));
                win->text.printText_Solid(font,25.0f,175.0f, "Farthest Object: " + std::to_string(farthestPlanet));
            }
            win->text.printText_Solid(font, 25.0f, 200.0f, 
                "Speed: " + std::to_string(ship.currentSpeed) + " / " + std::to_string(ship.maxSpeed));
            std::string con_str = controller.active() ? ("Connected: " + controller.name()) : "Disconnected";
            win->text.printText_Solid(font, 25.0f, 225.0f, "Controller: " + con_str);
            win->text.printText_Solid(font, 25.0f, 250.0f, "Press ENTER to randomize asteroids");
        }
    }
    
    void handleInput(gl::GLWindow* win, float deltaTime) {

        if(win->console_visible == true) {
            return;
        }

        const Uint8* state = SDL_GetKeyboardState(NULL);
        Uint32 currentTime = SDL_GetTicks();
        if (state[SDL_SCANCODE_SPACE]) {
            if (currentTime - lastFireTime >= FIRE_COOLDOWN) {
                ship.fireProjectile();
                lastFireTime = currentTime;
            }
        }
        if (controller.getButton(SDL_CONTROLLER_BUTTON_A)) {
            if (currentTime - lastFireTime >= FIRE_COOLDOWN) {        
                ship.fireProjectile();
                lastFireTime = currentTime;
            }
        } 
        if(controller.getButton(SDL_CONTROLLER_BUTTON_LEFTSHOULDER)) {
            ship.increaseSpeed(deltaTime); 
        }
        else if(controller.getButton(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)) {
            ship.decreaseSpeed(deltaTime); 
        }
        if (state[SDL_SCANCODE_UP]) {
            ship.increaseSpeed(deltaTime); 
        }
        else if (state[SDL_SCANCODE_DOWN]) {
            ship.decreaseSpeed(deltaTime); 
        }
        else {   
            if (ship.currentSpeed > 5.0f) {
                ship.decreaseSpeed(deltaTime * 0.5f);
            }
            else if (ship.currentSpeed < 5.0f) {
                ship.increaseSpeed(deltaTime * 0.5f);
            }
        }

        bool rollingInput = false;

        if (state[SDL_SCANCODE_LEFT]) {
            ship.yaw(1.0f, deltaTime);
            
                      
            float targetRoll = -30.0f; 
            float rollInput;
            
            if (ship.rotation.z > targetRoll + 5.0f) {
                
                rollInput = -1.0f;
            } else if (ship.rotation.z < targetRoll - 5.0f) {
                
                rollInput = 0.5f;
            } else {
                
                rollInput = (targetRoll - ship.rotation.z) * 0.05f;
            }
            
            ship.roll(rollInput, deltaTime);
            rollingInput = true;
        }
        else if (state[SDL_SCANCODE_RIGHT]) {
            ship.yaw(-1.0f, deltaTime);
            
            float targetRoll = 30.0f; 
            float rollInput;
            
            if (ship.rotation.z < targetRoll - 5.0f) {
                
                rollInput = 1.0f;
            } else if (ship.rotation.z > targetRoll + 5.0f) {
                
                rollInput = -0.5f;
            } else {
                rollInput = (targetRoll - ship.rotation.z) * 0.05f;
            }
            
            ship.roll(rollInput, deltaTime);
            rollingInput = true;
        }

        if(controller.getAxis(SDL_CONTROLLER_AXIS_LEFTX) > 0.5f) {
            ship.yaw(-1.0f, deltaTime);
            
            float targetRoll = 30.0f; 
            float rollInput;
            
            if (ship.rotation.z < targetRoll - 5.0f) {
                rollInput = 1.0f;
            } else if (ship.rotation.z > targetRoll + 5.0f) {
                rollInput = -0.5f;
            } else {
                rollInput = (targetRoll - ship.rotation.z) * 0.05f;
            }
            
            ship.roll(rollInput, deltaTime);
            rollingInput = true;
        }
        else if(controller.getAxis(SDL_CONTROLLER_AXIS_LEFTX) < -0.5f) {
            ship.yaw(1.0f, deltaTime);
            
            float targetRoll = -30.0f; 
            float rollInput;
            
            if (ship.rotation.z > targetRoll + 5.0f) {
                rollInput = -1.0f;
            } else if (ship.rotation.z < targetRoll - 5.0f) {
                rollInput = 0.5f;
            } else {
                rollInput = (targetRoll - ship.rotation.z) * 0.05f;
            }
            
            ship.roll(rollInput, deltaTime);
            rollingInput = true;
        }

        if (!rollingInput) {
            while (ship.rotation.z > 180.0f) ship.rotation.z -= 360.0f;
            while (ship.rotation.z < -180.0f) ship.rotation.z += 360.0f;    
            float wingLevelingSpeed = 2.0f;
            ship.rotation.z = glm::mix(ship.rotation.z, 0.0f, wingLevelingSpeed * deltaTime);
        }
        
        if(inverted_controls) {
            if (state[SDL_SCANCODE_W]) {
                ship.pitch(-1.0f, deltaTime); 
            }
            if (state[SDL_SCANCODE_S]) {
                ship.pitch(1.0f, deltaTime); 
            }
        } else {
            if (state[SDL_SCANCODE_S]) {
                ship.pitch(-1.0f, deltaTime); 
            }
            if (state[SDL_SCANCODE_W]) {
                ship.pitch(1.0f, deltaTime); 
            }
        }

        if (state[SDL_SCANCODE_RETURN]) {
            randomizePlanetPositions();
            emiter.reset();
        }
        if(inverted_controls) {
            if(controller.getAxis(SDL_CONTROLLER_AXIS_RIGHTY) < -0.5f) {
                ship.pitch(-1.0f, deltaTime); 
            }
            else if(controller.getAxis(SDL_CONTROLLER_AXIS_RIGHTY) > 0.5f) {
                ship.pitch(1.0f, deltaTime);  
            }
        } else {
            if(controller.getAxis(SDL_CONTROLLER_AXIS_RIGHTY) < -0.5f) {
                ship.pitch(1.0f, deltaTime); 
            }
            else if(controller.getAxis(SDL_CONTROLLER_AXIS_RIGHTY) > 0.5f) {
                ship.pitch(-1.0f, deltaTime);  
            }
        }
    }
    
    void event(gl::GLWindow *win, SDL_Event &e) override {

        if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.sym == SDLK_ESCAPE) {
                win->setObject(new Intro());
                win->object->load(win);
                return;
            }
            if (e.key.keysym.sym == SDLK_F1) {
                debug_menu = !debug_menu;
            }
            if (e.key.keysym.sym == SDLK_F2) {
                inverted_controls = !inverted_controls;
            }
            if (e.key.keysym.sym == SDLK_F3) {
                win->console_visible = !win->console_visible;
             
                if(win->console_visible)
                    win->console.show();
                else
                    win->console.hide();
            }
        }   
    }
    
    virtual void resize(gl::GLWindow *win, int w, int h) override {
        win->console.resize(win, w , h);   
    }

private:
    mx::Font font;
    Uint32 lastUpdateTime = SDL_GetTicks();
    Uint32 lastFireTime = 0;           
    const Uint32 FIRE_COOLDOWN = 200;  
    glm::mat4 viewMatrix{1.0f};
    glm::mat4 projectionMatrix{1.0f};
    glm::vec3 lightPos{10.0f, 10.0f, 10.0f};
    bool debug_menu = false;
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("3D Asteroids", tw, th) {
        setPath(path);
        SDL_Surface *ico = png::LoadPNG(util.getFilePath("data/asteroids_icon.png").c_str());
        if(ico) {         
            setWindowIcon(ico);
            SDL_FreeSurface(ico);
        }
        setObject(new Intro());
        object->load(this);
        activateConsole(util.getFilePath("data/font.ttf"), 16, {255, 255, 255, 255});
    }   
    
    ~MainWindow() override {}
    
    virtual void event(SDL_Event &e) override {

        if (e.type == SDL_WINDOWEVENT) {
            if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                updateViewport();
            }
        } 

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

void Intro::draw(gl::GLWindow *win) {
    glDisable(GL_DEPTH_TEST);
    #ifndef __EMSCRIPTEN__
        Uint32 currentTime = SDL_GetTicks();
    #else
        double currentTime = emscripten_get_now();
    #endif
        shader.useProgram();
        shader.setUniform("alpha", fade);
        shader.setUniform("time_f", SDL_GetTicks() / 1000.0f);
        intro.draw();
        if((currentTime - lastUpdateTime) > 35) {
            lastUpdateTime = currentTime;
            fade -= .01;
        }
        if(fade <= 0.0) {
            win->setObject(new Game());
            win->object->load(win);
            return;
        }
        intro.draw();
        if(fade <= 0.1) {
            win->text.setColor({255,255,255,255});
            win->text.printText_Solid(font, 25.0f, 25.0f, "Loading ...");
        }
}

MainWindow *main_w = nullptr;

void eventProc() {
    main_w->proc();
}

int main(int argc, char **argv) {
    mx::system_out << "MX2 Asteroids " << ASTEROIDS_VERSION << "\n";
    mx::system_out << "MX2 Engine: v" << PROJECT_VERSION_MAJOR << "." << PROJECT_VERSION_MINOR << "\n";
    mx::system_out << "https://lostsidedead.biz\n";
#ifdef __EMSCRIPTEN__
    try {
        MainWindow main_window("", 1920, 1080);
        main_w = &main_window;
        emscripten_set_main_loop(eventProc, 0, 1);
    } catch(mx::Exception &e) {
        mx::system_err << "mx: Exception: " << e.text() << "\n";
        mx::system_err.flush();
        exit(EXIT_FAILURE);
    }
#else
    Arguments args = proc_args(argc, argv);
    try {
        MainWindow main_window(args.path, args.width, args.height);
        if(args.fullscreen) {
		    main_window.setFullScreen(true);
            SDL_ShowCursor(SDL_DISABLE);
        }
        main_window.loop();
  
        if(args.fullscreen) 
            SDL_ShowCursor(SDL_ENABLE);
    } catch(const mx::Exception &e) {
        mx::system_err << "mx: Exception: " << e.text() << "\n";
        mx::system_err.flush();
        exit(EXIT_FAILURE);
    }
#endif
    return 0;
}