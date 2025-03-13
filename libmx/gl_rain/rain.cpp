#include"mx.hpp"
#include"argz.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <GLES3/gl3.h>
#endif

#include"gl.hpp"
#include"loadpng.hpp"

#define CHECK_GL_ERROR() \
{ GLenum err = glGetError(); \
if (err != GL_NO_ERROR) \
printf("OpenGL Error: %d at %s:%d\n", err, __FILE__, __LINE__); }

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


struct WaterParticle {
    float x, y, z;
    float vx, vy, vz;
    float life;
    float size;
    float angle;
    float speed;
    float gravity;
    float friction;
    float rotation;
    float rotationSpeed;
    float color[4];
};

class WaterEmiter {
public:
    static const int MAX_PARTICLES = 10000;
    
    WaterEmiter() = default;
    ~WaterEmiter() {
        releaseResources();
    }

    void load(gl::GLWindow *win) {
        SDL_Surface* waterSurface = png::LoadPNG(win->util.getFilePath("data/water_droplet.png").c_str());
        if(!waterSurface) {
            throw mx::Exception("Failed to load water droplet texture");
        }

        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);        

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, waterSurface->w, waterSurface->h, 
                    0, GL_RGBA, GL_UNSIGNED_BYTE, waterSurface->pixels);
        
        SDL_FreeSurface(waterSurface);

#ifdef __EMSCRIPTEN__
const char *vertexShader = R"(#version 300 es
precision highp float;
layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in float size;
layout(location = 3) in float life;
out vec4 particleColor;
out float particleLife;
uniform mat4 viewProj;
uniform float time;

void main() {
    vec3 pos = position;
    gl_Position = viewProj * vec4(pos, 1.0);
    float particleScale = size * (0.9 + sin(time * 5.0 + position.y * 10.0) * 0.1);
    float distanceToCamera = gl_Position.w;
    gl_PointSize = particleScale * 60.0 / distanceToCamera;  // Was 30.0
    particleColor = color;
    particleLife = life;
}
)";
        
        const char *fragmentShader = R"(#version 300 es
precision highp float;
in vec4 particleColor;
in float particleLife;
out vec4 fragColor;
uniform sampler2D waterTexture;
uniform float time;

void main() {
    vec2 texCoord = gl_PointCoord;
    
    texCoord.y = (texCoord.y - 0.5) * 2.0 + 0.5;  // Stretch vertically
    
    vec4 texColor = texture(waterTexture, texCoord);
    if(texColor.r < 0.1 && texColor.g < 0.1 && texColor.b < 0.1) discard;
    
    float alpha = texColor.a * particleLife * 0.9;  // Was 0.8
    vec3 waterColor = vec3(0.7, 0.85, 1.0) * texColor.rgb;  // Brighter blue
    
    float highlight = pow(texCoord.y, 2.0) * 0.7;  // Was pow(texCoord.y, 3.0) * 0.5
    waterColor += highlight * vec3(1.0);
    
    float ripple = sin(time * 8.0 + gl_FragCoord.y * 0.2) * 0.05 + 0.95;
    fragColor = vec4(waterColor * ripple * particleColor.rgb, alpha);
    if(alpha < 0.01) discard;
}
)";
#else
        const char *vertexShader = R"(#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in float size;
layout(location = 3) in float life;
out vec4 particleColor;
out float particleLife;
uniform mat4 viewProj;
uniform float time;

void main() {
    vec3 pos = position;
    gl_Position = viewProj * vec4(pos, 1.0);
    float particleScale = size * (0.9 + sin(time * 5.0 + position.y * 10.0) * 0.1);
    float distanceToCamera = gl_Position.w;
    gl_PointSize = particleScale * 60.0 / distanceToCamera;  // Was 30.0
    particleColor = color;
    particleLife = life;
}
)";
        
        const char *fragmentShader = R"(#version 330 core
in vec4 particleColor;
in float particleLife;
out vec4 fragColor;
uniform sampler2D waterTexture;
uniform float time;

