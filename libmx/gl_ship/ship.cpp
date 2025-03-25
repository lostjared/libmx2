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
    vec3 norm = normalize(mat3(transpose(inverse(model)))* aNormal);
    vec3 lightDir = normalize(lightPos - vec3(worldPos));
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    float specularStrength = 1.0;    // Increase from 0.5 to 1.0 for more shine
    float shininess = 64.0;          // Increase this value for a tighter, shinier highlight
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
            FragColor = vec4(vertexColor, 1.0) * texture(texture1, TexCoords);
            vec4 ctx = distort(TexCoords, time_f, texture1);
            FragColor = alphaXor(ctx);
            FragColor.a = 1.0;
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
            FragColor = vec4(vertexColor, 1.0) * texture(texture1, TexCoords);
            vec4 ctx = distort(TexCoords, time_f, texture1);
            FragColor = alphaXor(ctx);
            FragColor.a = 1.0;
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

    
    glm::vec3 lastCameraPos{0.0f, 0.0f, 0.0f};
    float starFieldRadius = 30.0f; 

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
            float radius = generateRandomFloat(10.0f, 30.0f);
            float theta = generateRandomFloat(0.0f, 2.0f * M_PI);
            float phi = generateRandomFloat(0.0f, M_PI);
            p.x = radius * sin(phi) * cos(theta);
            p.y = radius * sin(phi) * sin(theta);
            p.z = radius * cos(phi);
            p.vx = generateRandomFloat(-0.01f, 0.01f);
            p.vy = generateRandomFloat(-0.01f, 0.01f);
            p.vz = generateRandomFloat(-0.01f, 0.01f);
            
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
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; // seconds
        lastUpdateTime = currentTime;

        update(deltaTime);

        CHECK_GL_ERROR();

        program.useProgram();

        glm::mat4 MVP = projectionMatrix * viewMatrix * glm::mat4(1.0f);
        program.setUniform("MVP", MVP);
        program.setUniform("spriteTexture", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindVertexArray(VAO);
        glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);
        CHECK_GL_ERROR();

        glDisable(GL_BLEND);
    }

    void update(float deltaTime) {
        if(deltaTime > 0.1f) 
            deltaTime = 0.1f;
            
        
        glm::mat4 invView = glm::inverse(viewMatrix);
        glm::vec3 cameraPos = glm::vec3(invView[3]);
        glm::vec3 cameraForward = -glm::vec3(invView[2]); 
        
        float cameraMoveDistance = glm::length(cameraPos - lastCameraPos);
        if (cameraMoveDistance > 0.5f) {
            lastCameraPos = cameraPos;
        }

        std::vector<float> positions;
        std::vector<float> sizes;
        std::vector<float> colors;
        positions.reserve(NUM_PARTICLES * 3);
        sizes.reserve(NUM_PARTICLES);
        colors.reserve(NUM_PARTICLES * 4);
          
        for (auto& p : particles) {
            
            glm::vec3 starPos(p.x, p.y, p.z);
            glm::vec3 relativePos = starPos - cameraPos;
            float dotProduct = glm::dot(relativePos, cameraForward);
            float distance = glm::length(relativePos);
            bool needsRespawn = distance > starFieldRadius || 
                               (dotProduct < -0.7f && distance > starFieldRadius * 0.5f);
                               
            if (needsRespawn) {
                float forwardDistance = generateRandomFloat(starFieldRadius * 0.5f, starFieldRadius * 0.9f);
                float lateralDistance = generateRandomFloat(0.0f, starFieldRadius * 0.7f);
                float verticalDistance = generateRandomFloat(-starFieldRadius * 0.6f, starFieldRadius * 0.6f);
                glm::vec3 cameraRight = glm::vec3(invView[0]);
                glm::vec3 cameraUp = glm::vec3(invView[1]);
                float angle = generateRandomFloat(0.0f, 2.0f * M_PI);
                
                glm::vec3 newPos = cameraPos + 
                                   cameraForward * forwardDistance +
                                   cameraRight * (lateralDistance * cos(angle)) +
                                   cameraUp * (lateralDistance * sin(angle) + verticalDistance);
                
                p.x = newPos.x;
                p.y = newPos.y;
                p.z = newPos.z;
                
                p.life = generateRandomFloat(0.6f, 1.0f);
                p.twinkle = generateRandomFloat(1.0f, 5.0f);
            }
            
            float twinkleFactor = 0.8f * (1.0f + sin(SDL_GetTicks() * 0.002f * p.twinkle));
            float brightness = 1.3f * p.life * twinkleFactor;
            float distFactor = 1.0f - (distance / starFieldRadius * 0.7f);
            distFactor = glm::clamp(distFactor, 0.3f, 1.0f);
            brightness *= distFactor;
            
            positions.push_back(p.x);
            positions.push_back(p.y);
            positions.push_back(p.z);
            
            float size = 10.0f * p.life * (0.8f + 0.2f * distFactor);
            sizes.push_back(size);
            
            float alpha = p.life * distFactor * 1.2f;
            alpha = glm::clamp(alpha, 0.0f, 1.0f);
            
            float colorVar = 0.15f * sin(SDL_GetTicks() * 0.0015f * p.twinkle + 2.0f);
            colors.push_back(brightness + colorVar);
            colors.push_back(brightness);
            colors.push_back(brightness + colorVar * 0.5f);
            colors.push_back(alpha);
        }

        
        glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, positions.size() * sizeof(float), positions.data());

        glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizes.size() * sizeof(float), sizes.data());

        glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, colors.size() * sizeof(float), colors.data());
    }

    void repositionStarsAroundCamera(const glm::vec3& cameraPos) {
        for (auto& p : particles) {
            float radius = generateRandomFloat(10.0f, starFieldRadius);
            float theta = generateRandomFloat(0.0f, 2.0f * M_PI);
            float phi = generateRandomFloat(0.0f, M_PI);
            
            p.x = cameraPos.x + radius * sin(phi) * cos(theta);
            p.y = cameraPos.y + radius * sin(phi) * sin(theta);
            p.z = cameraPos.z + radius * cos(phi);
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

    ExplodeEmiter() = default;
    ~ExplodeEmiter() {
        if (vao != 0) glDeleteVertexArrays(1, &vao);
        if (vbo != 0) glDeleteBuffers(1, &vbo);
    }

    struct Particle {
        float x, y, z;
        float vx, vy, vz;
        float intensity;
        float size;
        float life;
        float maxLife;
        bool active;
    };

    // Add position property
    glm::vec3 position{0.0f, 0.0f, 0.0f};

    void load(gl::GLWindow *win) {
#ifndef __EMSCRIPTEN__
        const char* particleVS = R"(#version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in float aSize;
    layout (location = 2) in float aIntensity;
    
    out float intensity;
    
    uniform mat4 projection;
    uniform mat4 view;
    
    void main() {
        vec4 viewPos = view * vec4(aPos, 1.0);
        gl_Position = projection * viewPos;
        
        float distance = length(viewPos.xyz);
        gl_PointSize = aSize / (distance) * 5.0; 
        
        intensity = aIntensity;
    }
)";

const char *particleFS = R"(#version 330 core

in float intensity;
out vec4 FragColor;

uniform sampler2D particleTexture;
uniform float time_f;

vec4 alphaXor(vec4 color) {
    ivec3 source;
    for (int i = 0; i < 3; ++i) {
        source[i] = int(255 * color[i]);
    }
    color[0] *= int(time_f);
    color[1] *= int(time_f);
    color[2] *= int(time_f);
    
    ivec3 int_color;
    for (int i = 0; i < 3; ++i) {
        int_color[i] = int(255 * color[i]);
        int_color[i] = int_color[i] ^ source[i];
        if (int_color[i] > 255)
            int_color[i] = int_color[i] % 255;
        color[i] = float(int_color[i]) / 255;
    }
    return color;
}

void main() {

    vec2 texCoord = gl_PointCoord;
    vec4 texColor = texture(particleTexture, texCoord);
    

    vec3 startColor = vec3(1.0, 0.6, 0.2); 
    vec3 endColor   = vec3(0.8, 0.2, 0.1);   
    vec3 glowColor  = mix(endColor, startColor, intensity);
    glowColor = mix(glowColor, texColor.rgb, 0.4);
    

    float alpha = texColor.a * intensity;
    float dist  = length(texCoord - vec2(0.5));
    alpha *= smoothstep(0.5, 0.3, dist);
    
    vec4 combinedColor = vec4(glowColor, alpha);
    
    if(alpha < 0.1)
        discard;
    if(combinedColor.r < 0.1 && combinedColor.g < 0.1 && combinedColor.b < 0.1)
        discard;
        

    vec4 finalColor = alphaXor(combinedColor);
    finalColor.a = 1.0;
    
    FragColor = finalColor;
}
)";
#else
const char* particleVS = R"(#version 300 es
precision highp float;
layout (location = 0) in vec3 aPos;
layout (location = 1) in float aSize;
layout (location = 2) in float aIntensity;

out float intensity;

uniform mat4 projection;
uniform mat4 view;

void main() {
    vec4 viewPos = view * vec4(aPos, 1.0);
    gl_Position = projection * viewPos;
    
    float distance = length(viewPos.xyz);
    gl_PointSize = aSize / (distance) * 5.0; 
    
    intensity = aIntensity;
}
)";

    const char* particleFS = R"(#version 300 es
