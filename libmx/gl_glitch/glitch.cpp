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
            if (p.z > 20.0f || p.z < -20.0f || p.x > 20.0f || p.x < -20.0f || p.y > 20.0f || p.y < -20.0f) {
                
                float radius = generateRandomFloat(15.0f, 20.0f);
                float theta = generateRandomFloat(0.0f, 2.0f * M_PI);
                float phi = generateRandomFloat(0.0f, M_PI);
                
                
                p.x = radius * sin(phi) * cos(theta);
                p.y = radius * sin(phi) * sin(theta);
                p.z = radius * cos(phi);
                
                
                float speed = generateRandomFloat(0.01f, 0.05f);
                theta = generateRandomFloat(0.0f, 2.0f * M_PI);
                phi = generateRandomFloat(0.0f, M_PI);
                
                p.vx = speed * sin(phi) * cos(theta);
                p.vy = speed * sin(phi) * sin(theta);
                p.vz = speed * cos(phi);
                
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

    void setViewProjectionMatrices(const glm::mat4& view, const glm::mat4& projection) {
        this->projectionMatrix = projection;
        this->viewMatrix = view;
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

        int newActiveCount = 0;
        for (auto& p : particles) {
            if (!p.active) continue;
            
            p.x += p.vx * deltaTime;
            p.y += p.vy * deltaTime;
            p.z += p.vz * deltaTime;
            
            p.vy -= 0.5f * deltaTime;
            p.vx *= 0.99f;  
            p.vz *= 0.99f;  
            
            p.life -= deltaTime;
            
            float lifeRatio = p.life / p.maxLife;
            p.size = p.size * 0.99f;
            if (lifeRatio < 0.7f) {
                p.intensity = lifeRatio / 0.7f;
            }
            
            if (p.life <= 0.0) {
                p.active = false;
            } else {
                newActiveCount++; 
            }
        }
        activeParticles = newActiveCount; 
        if(activeParticles < 100)
            reset();
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
        
        glUniformMatrix4fv(glGetUniformLocation(shader.id(), "projection"), 1, GL_FALSE, 
                           glm::value_ptr(projectionMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shader.id(), "view"), 1, GL_FALSE, 
                           glm::value_ptr(viewMatrix));
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID); 
        glUniform1i(glGetUniformLocation(shader.id(), "particleTexture"), 0);
        
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
        for (auto& p : particles) {
            if (activeParticles >= MAX_PARTICLES) break;  
            
            float radius = generateRandomFloat(0.1f, 0.5f);
            float theta = generateRandomFloat(0.0f, 2.0f * M_PI);
            float phi = generateRandomFloat(0.0f, M_PI);
            
            p.x = radius * sin(phi) * cos(theta);
            p.y = radius * sin(phi) * sin(theta);
            p.z = radius * cos(phi);
            
            float speed = generateRandomFloat(0.4f, 1.5f);  
            p.vx = speed * sin(phi) * cos(theta);
            p.vy = speed * sin(phi) * sin(theta);
            p.vz = speed * cos(phi);
            
            
            p.size = generateRandomFloat(1.0f, 5.0f);  
            p.maxLife = generateRandomFloat(3.0f, 7.0f);
            p.life = p.maxLife;
            p.intensity = 0.8f; 
            p.active = true;
            
            activeParticles++;
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


class Game : public gl::GLObject {
    ExplodeEmiter emiter;
    StarField field;
    GLuint texture = 0;
    bool planetVisible = true; 
public:

    Game() = default;
    virtual ~Game() override {
        if(texture)
            glDeleteTextures(1, &texture);
    }

    void load(gl::GLWindow *win) override {
        font.loadFont(win->util.getFilePath("data/font.ttf"), 36);
        if(!saturn_shader.loadProgramFromText(g_vSource, g_fSource)) {
            throw mx::Exception("Failed to load shader program");
        }
        if(!saturn.openModel(win->util.getFilePath("data/saturn.mxmod.z"))) {
            throw mx::Exception("Failed to load model");
        }
        emiter.load(win);
        saturn.setTextures(win, win->util.getFilePath("data/planet.tex"), win->util.getFilePath("data"));
        saturn.setShaderProgram(&saturn_shader, "texture1");
        saturn_shader.useProgram();
        emiter.setTextureID(gl::loadTexture(win->util.getFilePath("data/ember.png")));        
        field.load(win);
    }

    void draw(gl::GLWindow *win) override {
        glEnable(GL_DEPTH_TEST);
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; 
        lastUpdateTime = currentTime;
        
        static float rotationAngle = 0.0f;
        rotationAngle += deltaTime * 30.0f; 
        static float time_f = 0.0f;
        time_f += deltaTime;
        
        float camX = cameraDistance * sin(glm::radians(cameraRotationY)) * cos(glm::radians(cameraRotationX));
        float camY = cameraDistance * sin(glm::radians(cameraRotationX));
        float camZ = cameraDistance * cos(glm::radians(cameraRotationY)) * cos(glm::radians(cameraRotationX));
        
        cameraPosition = glm::vec3(camX, camY, camZ);
        glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);
        
        modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::scale(modelMatrix, glm::vec3(0.3f, 0.3f, 0.3f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationAngle), glm::vec3(1.0f, 1.0f, 0.0f));
        
        viewMatrix = glm::lookAt(cameraPosition, cameraTarget, upVector);
        
        float aspectRatio = static_cast<float>(win->w) / static_cast<float>(win->h);
        projectionMatrix = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);
    
        field.setViewProjectionMatrices(viewMatrix, projectionMatrix);  
        emiter.setProjectionMatrix(projectionMatrix);
        emiter.setViewMatrix(viewMatrix);
            
        field.draw(win);
        field.update(deltaTime);
    
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        if (planetVisible) {
            saturn_shader.useProgram();
            glUniformMatrix4fv(glGetUniformLocation(saturn_shader.id(), "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
            glUniformMatrix4fv(glGetUniformLocation(saturn_shader.id(), "view"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
            glUniformMatrix4fv(glGetUniformLocation(saturn_shader.id(), "projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
            glUniform3f(glGetUniformLocation(saturn_shader.id(), "lightPos"), lightPos.x, lightPos.y, lightPos.z);
            glUniform3f(glGetUniformLocation(saturn_shader.id(), "viewPos"), cameraPosition.x, cameraPosition.y, cameraPosition.z);
            glUniform3f(glGetUniformLocation(saturn_shader.id(), "lightColor"), 1.0f, 1.0f, 1.0f);
            glUniform3f(glGetUniformLocation(saturn_shader.id(), "objectColor"), 1.0f, 1.0f, 1.0f); 
            saturn_shader.setUniform("time_f", time_f);
            saturn.drawArrays();
        }
        
        emiter.draw(win);
        emiter.update(deltaTime);
    }
    
    void event(gl::GLWindow *win, SDL_Event &e) override {
        switch(e.type) {
            case SDL_KEYDOWN:
                if(e.key.keysym.sym == SDLK_LEFT)
                    cameraRotationY += 5.0f;
                else if(e.key.keysym.sym == SDLK_RIGHT)
                    cameraRotationY -= 5.0f;
            
                else if(e.key.keysym.sym == SDLK_UP)
                    cameraDistance = std::max(1.0f, cameraDistance - 0.5f);
                else if(e.key.keysym.sym == SDLK_DOWN)
                    cameraDistance = std::min(20.0f, cameraDistance + 0.5f);
        
                else if(e.key.keysym.sym == SDLK_RETURN) {
                    planetVisible = true;
                    emiter.reset(); 
                }
                break;
            case SDL_KEYUP:
                if(e.key.keysym.sym == SDLK_SPACE) {
                    if (planetVisible) { 
                        explode();
                    }
                }
                break;
            case SDL_MOUSEWHEEL:
                
                if(e.wheel.y > 0) 
                    cameraDistance = std::max(1.0f, cameraDistance - 0.5f);
                else if(e.wheel.y < 0) 
                    cameraDistance = std::min(20.0f, cameraDistance + 0.5f);
                break;
        }
    }
    
    void explode() {
        planetVisible = false; 
        emiter.reset();        
        emiter.explode();      
    }
    
    void update(float deltaTime) {
        emiter.update(deltaTime);
    }
private:
    mx::Font font;
    Uint32 lastUpdateTime = SDL_GetTicks();
    mx::Model saturn;
    gl::ShaderProgram saturn_shader;
    glm::mat4 modelMatrix{1.0f};
    glm::mat4 viewMatrix{1.0f};
    glm::mat4 projectionMatrix{1.0f};
    glm::vec3 cameraPosition{0.0f, 2.0f, 5.0f};
    glm::vec3 lightPos{0.0f, 5.0f, 0.0f};
    float cameraRotationY = 0.0f;
    float cameraRotationX = 30.0f; 
    float cameraDistance = 5.0f;
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
        MainWindow main_window("/", 1920, 1080);
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