void main() {
    vec2 texCoord = gl_PointCoord;
    
    texCoord.y = (texCoord.y - 0.5) * 2.0 + 0.5;  // Stretch vertically
    
    vec4 texColor = texture(waterTexture, texCoord);
    if(texColor.r < 0.1 && texColor.g < 0.1 && texColor.b < 0.1) discard;
    
    float alpha = texColor.a * particleLife * 0.9;  // Was 0.8
    vec3 waterColor = vec3(0.7, 0.85, 1.0) * texColor.rgb;  // Brighter blue
    
    float highlight = pow(texCoord.y, 2.0) * 0.7;  // Was pow(texCoord.y, 3.0) * 0.5
    waterColor += highlight * vec3(1.0);
    
    float ripple = sin(time * 8.0 + gl_FragCoord.y * 0.2) * 0.05 + 0.95;
    fragColor = vec4(waterColor * ripple * particleColor.rgb, alpha);
    if(alpha < 0.01) discard;
}
)";
#endif

        if (!waterShader.loadProgramFromText(vertexShader, fragmentShader)) {
            throw mx::Exception("Failed to load water shaders");
        }

        particles.resize(MAX_PARTICLES);
        resetParticles();
        
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, particles.size() * sizeof(WaterParticle), 
                   particles.data(), GL_DYNAMIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(WaterParticle), (void*)0);
        glEnableVertexAttribArray(0);
        
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(WaterParticle), 
                            (void*)(offsetof(WaterParticle, color)));
        glEnableVertexAttribArray(1);
        
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(WaterParticle), 
                            (void*)(offsetof(WaterParticle, size)));
        glEnableVertexAttribArray(2);
        
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(WaterParticle), 
                            (void*)(offsetof(WaterParticle, life)));
        glEnableVertexAttribArray(3);
        
        glBindVertexArray(0);

        CHECK_GL_ERROR();
    }

    void resetParticles() {
        for (auto& p : particles) {
            resetParticle(p, true);
        }
    }

    void resetParticle(WaterParticle& p, bool initialSetup = false) {
        p.x = ((float)(rand() % 40) / 100.0f - 0.2f);
        p.y = -0.2f;  
        p.z = ((float)(rand() % 40) / 100.0f - 0.2f);
        
        float angle = ((float)(rand() % 100) / 100.0f) * 2.0f * M_PI;
        float spread = ((float)(rand() % 30) / 100.0f + 0.05f);
        
        p.vx = cos(angle) * spread;
        p.vy = 1.0f + ((float)(rand() % 50) / 100.0f) * 0.8f; 
        p.vz = sin(angle) * spread;
        if (initialSetup) {
            p.life = ((float)(rand() % 100) / 100.0f);
        } else {
            p.life = 1.0f;
        }
        p.size = 0.08f + ((float)(rand() % 60) / 1000.0f);
        p.gravity = 1.2f + ((float)(rand() % 40) / 100.0f);
        p.friction = 0.02f + ((float)(rand() % 2) / 100.0f);
        p.angle = ((float)(rand() % 100) / 100.0f) * 2.0f * M_PI;
        p.rotationSpeed = ((float)(rand() % 100) / 100.0f - 0.5f) * 2.0f;
        float blueVariation = ((float)(rand() % 30) / 100.0f);
        p.color[0] = 0.7f + blueVariation; 
        p.color[1] = 0.8f + blueVariation; 
        p.color[2] = 1.0f;        
        p.color[3] = 0.8f;        
    }

    void update(float deltaTime) {
        rainHitCount = 0; 
        int newRaindrops = 150;  
        for (int i = 0; i < newRaindrops; i++) {
            int idx = rand() % (particles.size() * 3 / 4); 
            if (particles[idx].life <= 0.1f) {
                resetRainParticle(particles[idx]);
            }
        }
        
        for (auto& p : particles) {
            p.vy -= p.gravity * deltaTime;
            p.vx *= (1.0f - p.friction);
            p.vy *= (1.0f - p.friction);
            p.vz *= (1.0f - p.friction);
            float oldY = p.y; 
            p.x += p.vx * deltaTime;
            p.y += p.vy * deltaTime;
            p.z += p.vz * deltaTime;
            
        
            if (p.y <= floorY && oldY > floorY && p.vy < -0.5f) { 
                rainHitCount++;
                createSplash(p.x, floorY, p.z);
                resetRainParticle(p);
            }
            
            p.life -= 0.5f * deltaTime;
            if (p.life <= 0.0f) {
                resetRainParticle(p);
            }
        }
        
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, particles.size() * sizeof(WaterParticle), 
                       particles.data());
    }

    void draw(gl::GLWindow *win) {
        float camX = sin(cameraRotation) * cameraDistance;
        float camZ = cos(cameraRotation) * cameraDistance;
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 
                                             (float)win->w / (float)win->h, 
                                             0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(
            glm::vec3(camX, 0.3f, camZ),     
            glm::vec3(0.0f, 0.0f, 0.0f),     
            glm::vec3(0.0f, 1.0f, 0.0f)      
        );
        
        glm::mat4 viewProj = projection * view;
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#ifndef __EMSCRIPTEN__
        glEnable(GL_PROGRAM_POINT_SIZE);
#endif
        glDepthMask(GL_FALSE); 
        
        waterShader.useProgram();
        
        GLint viewProjLoc = glGetUniformLocation(waterShader.id(), "viewProj");
        glUniformMatrix4fv(viewProjLoc, 1, GL_FALSE, glm::value_ptr(viewProj));
        
        GLint texLoc = glGetUniformLocation(waterShader.id(), "waterTexture");
        glUniform1i(texLoc, 0);
        
        GLint timeLoc = glGetUniformLocation(waterShader.id(), "time");
        glUniform1f(timeLoc, SDL_GetTicks() / 1000.0f);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        
        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, particles.size());
        glBindVertexArray(0);
    