precision highp float;
in float intensity;
out vec4 FragColor;

uniform sampler2D particleTexture;
uniform float time_f;

vec4 alphaXor(vec4 color) {
    // Convert float colors to ints more safely for ES
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

void main() {
    vec2 texCoord = gl_PointCoord;
    vec4 texColor = texture(particleTexture, texCoord);
    
    vec3 startColor = vec3(1.0, 0.6, 0.2); 
    vec3 endColor = vec3(0.8, 0.2, 0.1);   
    vec3 glowColor = mix(endColor, startColor, intensity);
    glowColor = mix(glowColor, texColor.rgb, 0.4);
    
    float alpha = texColor.a * intensity;
    float dist = length(texCoord - vec2(0.5));
    alpha *= smoothstep(0.5, 0.3, dist);
    
    vec4 combinedColor = vec4(glowColor, alpha);
    
    if(alpha < 0.1)
        discard;
    if(combinedColor.r < 0.1 && combinedColor.g < 0.1 && combinedColor.b < 0.1)
        discard;
    
    vec4 finalColor = alphaXor(combinedColor);
    finalColor.a = 1.0;
    FragColor = finalColor; 
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
        glBufferData(GL_ARRAY_BUFFER, particles.size() * 5 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(4 * sizeof(float)));
        glEnableVertexAttribArray(2);
        
        glBindVertexArray(0);
    }

    void update(float deltaTime) {
        static float time_f = 0.0f;
        time_f += deltaTime;
        
        shader.useProgram();
        shader.setUniform("time_f", time_f);
        
        activeParticles = 0;
        
        for (auto& p : particles) {
            if (!p.active) continue;
            
            
            p.x += p.vx * deltaTime;
            p.y += p.vy * deltaTime;
            p.z += p.vz * deltaTime;
            
            
            p.vx *= 0.98f;  
            p.vy *= 0.98f;
            p.vz *= 0.98f;
            
            
            p.vy -= 0.3f * deltaTime;  
            
            p.life -= deltaTime;
            float lifeRatio = p.life / p.maxLife;
            
            
            if (lifeRatio > 0.8f) {
            
                p.intensity = (1.0f - lifeRatio) * 5.0f;
            } else if (lifeRatio < 0.3f) {
            
                p.intensity = lifeRatio / 0.3f;
            } else {
            
                p.intensity = 1.0f;
            }
            
            
            if (lifeRatio > 0.7f) {
                p.size *= 1.01f;  
            } else {
                p.size *= 0.99f;  
            }
            
            if (p.life <= 0.0f || p.size < 0.1f || p.intensity < 0.05f) {
                p.active = false;
            } else {
                activeParticles++;
            }
        }
    }
        
    void draw(gl::GLWindow *win) {
        if (activeParticles == 0) return;
    
        std::vector<float> particleData;
        particleData.reserve(activeParticles * 5);
        for (const auto& p : particles) {
            if (!p.active) continue;
            particleData.push_back(p.x);
            particleData.push_back(p.y);
            particleData.push_back(p.z);
            particleData.push_back(p.size);
            particleData.push_back(p.intensity);
        }
    
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);
    
        shader.useProgram();
        shader.setUniform("projection", projectionMatrix);
        shader.setUniform("view", viewMatrix);
    
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID); 
        shader.setUniform("particleTexture", 0);
    
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, particleData.size() * sizeof(float), particleData.data());
    
    #ifndef __EMSCRIPTEN__
        glEnable(GL_PROGRAM_POINT_SIZE);
    #endif
    
        glDrawArrays(GL_POINTS, 0, activeParticles);
    
    #ifndef __EMSCRIPTEN__
        glDisable(GL_PROGRAM_POINT_SIZE);
    #endif
    
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }
    

        
    void explode() {
        
        activeParticles = 0;
        
        
        const int WAVE_COUNT = 3;
        int particlesPerWave = MAX_PARTICLES / WAVE_COUNT;
        
        for (int wave = 0; wave < WAVE_COUNT; wave++) {
            for (int i = 0; i < particlesPerWave && activeParticles < MAX_PARTICLES; i++) {
                auto& p = particles[activeParticles];
                
        
                float theta = generateRandomFloat(0.0f, 2.0f * M_PI);
                float phi = generateRandomFloat(0.0f, M_PI);
                
        
                float x = sin(phi) * cos(theta);
                float y = sin(phi) * sin(theta);
                float z = cos(phi);
                
                float offset = (wave == 0) ? 0.1f : (wave == 1) ? 0.5f : 0.9f;
                float radiusScale = 5.0f * offset;
                
                p.x = position.x + x * radiusScale; 
                p.y = position.y + y * radiusScale;
                p.z = position.z + z * radiusScale;
                
                float speed = (wave == 0) ? generateRandomFloat(30.0f, 40.0f) : 
                            (wave == 1) ? generateRandomFloat(20.0f, 30.0f) :
                                            generateRandomFloat(10.0f, 20.0f);
                
                p.vx = x * speed;
                p.vy = y * speed;
                p.vz = z * speed;
                
                p.size = (wave == 0) ? generateRandomFloat(25.0f, 35.0f) :
                        (wave == 1) ? generateRandomFloat(15.0f, 25.0f) :
                                    generateRandomFloat(10.0f, 20.0f);
                                    
                p.maxLife = (wave == 0) ? generateRandomFloat(1.0f, 2.0f) :
                        (wave == 1) ? generateRandomFloat(2.0f, 3.0f) :
                                        generateRandomFloat(3.0f, 4.0f);
                                        
                p.life = p.maxLife;
                p.intensity = 1.0f;
                p.active = true;
                
                activeParticles++;
            }
        }
    
    }
    
    
    void reset() {
        activeParticles = 0;
        for (auto& p : particles) {
            p.active = false;
        }
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
    // Ambient
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * vec3(1.0, 1.0, 1.0);
    
    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);
    
    // Specular
    float specularStrength = 0.5;
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
// GLES 3.0 versions with precision qualifiers
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
    // Ambient
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * vec3(1.0, 1.0, 1.0);
    
    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);
    
    // Specular
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
        glm::vec3 velocity;
        float life;
        float maxLife;
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
        uniform float time_f;
        
        void main() {
            vec2 texCoord = gl_PointCoord;
            vec4 texColor = texture(particleTexture, texCoord);
            if(texColor.r < 0.1 && texColor.g < 0.1 && texColor.b < 0.1) discard;
            vec3 color = vec3(1.0, 0.1, 0.2); 
            float alpha = texColor.a * intensity;
            
            float pulse = 0.8 + 0.2 * sin(time_f * 10.0);
            color *= pulse;

            
            if (alpha < 0.05) discard;
            
            FragColor = vec4(color, alpha);
        })";
#else
        R"(#version 330 core
        in float intensity;
        out vec4 FragColor;
        uniform sampler2D particleTexture;
        uniform float time_f;
        
        void main() {
            vec2 texCoord = gl_PointCoord;
            vec4 texColor = texture(particleTexture, texCoord);
            
            if(texColor.r < 0.1 && texColor.g < 0.1 && texColor.b < 0.1) discard;

            vec3 color = vec3(1.0, 0.1, 0.2); 

            float alpha = texColor.a * intensity;
            
            float pulse = 0.8 + 0.2 * sin(time_f * 10.0);
            color *= pulse;
            
            if (alpha < 0.05) discard;
            
            FragColor = vec4(color, alpha);
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
            gl_PointSize = aSize;
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
            gl_PointSize = aSize;
            intensity = aIntensity;
        })";
#endif

        if (!projectileShader.loadProgramFromText(fullProjVert, fullProjFrag)) {
            throw mx::Exception("Failed to load projectile shader program");
        }

        texture = gl::loadTexture(win->util.getFilePath("data/star.png"));

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
        projectile.maxLife = 1.5f; 
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
        
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);
        
        
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
        
        
        glBindBuffer(GL_ARRAY_BUFFER, projectileVBO[0]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, positions.size() * sizeof(float), positions.data());
        
        glBindBuffer(GL_ARRAY_BUFFER, projectileVBO[1]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizes.size() * sizeof(float), sizes.data());
        
        glBindBuffer(GL_ARRAY_BUFFER, projectileVBO[2]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, intensities.size() * sizeof(float), intensities.data());
        
        projectileShader.useProgram();
        float time_f = SDL_GetTicks() / 1000.0f;
        projectileShader.setUniform("time_f", time_f);
        
        glm::mat4 MVP = projectionMatrix * viewMatrix;
        projectileShader.setUniform("MVP", MVP);
        projectileShader.setUniform("particleTexture", 0);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);