#ifndef __EMSCRIPTEN__
        glDisable(GL_PROGRAM_POINT_SIZE);
#endif
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
        
        CHECK_GL_ERROR();
    }

    void releaseResources() {
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

    void setCameraDistance(float distance) {
        cameraDistance = distance;
    }

    void setCameraRotation(float angle) {
        cameraRotation = angle;
    }

private:
    std::vector<WaterParticle> particles;
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint textureID = 0;
    gl::ShaderProgram waterShader;
    float cameraDistance = 3.0f;
    float cameraRotation = 0.0f;
    std::vector<WaterParticle> splashParticles;
    const float floorY = 0.0f;  
    int rainHitCount = 0;

public:
    void resetRainParticle(WaterParticle& p, bool initialSetup = false) {
        
        p.x = ((float)(rand() % 200) / 100.0f - 1.0f) * 2.0f; 
        p.y = 3.0f + ((float)(rand() % 100) / 100.0f) * 2.0f; 
        p.z = ((float)(rand() % 200) / 100.0f - 1.0f) * 2.0f; 
        
        p.vx = ((float)(rand() % 20) / 100.0f - 0.1f) * 0.5f;
        p.vy = -3.5f - ((float)(rand() % 100) / 100.0f) * 2.0f;
        p.vz = ((float)(rand() % 20) / 100.0f - 0.1f) * 0.5f;
        
        p.size = 0.15f + ((float)(rand() % 40) / 1000.0f);  
        p.life = 1.0f;
        p.gravity = 0.5f;
        p.friction = 0.01f;
        
        p.color[0] = 0.8f;
        p.color[1] = 0.9f; 
        p.color[2] = 1.0f;
        p.color[3] = 0.9f;  
    }
    
    void createSplash(float x, float y, float z) {
        int splashCount = 5 + rand() % 5;
        for (int i = 0; i < splashCount; i++) {
            int idx = rand() % (particles.size() / 4); 
            WaterParticle& p = particles[idx];
            p.x = x;
            p.y = y + 0.02f;
            p.z = z;
        
            float angle = ((float)(rand() % 100) / 100.0f) * 2.0f * M_PI;
            float speed = 0.5f + ((float)(rand() % 100) / 100.0f) * 0.5f;
            
            p.vx = cos(angle) * speed;
            p.vy = 0.5f + ((float)(rand() % 100) / 100.0f) * 0.5f; 
            p.vz = sin(angle) * speed;
            
            p.size = 0.03f + ((float)(rand() % 30) / 1000.0f); 
            p.life = 0.5f + ((float)(rand() % 30) / 100.0f); 
            p.gravity = 3.0f; 
            
            p.color[0] = 0.8f;
            p.color[1] = 0.9f;
            p.color[2] = 1.0f;
            p.color[3] = 0.8f;
        }
    }
    
    int getRainHitCount() const {
        return rainHitCount;
    }
};