#ifndef __EMSCRIPTEN__
        glEnable(GL_PROGRAM_POINT_SIZE);
#endif
        
        glBindVertexArray(projectileVAO);
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(projectiles.size()));
        
#ifndef __EMSCRIPTEN__
        glDisable(GL_PROGRAM_POINT_SIZE);
#endif
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    void update(float deltaTime) {
        for (auto& projectile : projectiles) {
            projectile.position += projectile.velocity * deltaTime;
            projectile.life += deltaTime;
        }
        
        projectiles.erase(
            std::remove_if(projectiles.begin(), projectiles.end(),
                [](const Projectile& p) { return p.life >= p.maxLife; }),
            projectiles.end()
        );
    }
    
    bool checkCollision(const glm::vec3& position, float radius) {
        for (auto it = projectiles.begin(); it != projectiles.end(); ) {
            if (glm::length(it->position - position) < radius) {
                it = projectiles.erase(it);
                return true;
            } else {
                ++it;
            }
        }
        return false;
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
        rotationMatrix = glm::rotate(rotationMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f)); // Yaw
        rotationMatrix = glm::rotate(rotationMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f)); // Pitch
        
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
        rotation.y += amount * turnSpeed * deltaTime;
        
        if (rotation.y > 360.0f) rotation.y -= 360.0f;
        if (rotation.y < 0.0f) rotation.y += 360.0f;
    }
    
    void pitch(float amount, float deltaTime) {
        rotation.x += amount * turnSpeed * deltaTime;
        rotation.x = glm::clamp(rotation.x, -80.0f, 80.0f);
    }
    
    void roll(float amount, float deltaTime) {
        rotation.z += amount * turnSpeed * deltaTime;
        
        if (amount == 0.0f) {
            if (fabs(rotation.z) < 1.0f) {
                rotation.z = 0.0f;
            } else if (rotation.z > 0.0f) {
                rotation.z -= 50.0f * deltaTime;
            } else {
                rotation.z += 50.0f * deltaTime;
            }
        }
        
        rotation.z = glm::clamp(rotation.z, -45.0f, 45.0f);
    }

    void update(float deltaTime) {
        glm::vec3 forward = glm::normalize(glm::vec3(
            -sin(glm::radians(rotation.y)),
            sin(glm::radians(rotation.x)),
            -cos(glm::radians(rotation.y)) * cos(glm::radians(rotation.x))
        ));
        velocity = forward * currentSpeed;
        position += velocity * deltaTime;
    
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
        projectiles.fire(projectilePos, forward, 50.0f + currentSpeed);
    }

protected:
    mx::Model model;
    gl::ShaderProgram shader;
};

class Planet {
public:
    Planet() = default;
    ~Planet() = default;

    glm::vec3 position{0.0f, 0.0f, -30.0f}; 
    float rotationSpeed = 2.0f;             
    float rotationAngle = 0.0f;
    float scale = 5.0f;                     
    float time_f = 0.0f;  
    
    bool isDestroyed = false;
    float radius = 5.0f; 

    void load(gl::GLWindow *win) {
        if(!model.openModel(win->util.getFilePath("data/saturn.mxmod.z"))) {
                throw mx::Exception("Failed to load planet model");
        }
        if(!shader.loadProgramFromText(g_vSource, g_fSource)) {
            throw mx::Exception("Failed to load planet shader program");
        }
        model.setTextures(win, win->util.getFilePath("data/planet.tex"), win->util.getFilePath("data"));
        model.setShaderProgram(&shader, "texture1");
        
        shader.useProgram();
        shader.setUniform("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
        shader.setUniform("objectColor", glm::vec3(1.0f, 1.0f, 1.0f));
    }

    void update(float deltaTime) {
        rotationAngle += rotationSpeed * deltaTime;
        if(rotationAngle > 360.0f) rotationAngle -= 360.0f;
        
        
        time_f += deltaTime;
        if(time_f > 10000.0f) time_f = 0.0f; 
    }

    void draw(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& lightPos, const glm::vec3& viewPos) {
        shader.useProgram();
        
        
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, position);
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrix = glm::scale(modelMatrix, glm::vec3(scale, scale, scale));
        
        
        shader.setUniform("model", modelMatrix);
        shader.setUniform("view", view);
        shader.setUniform("projection", projection);
        shader.setUniform("time_f", time_f); 
        
        if(isDestroyed == false)
            model.drawArrays();
    }