class Floor {
public:
    void load(gl::GLWindow *win) {
        
#ifndef __EMSCRIPTEN__
        const char *vertexShader = R"(#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;
out vec2 TexCoord;
uniform mat4 viewProj;
void main() {
    gl_Position = viewProj * vec4(position, 1.0);
    TexCoord = texCoord;
}
)";
        
        const char *fragmentShader = R"(#version 330 core
in vec2 TexCoord;
out vec4 fragColor;
uniform sampler2D floorTexture;
uniform float wetness;
void main() {
    vec4 color = texture(floorTexture, TexCoord);
    // Darken the floor where wet
    color.rgb = mix(color.rgb, color.rgb * 0.5 + vec3(0.0, 0.1, 0.2), wetness);
    fragColor = color;
}
)";
#else
    const char *vertexShader = R"(#version 300 es
precision highp float;
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;
out vec2 TexCoord;
uniform mat4 viewProj;
void main() {
    gl_Position = viewProj * vec4(position, 1.0);
    TexCoord = texCoord;
}
)";
        
        const char *fragmentShader = R"(#version 300 es
precision highp float;
in vec2 TexCoord;
out vec4 fragColor;
uniform sampler2D floorTexture;
uniform float wetness;
void main() {
    vec4 color = texture(floorTexture, TexCoord);
    // Darken the floor where wet
    color.rgb = mix(color.rgb, color.rgb * 0.5 + vec3(0.0, 0.1, 0.2), wetness);
    fragColor = color;
}
)";

#endif
        shader.loadProgramFromText(vertexShader, fragmentShader);
        
        const float size = 5.0f;
        float vertices[] = {
            -size, 0.0f, -size,     0.0f, 0.0f,
             size, 0.0f, -size,     1.0f, 0.0f,
             size, 0.0f,  size,     1.0f, 1.0f,
            -size, 0.0f,  size,     0.0f, 1.0f
        };
        
        unsigned int indices[] = {
            0, 1, 2,
            2, 3, 0
        };
        
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        SDL_Surface* floorSurface = png::LoadPNG(win->util.getFilePath("data/ground.png").c_str());
        if(!floorSurface) {
            throw mx::Exception("Failed to load floor texture");  
        }
        
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, floorSurface->w, floorSurface->h, 
                    0, GL_RGBA, GL_UNSIGNED_BYTE, floorSurface->pixels);
        
        SDL_FreeSurface(floorSurface);
        
        wetness = 0.0f;
    }
    
    void draw(glm::mat4 viewProj) {
        shader.useProgram();
        
        GLint viewProjLoc = glGetUniformLocation(shader.id(), "viewProj");
        glUniformMatrix4fv(viewProjLoc, 1, GL_FALSE, glm::value_ptr(viewProj));
        
        GLint texLoc = glGetUniformLocation(shader.id(), "floorTexture");
        glUniform1i(texLoc, 0);
        
        GLint wetnessLoc = glGetUniformLocation(shader.id(), "wetness");
        glUniform1f(wetnessLoc, wetness);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureId);
        
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }
    
    void increaseWetness(float amount) {
        wetness += amount;
        if (wetness > 1.0f) wetness = 1.0f;
    }
    
    void release() {
        if (vbo) glDeleteBuffers(1, &vbo);
        if (vao) glDeleteVertexArrays(1, &vao);
        if (ebo) glDeleteBuffers(1, &ebo);
        if (textureId) glDeleteTextures(1, &textureId);
    }
    
private:
    GLuint vao = 0, vbo = 0, ebo = 0;
    GLuint textureId = 0;
    gl::ShaderProgram shader;
    float wetness = 0.0f;
};

class WaterPool {
public:
    void load(gl::GLWindow *win) {
        
#ifndef __EMSCRIPTEN__
        const char *vertexShader = R"(#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;
out vec2 TexCoord;
out vec3 FragPos;
uniform mat4 viewProj;
uniform float time;
uniform float size;
void main() {
    vec3 pos = position * vec3(size, 1.0, size);
    pos.y = 0.02f;  // Set higher above floor
    pos.y += sin(time * 2.0 + position.x * 10.0) * 0.01;
    pos.y += cos(time * 1.5 + position.z * 8.0) * 0.01;
    gl_Position = viewProj * vec4(pos, 1.0);
    TexCoord = texCoord;
    FragPos = pos;
}
)";
        
        const char *fragmentShader = R"(#version 330 core
in vec2 TexCoord;
in vec3 FragPos;
out vec4 fragColor;
uniform float time;
uniform sampler2D reflectionTexture;
uniform float poolSize;
void main() {
    
    float dist = length(FragPos.xz) / poolSize;
    float edgeFade = smoothstep(0.95, 0.75, dist);  
    
    
    vec3 waterColor = vec3(0.2, 0.5, 0.8);  
    
    
    vec2 rippleCoord = TexCoord * 5.0;
    rippleCoord.x += sin(time * 0.5) * 0.1;
    rippleCoord.y += cos(time * 0.3) * 0.1;
    
    float ripple1 = sin(rippleCoord.x * 10.0 + time * 3.0) * 0.5 + 0.5;
    float ripple2 = sin(rippleCoord.y * 8.0 + time * 2.5) * 0.5 + 0.5;
    float rippleValue = mix(ripple1, ripple2, 0.5) * 0.3;  
    
    vec3 highlight = vec3(0.8, 0.9, 1.0) * rippleValue;
    vec3 finalColor = waterColor + highlight;
    
    fragColor = vec4(finalColor, 0.85 * edgeFade);  
}
)";
#else
const char *vertexShader = R"(#version 300 es
precision highp float;
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;
out vec2 TexCoord;
out vec3 FragPos;
uniform mat4 viewProj;
uniform float time;
uniform float size;
void main() {
    vec3 pos = position * vec3(size, 1.0, size);
    pos.y = 0.02f;  // Set higher above floor
    pos.y += sin(time * 2.0 + position.x * 10.0) * 0.01;
    pos.y += cos(time * 1.5 + position.z * 8.0) * 0.01;
    gl_Position = viewProj * vec4(pos, 1.0);
    TexCoord = texCoord;
    FragPos = pos;
}
)";
        
        const char *fragmentShader = R"(#version 300 es