    void reset() {
        isDestroyed = false;
    }

private:
    mx::Model model;
    gl::ShaderProgram shader;
};

class Game : public gl::GLObject {
    ExplodeEmiter emiter;
    StarField field;
    StarFighter ship;  
    Planet planet;     
    GLuint texture = 0;
    bool spacePressed = false;  
    
public:
    Game() = default;
    virtual ~Game() override {
        if(texture)
            glDeleteTextures(1, &texture);
    }

    void load(gl::GLWindow *win) override {
        font.loadFont(win->util.getFilePath("data/font.ttf"), 22);
        emiter.load(win);
        emiter.setTextureID(gl::loadTexture(win->util.getFilePath("data/ember.png")));        
        field.load(win);
        ship.load(win);  
        planet.load(win); 
        field.repositionStarsAroundCamera(ship.cameraPosition);
    }

    void draw(gl::GLWindow *win) override {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);  
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; 
        lastUpdateTime = currentTime;
        
        if (deltaTime > 0.1f) deltaTime = 0.1f;
        
        handleInput(win, deltaTime);
        
        ship.update(deltaTime);
        
        
        planet.update(deltaTime); 
        
        float aspectRatio = static_cast<float>(win->w) / static_cast<float>(win->h);
        projectionMatrix = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);
        
        viewMatrix = ship.getViewMatrix();
        
        field.setViewProjectionMatrices(viewMatrix, projectionMatrix);  
        emiter.setProjectionMatrix(projectionMatrix);
        emiter.setViewMatrix(viewMatrix);
        
        field.draw(win);
        field.update(deltaTime);
        glEnable(GL_DEPTH_TEST);
        planet.draw(viewMatrix, projectionMatrix, lightPos, ship.cameraPosition);
        
        ship.draw(win, projectionMatrix, lightPos);
        
        
        if (!planet.isDestroyed) {
            if (ship.projectiles.checkCollision(planet.position, planet.radius)) {
                emiter.reset();
                emiter.position = planet.position;
                emiter.explode();
                planet.isDestroyed = true;
            }
        }

        emiter.update(deltaTime);
        emiter.draw(win);
        win->text.setColor({255,255,255,255});
        win->text.printText_Solid(font,25.0f,25.0f, "Ship X,Y,Z: " + std::to_string(ship.position.x) + ", " + std::to_string(ship.position.y) + ", " + std::to_string(ship.position.z));
        win->text.printText_Solid(font,25.0f,50.0f, "Velocity X,Y,Z: " + std::to_string(ship.velocity.x) + ", " + std::to_string(ship.velocity.y) + ", " + std::to_string(ship.velocity.z)); 
        win->text.printText_Solid(font,25.0f,75.0f, "FPS: " + std::to_string(1.0f / deltaTime));
        win->text.printText_Solid(font,25.0f, 100.0f, "Left, Right Arrow Keys: Yaw, Up, Down Arrow Keys: Move Forward, Backward");
        win->text.printText_Solid(font,25.0f, 125.0f, "W, S Keys: Pitch");
    }
    
    void handleInput(gl::GLWindow* win, float deltaTime) {
        
        const Uint8* state = SDL_GetKeyboardState(NULL);
        
        if (state[SDL_SCANCODE_UP]) {
            ship.moveForward(deltaTime);
        }
        if (state[SDL_SCANCODE_DOWN]) {
            ship.moveBackward(deltaTime);
        }
        
        
        if (state[SDL_SCANCODE_LEFT]) {
            ship.yaw(1.0f, deltaTime);
            ship.roll(-1.0f, deltaTime);  
        }
        else if (state[SDL_SCANCODE_RIGHT]) {
            ship.yaw(-1.0f, deltaTime);
            ship.roll(1.0f, deltaTime);   
        }
        else {
            ship.roll(0.0f, deltaTime);   
        }
        
        
        if (state[SDL_SCANCODE_W]) {
            ship.pitch(-1.0f, deltaTime); 
        }
        if (state[SDL_SCANCODE_S]) {
            ship.pitch(1.0f, deltaTime); 
        }
        
        if (state[SDL_SCANCODE_SPACE] && !spacePressed) {
            spacePressed = true;
            ship.fireProjectile();
        } 
        else if (!state[SDL_SCANCODE_SPACE]) {
            spacePressed = false;
        }
        
        
        if (state[SDL_SCANCODE_RETURN] && planet.isDestroyed) {
            planet.reset();
            emiter.reset();
        }
    }
    
    void event(gl::GLWindow *win, SDL_Event &e) override {
        switch(e.type) {
            case SDL_KEYDOWN:
                break;
        }
        switch(e.type) {
            case SDL_KEYDOWN:
                if(e.key.keysym.sym == SDLK_RETURN && planet.isDestroyed) {
                    planet.reset();
                    emiter.reset();
                }
                break;
        }
    }
    
private:
    mx::Font font;
    Uint32 lastUpdateTime = SDL_GetTicks();
    glm::mat4 viewMatrix{1.0f};
    glm::mat4 projectionMatrix{1.0f};
    glm::vec3 lightPos{10.0f, 10.0f, 10.0f};
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
        main_window.loop();
    } catch(const mx::Exception &e) {
        mx::system_err << "mx: Exception: " << e.text() << "\n";
        mx::system_err.flush();
        exit(EXIT_FAILURE);
    }
#endif
    return 0;
}