precision highp float;
in vec2 TexCoord;
in vec3 FragPos;
out vec4 fragColor;
uniform float time;
uniform sampler2D reflectionTexture;
uniform float poolSize;
void main() {
    
    float dist = length(FragPos.xz) / poolSize;
    float edgeFade = smoothstep(0.95, 0.75, dist);  
    
    
    vec3 waterColor = vec3(0.2, 0.5, 0.8);  
    
    
    vec2 rippleCoord = TexCoord * 5.0;
    rippleCoord.x += sin(time * 0.5) * 0.1;
    rippleCoord.y += cos(time * 0.3) * 0.1;
    
    float ripple1 = sin(rippleCoord.x * 10.0 + time * 3.0) * 0.5 + 0.5;
    float ripple2 = sin(rippleCoord.y * 8.0 + time * 2.5) * 0.5 + 0.5;
    float rippleValue = mix(ripple1, ripple2, 0.5) * 0.3;  
    
    vec3 highlight = vec3(0.8, 0.9, 1.0) * rippleValue;
    vec3 finalColor = waterColor + highlight;
    
    fragColor = vec4(finalColor, 0.85 * edgeFade);  
}
)";
#endif
        shader.loadProgramFromText(vertexShader, fragmentShader);
        
        const int segments = 32;
        std::vector<float> vertices;
        std::vector<unsigned int> indices;
        
        vertices.push_back(0.0f); 
        vertices.push_back(0.01f); 
        vertices.push_back(0.0f); 
        vertices.push_back(0.5f); 
        vertices.push_back(0.5f); 
        
        for (int i = 0; i <= segments; i++) {
            float angle = ((float)i / segments) * 2.0f * M_PI;
            float x = sin(angle);
            float z = cos(angle);
            
            vertices.push_back(x); 
            vertices.push_back(0.01f); 
            vertices.push_back(z); 
            vertices.push_back(x * 0.5f + 0.5f); 
            vertices.push_back(z * 0.5f + 0.5f); 
            
            if (i < segments) {
                indices.push_back(0); 
                indices.push_back(i + 1);
                indices.push_back(i + 2);
            }
        }
        
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        int texSize = 256;
        SDL_Surface* waterSurface = SDL_CreateRGBSurface(0, texSize, texSize, 32, 0xFF, 0xFF00, 0xFF0000, 0xFF000000);
        
        for(int y = 0; y < texSize; y++) {
            for(int x = 0; x < texSize; x++) {
                Uint8 blue = 180 + (rand() % 50);
                Uint8 green = 130 + (rand() % 40);
                Uint8 red = 100 + (rand() % 30);
                Uint32 color = (blue << 16) | (green << 8) | red;
                ((Uint32*)waterSurface->pixels)[y*texSize + x] = color;
            }
        }
        
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, waterSurface->w, waterSurface->h, 
                    0, GL_RGBA, GL_UNSIGNED_BYTE, waterSurface->pixels);
        
        SDL_FreeSurface(waterSurface);
        
        poolSize = 0.2f;  
        maxPoolSize = 3.0f;  
        this->indices = indices;
    }
    
    void update(float deltaTime, int raindropsHit) {
        
        float growAmount = raindropsHit * 0.002f * deltaTime;  
        
        if (poolSize > maxPoolSize * 0.5f) {
            growAmount *= (maxPoolSize - poolSize) / (maxPoolSize * 0.5f);
        }
        
        poolSize += growAmount;
        if (poolSize > maxPoolSize)
            poolSize = maxPoolSize;
    }
    
    void draw(glm::mat4 viewProj, float time) {
        if (poolSize < 0.1f) return; 
        
        shader.useProgram();
        
        GLint viewProjLoc = glGetUniformLocation(shader.id(), "viewProj");
        glUniformMatrix4fv(viewProjLoc, 1, GL_FALSE, glm::value_ptr(viewProj));
        
        GLint timeLoc = glGetUniformLocation(shader.id(), "time");
        glUniform1f(timeLoc, time);
        
        GLint sizeLoc = glGetUniformLocation(shader.id(), "size");
        glUniform1f(sizeLoc, poolSize);
        
        GLint poolSizeLoc = glGetUniformLocation(shader.id(), "poolSize");
        glUniform1f(poolSizeLoc, poolSize);
        
        GLint texLoc = glGetUniformLocation(shader.id(), "reflectionTexture");
        glUniform1i(texLoc, 0);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureId);
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE); 
        
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        
        glDepthMask(GL_TRUE);  
        glDisable(GL_BLEND);
    }
    
    void release() {
        if (vbo) glDeleteBuffers(1, &vbo);
        if (vao) glDeleteVertexArrays(1, &vao);
        if (ebo) glDeleteBuffers(1, &ebo);
        if (textureId) glDeleteTextures(1, &textureId);
    }
    
    float getSize() const { return poolSize; }
    
private:
    GLuint vao = 0, vbo = 0, ebo = 0;
    GLuint textureId = 0;
    gl::ShaderProgram shader;
    float poolSize = 0.2f;  
    float maxPoolSize = 3.0f;  
    std::vector<unsigned int> indices;
};

class Game : public gl::GLObject {
    WaterEmiter emiter;
    Floor floor;
    WaterPool pool;
    float cameraZoom = 7.0f;       
    float zoomSpeed = 0.2f;        
    float cameraRotation = 0.0f;   
    float rotationSpeed = 0.1f;
    float rainAccumulation = 0.0f;
    
public:
    Game() = default;
    virtual ~Game() override {}

    void load(gl::GLWindow *win) override {
        font.loadFont(win->util.getFilePath("data/font.ttf"), 36);
        emiter.load(win);
        floor.load(win);
        pool.load(win);
    }

    void draw(gl::GLWindow *win) override {
        glClearColor(0.18f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f;
        lastUpdateTime = currentTime;
        
        update(deltaTime);
        emiter.setCameraDistance(cameraZoom);
        emiter.setCameraRotation(cameraRotation);
        
        float camX = sin(cameraRotation) * cameraZoom;
        float camZ = cos(cameraRotation) * cameraZoom;
        
        glm::mat4 projection = glm::perspective(glm::radians(45.0f),
                                             (float)win->w / (float)win->h,
                                             0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(
            glm::vec3(camX, 2.0f, camZ),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        
        glm::mat4 viewProj = projection * view;
        
        glEnable(GL_DEPTH_TEST);
        floor.draw(viewProj);
        pool.draw(viewProj, currentTime / 1000.0f);
        emiter.draw(win);
        win->text.setColor({255, 255, 255, 255});
        win->text.printText_Solid(font, 25.0f, 25.0f,
            "Rain Demo - Zoom: " + std::to_string(cameraZoom) +
            " Pool: " + std::to_string(int(pool.getSize() * 100)) + "%");
    }
    
    void event(gl::GLWindow *win, SDL_Event &e) override {
        if (e.type == SDL_KEYDOWN) {
            switch(e.key.keysym.sym) {
                case SDLK_UP:    
                    cameraZoom -= zoomSpeed;
                    if (cameraZoom < 0.5f) cameraZoom = 0.5f;
                    break;
                case SDLK_DOWN:  
                    cameraZoom += zoomSpeed;
                    if (cameraZoom > 10.0f) cameraZoom = 10.0f;
                    break;
                case SDLK_LEFT:  
                    cameraRotation += rotationSpeed;
                    if (cameraRotation > 2.0f * M_PI) 
                        cameraRotation -= 2.0f * M_PI;  
                    break;
                case SDLK_RIGHT: 
                    cameraRotation -= rotationSpeed;
                    if (cameraRotation < 0.0f) 
                        cameraRotation += 2.0f * M_PI;  
                    break;
                default:
                    break;
            }
        }
    }

    void update(float deltaTime) {
        
        emiter.update(deltaTime);
        
        
        int hitCount = emiter.getRainHitCount();
        
        
        pool.update(deltaTime, hitCount);
        
        
        floor.increaseWetness(hitCount * 0.0005f * deltaTime);
    }
private:
    mx::Font font;
    Uint32 lastUpdateTime = SDL_GetTicks();
    gl::GLSprite bg;
    gl::ShaderProgram bg_shader;
    
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
        MainWindow main_window("",1920, 1080);
        main_w = &main_window;
        emscripten_set_main_loop(eventProc, 0, 1);
    } catch(const mx::Exception &e) {
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
