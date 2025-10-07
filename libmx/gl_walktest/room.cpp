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

class Objects;

class Pillar : public gl::GLObject {
public:
    struct PillarInstance {
        glm::vec3 position;
        float radius;
        float height;
    };

    std::vector<PillarInstance> pillars;
    GLuint vao = 0, vbo = 0, ebo = 0;
    GLuint textureId = 0;
    gl::ShaderProgram pillarShader;

    Pillar() = default;
    ~Pillar() {
        if (vao != 0) glDeleteVertexArrays(1, &vao);
        if (vbo != 0) glDeleteBuffers(1, &vbo);
        if (ebo != 0) glDeleteBuffers(1, &ebo);
    }

    void load(gl::GLWindow *win) override {
#ifndef __EMSCRIPTEN__
        const char *vertexShader = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos;
            layout(location = 1) in vec2 aTexCoord;
            layout(location = 2) in vec3 aNormal;
            
            out vec2 TexCoord;
            out vec3 Normal;
            out vec3 FragPos;
            
            uniform mat4 model;
            uniform mat4 view;
            uniform mat4 projection;
            
            void main() {
                gl_Position = projection * view * model * vec4(aPos, 1.0);
                FragPos = vec3(model * vec4(aPos, 1.0));
                Normal = mat3(transpose(inverse(model))) * aNormal;
                TexCoord = aTexCoord;
            }
        )";

        const char *fragmentShader = R"(
            #version 330 core
            out vec4 FragColor;
            
            in vec2 TexCoord;
            in vec3 Normal;
            in vec3 FragPos;
            
            uniform sampler2D pillarTexture;
            uniform vec3 lightPos;
            uniform vec3 viewPos;
            
            void main() {
                float ambientStrength = 0.3;
                vec3 ambient = ambientStrength * vec3(1.0);
                
                vec3 norm = normalize(Normal);
                vec3 lightDir = normalize(lightPos - FragPos);
                float diff = max(dot(norm, lightDir), 0.0);
                vec3 diffuse = diff * vec3(1.0);
                
                vec4 texColor = texture(pillarTexture, TexCoord);
                vec3 result = (ambient + diffuse) * vec3(texColor);
                FragColor = vec4(result, texColor.a);
            }
        )";
#else
        const char *vertexShader = R"(#version 300 es
            precision highp float;
            layout(location = 0) in vec3 aPos;
            layout(location = 1) in vec2 aTexCoord;
            layout(location = 2) in vec3 aNormal;
            
            out vec2 TexCoord;
            out vec3 Normal;
            out vec3 FragPos;
            
            uniform mat4 model;
            uniform mat4 view;
            uniform mat4 projection;
            
            void main() {
                gl_Position = projection * view * model * vec4(aPos, 1.0);
                FragPos = vec3(model * vec4(aPos, 1.0));
                Normal = mat3(transpose(inverse(model))) * aNormal;
                TexCoord = aTexCoord;
            }
        )";

        const char *fragmentShader = R"(#version 300 es
            precision highp float;
            out vec4 FragColor;
            
            in vec2 TexCoord;
            in vec3 Normal;
            in vec3 FragPos;
            
            uniform sampler2D pillarTexture;
            uniform vec3 lightPos;
            uniform vec3 viewPos;
            
            void main() {
                float ambientStrength = 0.3;
                vec3 ambient = ambientStrength * vec3(1.0);
                
                vec3 norm = normalize(Normal);
                vec3 lightDir = normalize(lightPos - FragPos);
                float diff = max(dot(norm, lightDir), 0.0);
                vec3 diffuse = diff * vec3(1.0);
                
                vec4 texColor = texture(pillarTexture, TexCoord);
                vec3 result = (ambient + diffuse) * vec3(texColor);
                FragColor = vec4(result, texColor.a);
            }
        )";
#endif

        if(!pillarShader.loadProgramFromText(vertexShader, fragmentShader)) {
            throw mx::Exception("Failed to load pillar shader");
        }

        int segments = 16;
        std::vector<float> vertices;
        std::vector<unsigned int> indices;

        for (int i = 0; i <= segments; ++i) {
            float angle = (float)i / segments * 2.0f * M_PI;
            float x = cos(angle);
            float z = sin(angle);
            float u = (float)i / segments;

            vertices.insert(vertices.end(), {
                x, 0.0f, z,           
                u, 0.0f,              
                x, 0.0f, z            
            });

            vertices.insert(vertices.end(), {
                x, 1.0f, z,           
                u, 1.0f,              
                x, 0.0f, z            
            });
        }

        for (int i = 0; i < segments; ++i) {
            int current = i * 2;
            int next = (i + 1) * 2;

            indices.insert(indices.end(), {
                (unsigned int)current, (unsigned int)(current + 1), (unsigned int)next,
                (unsigned int)next, (unsigned int)(current + 1), (unsigned int)(next + 1)
            });
        }

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);
        
        glBindVertexArray(vao);
        
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
        glEnableVertexAttribArray(2);
        
        SDL_Surface* pillarSurface = png::LoadPNG(win->util.getFilePath("data/ground.png").c_str());
        if(!pillarSurface) {
            throw mx::Exception("Failed to load pillar texture");
        }

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pillarSurface->w, pillarSurface->h,
                    0, GL_RGBA, GL_UNSIGNED_BYTE, pillarSurface->pixels);
        
        SDL_FreeSurface(pillarSurface);

        numIndices = indices.size();

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> posX(-40.0f, 40.0f);
        std::uniform_real_distribution<float> posZ(-40.0f, 40.0f);
        std::uniform_real_distribution<float> radiusDist(0.5f, 1.5f);
        std::uniform_real_distribution<float> heightDist(3.0f, 6.0f);

        int numPillars = 15;
        for (int i = 0; i < numPillars; ++i) {
            PillarInstance pillar;
            pillar.position = glm::vec3(posX(gen), 0.0f, posZ(gen));
            pillar.radius = radiusDist(gen);
            pillar.height = heightDist(gen);
            pillars.push_back(pillar);
        }

        std::cout << "Generated " << numPillars << " pillars\n";
    }

    void draw(gl::GLWindow *win) override {
    }

    void event(gl::GLWindow *win, SDL_Event &e) override {
    }

    void draw(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos) {
        pillarShader.useProgram();
        
        GLuint viewLoc = glGetUniformLocation(pillarShader.id(), "view");
        GLuint projectionLoc = glGetUniformLocation(pillarShader.id(), "projection");
        GLuint lightPosLoc = glGetUniformLocation(pillarShader.id(), "lightPos");
        GLuint viewPosLoc = glGetUniformLocation(pillarShader.id(), "viewPos");
        
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3f(lightPosLoc, 0.0f, 15.0f, 0.0f);
        glUniform3f(viewPosLoc, cameraPos.x, cameraPos.y, cameraPos.z);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glUniform1i(glGetUniformLocation(pillarShader.id(), "pillarTexture"), 0);
        
        glBindVertexArray(vao);

        for (const auto& pillar : pillars) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, pillar.position);
            model = glm::scale(model, glm::vec3(pillar.radius, pillar.height, pillar.radius));
            
            GLuint modelLoc = glGetUniformLocation(pillarShader.id(), "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            
            glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, 0);
        }

        glBindVertexArray(0);
    }

    bool checkCollision(const glm::vec3& position, float playerRadius) {
        for (const auto& pillar : pillars) {
            glm::vec2 playerPos2D(position.x, position.z);
            glm::vec2 pillarPos2D(pillar.position.x, pillar.position.z);
            
            float distance = glm::length(playerPos2D - pillarPos2D);
            if (distance < (pillar.radius + playerRadius)) {
                return true;
            }
        }
        return false;
    }

private:
    size_t numIndices = 0;
};

class Explosion {
public:
    struct Particle {
        glm::vec3 position;
        glm::vec3 velocity;
        glm::vec4 color;
        float lifetime;
        float maxLifetime;
        float size;
        bool active;
    };

    std::vector<Particle> particles;
    GLuint vao, vbo;
    gl::ShaderProgram explosionShader;

    Explosion() = default;
    ~Explosion() {
        if (vao != 0) glDeleteVertexArrays(1, &vao);
        if (vbo != 0) glDeleteBuffers(1, &vbo);
    }

    void load(gl::GLWindow *win) {
#ifndef __EMSCRIPTEN__
        const char *vertexShader = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos;
            layout(location = 1) in vec4 aColor;
            layout(location = 2) in float aSize;
            
            out vec4 particleColor;
            
            uniform mat4 view;
            uniform mat4 projection;
            
            void main() {
                gl_Position = projection * view * vec4(aPos, 1.0);
                gl_PointSize = aSize;
                particleColor = aColor;
            }
        )";

        const char *fragmentShader = R"(
            #version 330 core
            in vec4 particleColor;
            out vec4 FragColor;
            
            void main() {
                vec2 coord = gl_PointCoord - vec2(0.5);
                float dist = length(coord);
                if (dist > 0.5) discard;
                
                float fade = 1.0 - smoothstep(0.0, 0.5, dist);
                FragColor = vec4(particleColor.rgb, particleColor.a * fade);
            }
        )";
#else
        const char *vertexShader = R"(#version 300 es
            precision highp float;
            layout(location = 0) in vec3 aPos;
            layout(location = 1) in vec4 aColor;
            layout(location = 2) in float aSize;
            
            out vec4 particleColor;
            
            uniform mat4 view;
            uniform mat4 projection;
            
            void main() {
                gl_Position = projection * view * vec4(aPos, 1.0);
                gl_PointSize = aSize;
                particleColor = aColor;
            }
        )";

        const char *fragmentShader = R"(#version 300 es
            precision highp float;
            in vec4 particleColor;
            out vec4 FragColor;
            
            void main() {
                vec2 coord = gl_PointCoord - vec2(0.5);
                float dist = length(coord);
                if (dist > 0.5) discard;
                
                float fade = 1.0 - smoothstep(0.0, 0.5, dist);
                FragColor = vec4(particleColor.rgb, particleColor.a * fade);
            }
        )";
#endif

        if(!explosionShader.loadProgramFromText(vertexShader, fragmentShader)) {
            throw mx::Exception("Failed to load explosion shader");
        }

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, 1000 * 8 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(7 * sizeof(float)));
        glEnableVertexAttribArray(2);
        
        glBindVertexArray(0);
    }

    void createExplosion(const glm::vec3& position, int numParticles = 100) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> speed(2.0f, 10.0f);
        std::uniform_real_distribution<float> angle(0.0f, 2.0f * M_PI);
        std::uniform_real_distribution<float> elevation(-M_PI/4.0f, M_PI/4.0f);
        std::uniform_real_distribution<float> colorVariation(0.8f, 1.0f);

        for (int i = 0; i < numParticles; ++i) {
            Particle particle;
            particle.position = position;
            
            float theta = angle(gen);
            float phi = elevation(gen);
            float velocity = speed(gen);
            
            particle.velocity = glm::vec3(
                velocity * cos(phi) * cos(theta),
                velocity * sin(phi),
                velocity * cos(phi) * sin(theta)
            );
            
            float r = colorVariation(gen);
            float g = colorVariation(gen) * 0.7f; 
            float b = colorVariation(gen) * 0.2f; 
            particle.color = glm::vec4(r, g, b, 1.0f);
            
            particle.lifetime = 0.0f;
            particle.maxLifetime = 1.0f + speed(gen) * 0.2f;
            particle.size = 10.0f + speed(gen) * 2.0f;
            particle.active = true;
            
            particles.push_back(particle);
        }
        
        std::cout << "Explosion created at (" << position.x << ", " << position.y << ", " << position.z 
                  << ") with " << numParticles << " particles\n";
    }

    void update(float deltaTime, Pillar& pillars) {
        for (auto& particle : particles) {
            if (!particle.active) continue;

            particle.position += particle.velocity * deltaTime;
            particle.velocity.y -= 9.8f * deltaTime;
            
            for (const auto& pillar : pillars.pillars) {
                glm::vec2 particlePos2D(particle.position.x, particle.position.z);
                glm::vec2 pillarPos2D(pillar.position.x, pillar.position.z);
                
                float distance = glm::length(particlePos2D - pillarPos2D);
                
                if (distance < pillar.radius && particle.position.y < pillar.height && particle.position.y > 0.0f) {
                    glm::vec2 normal = glm::normalize(particlePos2D - pillarPos2D);
                    glm::vec2 velocity2D(particle.velocity.x, particle.velocity.z);
                    
                    glm::vec2 reflected = velocity2D - 2.0f * glm::dot(velocity2D, normal) * normal;
                    
                    particle.velocity.x = reflected.x * 0.5f; 
                    particle.velocity.z = reflected.y * 0.5f;
                    
                    glm::vec2 correction = normal * (pillar.radius - distance + 0.1f);
                    particle.position.x += correction.x;
                    particle.position.z += correction.y;
                }
            }
            
            if (particle.position.y < 0.0f) {
                particle.position.y = 0.0f;
                particle.velocity.y = -particle.velocity.y * 0.3f; 
                particle.velocity.x *= 0.8f; 
                particle.velocity.z *= 0.8f;
            }
            
            particle.lifetime += deltaTime;

            float progress = particle.lifetime / particle.maxLifetime;
            particle.color.a = 1.0f - progress;
            particle.size *= 0.98f;

            if (particle.lifetime >= particle.maxLifetime) {
                particle.active = false;
            }
        }

        particles.erase(
            std::remove_if(particles.begin(), particles.end(),
                [](const Particle& p) { return !p.active; }),
            particles.end()
        );
    }

    void draw(const glm::mat4& view, const glm::mat4& projection) {
        if (particles.empty()) return;

#ifndef __EMSCRIPTEN__
        glEnable(GL_PROGRAM_POINT_SIZE);
#endif
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);

        explosionShader.useProgram();
        
        GLuint viewLoc = glGetUniformLocation(explosionShader.id(), "view");
        GLuint projectionLoc = glGetUniformLocation(explosionShader.id(), "projection");
        
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        std::vector<float> vertexData;
        for (const auto& particle : particles) {
            if (!particle.active) continue;

            vertexData.push_back(particle.position.x);
            vertexData.push_back(particle.position.y);
            vertexData.push_back(particle.position.z);
            vertexData.push_back(particle.color.r);
            vertexData.push_back(particle.color.g);
            vertexData.push_back(particle.color.b);
            vertexData.push_back(particle.color.a);
            vertexData.push_back(particle.size);
        }

        if (!vertexData.empty()) {
            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, vertexData.size() * sizeof(float), vertexData.data());
            glDrawArrays(GL_POINTS, 0, particles.size());
            glBindVertexArray(0);
        }

        glDepthMask(GL_TRUE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
};

class Floor {
public:
    Floor() = default;
    ~Floor() {
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
    }

    void load(gl::GLWindow *win) {
#ifndef __EMSCRIPTEN__
        const char *vertexShader = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos;
            layout(location = 1) in vec2 aTexCoord;
            
            out vec2 TexCoord;
            
            uniform mat4 model;
            uniform mat4 view;
            uniform mat4 projection;
            
            void main() {
                gl_Position = projection * view * model * vec4(aPos, 1.0);
                TexCoord = aTexCoord;
            }
        )";

        const char *fragmentShader = R"(
            #version 330 core
            out vec4 FragColor;
            
            in vec2 TexCoord;
            
            uniform sampler2D floorTexture;
            
            void main() {
                FragColor = texture(floorTexture, TexCoord);
            }
        )";
#else
const char *vertexShader = R"(#version 300 es
    precision highp float;
    layout(location = 0) in vec3 aPos;
    layout(location = 1) in vec2 aTexCoord;
    out vec2 TexCoord;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    void main() {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        TexCoord = aTexCoord;
    }
)";

const char *fragmentShader = R"(#version 300 es
    precision highp float;
    out vec4 FragColor;
    in vec2 TexCoord;
    uniform sampler2D floorTexture;
    void main() {
        FragColor = texture(floorTexture, TexCoord);
    }
)";
#endif
        if(floorShader.loadProgramFromText(vertexShader, fragmentShader) == false) {
            throw mx::Exception("Failed to load floor shader program");
        }

        float vertices[] = {
            -50.0f, 0.0f, -50.0f,  0.0f,  0.0f,
             50.0f, 0.0f, -50.0f, 25.0f,  0.0f,
             50.0f, 0.0f,  50.0f, 25.0f, 25.0f,
            -50.0f, 0.0f,  50.0f,  0.0f, 25.0f
        };
        
        unsigned int indices[] = {
            0, 1, 2,
            2, 3, 0
        };

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);
        
        glBindVertexArray(vao);
        
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
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
    }

    void update(float deltaTime) {
    }

    void draw(gl::GLWindow *win) {
        glm::mat4 model = glm::mat4(1.0f);
        
        glm::mat4 view = glm::lookAt(
            glm::vec3(cameraPos.x, cameraPos.y, cameraPos.z),
            glm::vec3(cameraPos.x + cameraFront.x, cameraPos.y + cameraFront.y, cameraPos.z + cameraFront.z),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 
                                            static_cast<float>(win->w) / static_cast<float>(win->h),
                                            0.1f, 100.0f);
        
        floorShader.useProgram();
        
        GLuint modelLoc = glGetUniformLocation(floorShader.id(), "model");
        GLuint viewLoc = glGetUniformLocation(floorShader.id(), "view");
        GLuint projectionLoc = glGetUniformLocation(floorShader.id(), "projection");
        
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glUniform1i(glGetUniformLocation(floorShader.id(), "floorTexture"), 0);
        
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    void setCameraPosition(const glm::vec3& pos) { cameraPos = pos; }
    void setCameraFront(const glm::vec3& front) { cameraFront = front; }
    glm::vec3 getCameraPosition() const { return cameraPos; }
    glm::vec3 getCameraFront() const { return cameraFront; }

private:
    gl::ShaderProgram floorShader;
    GLuint vao = 0, vbo = 0, ebo = 0;
    GLuint textureId = 0;
    glm::vec3 cameraPos = glm::vec3(0.0f, 1.7f, 3.0f); 
    glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f); 
};

class Objects {
public:
    static const int MAX_OBJECTS = 10;

    Objects() = default;
    ~Objects() {
        
    }

    mx::Model saturn;
    mx::Model bird;

    struct Object {
        mx::Model *model;
        glm::vec3 position;
        glm::vec3 rotation;
        glm::vec3 scale;
        float rotationSpeed;
        bool active;  
        float radius; 
    };

    std::vector<Object> objects;
    gl::ShaderProgram obj_shader;

    void load(gl::GLWindow *win) {
        
#ifndef __EMSCRIPTEN__
        const char *vertexShader = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos;
            layout(location = 1) in vec3 aNormal;
            layout(location = 2) in vec2 aTexCoord;
            
            out vec2 TexCoord;
            out vec3 Normal;
            out vec3 FragPos;
            
            uniform mat4 model;
            uniform mat4 view;
            uniform mat4 projection;
            
            void main() {
                gl_Position = projection * view * model * vec4(aPos, 1.0);
                FragPos = vec3(model * vec4(aPos, 1.0));
                Normal = mat3(transpose(inverse(model))) * aNormal;
                TexCoord = aTexCoord;
            }
        )";

        const char *fragmentShader = R"(
            #version 330 core
            out vec4 FragColor;
            
            in vec2 TexCoord;
            in vec3 Normal;
            in vec3 FragPos;
            
            uniform sampler2D objectTexture;
            uniform vec3 lightPos;
            uniform vec3 viewPos;
            uniform vec3 lightColor;
            
            void main() {
                float ambientStrength = 0.3;
                vec3 ambient = ambientStrength * lightColor;
                
                vec3 norm = normalize(Normal);
                vec3 lightDir = normalize(lightPos - FragPos);
                float diff = max(dot(norm, lightDir), 0.0);
                vec3 diffuse = diff * lightColor;
                
                float specularStrength = 0.5;
                vec3 viewDir = normalize(viewPos - FragPos);
                vec3 reflectDir = reflect(-lightDir, norm);
                float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
                vec3 specular = specularStrength * spec * lightColor;
                
                vec4 texColor = texture(objectTexture, TexCoord);
                vec3 result = (ambient + diffuse + specular) * vec3(texColor);
                FragColor = vec4(result, texColor.a);
            }
        )";
#else
        const char *vertexShader = R"(#version 300 es
            precision highp float;
            layout(location = 0) in vec3 aPos;
            layout(location = 1) in vec3 aNormal;
            layout(location = 2) in vec2 aTexCoord;
            
            out vec2 TexCoord;
            out vec3 Normal;
            out vec3 FragPos;
            
            uniform mat4 model;
            uniform mat4 view;
            uniform mat4 projection;
            
            void main() {
                gl_Position = projection * view * model * vec4(aPos, 1.0);
                FragPos = vec3(model * vec4(aPos, 1.0));
                Normal = mat3(transpose(inverse(model))) * aNormal;
                TexCoord = aTexCoord;
            }
        )";

        const char *fragmentShader = R"(#version 300 es
            precision highp float;
            out vec4 FragColor;
            
            in vec2 TexCoord;
            in vec3 Normal;
            in vec3 FragPos;
            
            uniform sampler2D objectTexture;
            uniform vec3 lightPos;
            uniform vec3 viewPos;
            uniform vec3 lightColor;
            
            void main() {
                float ambientStrength = 0.3;
                vec3 ambient = ambientStrength * lightColor;    
        
                vec3 norm = normalize(Normal);
                vec3 lightDir = normalize(lightPos - FragPos);
                float diff = max(dot(norm, lightDir), 0.0);
                vec3 diffuse = diff * lightColor;
            
                float specularStrength = 0.5;
                vec3 viewDir = normalize(viewPos - FragPos);
                vec3 reflectDir = reflect(-lightDir, norm);
                float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
                vec3 specular = specularStrength * spec * lightColor;
                
                vec4 texColor = texture(objectTexture, TexCoord);
                vec3 result = (ambient + diffuse + specular) * vec3(texColor);
                FragColor = vec4(result, texColor.a);
            }
        )";
#endif

        if(obj_shader.loadProgramFromText(vertexShader, fragmentShader) == false) {
            throw mx::Exception("Failed to load object shader program");
        }

        objects.resize(MAX_OBJECTS);
        std::vector<int> modelTypes(MAX_OBJECTS);
        for(int i = 0; i < MAX_OBJECTS / 2; i++) {
            modelTypes[i] = 0; 
        }
        for(int i = MAX_OBJECTS / 2; i < MAX_OBJECTS; i++) {
            modelTypes[i] = 1; 
        }
        
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(modelTypes.begin(), modelTypes.end(), g);
        
        if(!saturn.openModel(win->util.getFilePath("data/saturn.mxmod.z"))) {
            throw mx::Exception("Failed to load saturn model");
        }
        
        saturn.setTextures(win, win->util.getFilePath("data/planet.tex"), 
                                             win->util.getFilePath("data"));

        if(!bird.openModel(win->util.getFilePath("data/bird.mxmod.z"))) {
            throw mx::Exception("Failed to load bird model");
        }
        bird.setTextures(win, win->util.getFilePath("data/bird.tex"), 
                                        win->util.getFilePath("data"));

        for(int i = 0; i < MAX_OBJECTS; ++i) {
            objects[i].active = true; 
            
            if(modelTypes[i] == 0) { 
                objects[i].model = &saturn;  
                float scale = 0.4f + ((rand() % 1000) / 1000.0f) * 0.4f;
                objects[i].scale = glm::vec3(scale, scale, scale);
                objects[i].rotationSpeed = 5.0f + ((rand() % 1000) / 1000.0f) * 10.0f;
                objects[i].radius = 2.0f * scale; 
            } 
            else { 
                objects[i].model = &bird;    
                float scale = 0.3f + ((rand() % 1000) / 1000.0f) * 0.2f;
                objects[i].scale = glm::vec3(scale, scale, scale);
                objects[i].rotationSpeed = 20.0f + ((rand() % 1000) / 1000.0f) * 40.0f;
                objects[i].radius = 1.5f * scale; 
            }
            
            float x = ((i % 3) - 1) * 20.0f;  
            float z = ((i / 3) - 1) * 20.0f;  
            
            x += ((rand() % 1000) / 1000.0f) * 10.0f - 5.0f;
            z += ((rand() % 1000) / 1000.0f) * 10.0f - 5.0f;
            
            float y = (modelTypes[i] == 1) ? 1.5f + (rand() % 100) / 100.0f : 2.5f;
            objects[i].position = glm::vec3(x, y, z);
            
            objects[i].rotation = glm::vec3(0.0f, static_cast<float>(rand() % 360), 0.0f);
        }
    }

    bool checkCollision(const glm::vec3& point, float& hitObjectIndex) {
        for(size_t i = 0; i < objects.size(); ++i) {
            if (!objects[i].active) continue;
            
            float distance = glm::length(objects[i].position - point);
            if (distance < objects[i].radius) {
                hitObjectIndex = i;
                return true;
            }
        }
        return false;
    }

    void removeObject(int index) {
        if (index >= 0 && index < static_cast<int>(objects.size())) {
            objects[index].active = false;
            std::cout << "Object " << index << " destroyed!\n";
        }
    }

    int getActiveCount() const {
        int count = 0;
        for (const auto& obj : objects) {
            if (obj.active) count++;
        }
        return count;
    }

    void update(float deltaTime) {
        for(auto &obj : objects) {
            if (!obj.active) continue; 
            
            obj.rotation.y += obj.rotationSpeed * deltaTime;
            if(obj.rotation.y > 360.0f) {
                obj.rotation.y -= 360.0f;
            }
        }
    }

    void draw(gl::GLWindow *win, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos) {
        obj_shader.useProgram();
        
        GLuint viewLoc = glGetUniformLocation(obj_shader.id(), "view");
        GLuint projectionLoc = glGetUniformLocation(obj_shader.id(), "projection");
        GLuint lightPosLoc = glGetUniformLocation(obj_shader.id(), "lightPos");
        GLuint viewPosLoc = glGetUniformLocation(obj_shader.id(), "viewPos");
        GLuint lightColorLoc = glGetUniformLocation(obj_shader.id(), "lightColor");
        
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3f(lightPosLoc, 0.0f, 15.0f, 0.0f);
        glUniform3f(viewPosLoc, cameraPos.x, cameraPos.y, cameraPos.z);
        glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);
        
        for(auto &obj : objects) {
            if (!obj.active) continue; 
            
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, obj.position);
            model = glm::rotate(model, glm::radians(obj.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(obj.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(obj.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, obj.scale);
            
            GLuint modelLoc = glGetUniformLocation(obj_shader.id(), "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            
            obj.model->setShaderProgram(&obj_shader, "objectTexture");
            obj.model->drawArrays();
        }
    }
};


class Projectile {
public:
    struct TrailPoint {
        glm::vec3 position;
        float lifetime;
        float maxLifetime;
    };

    struct Bullet {
        glm::vec3 position;
        glm::vec3 direction;
        float speed;
        float lifetime;
        float maxLifetime;
        bool active;
        std::vector<TrailPoint> trail;
        float trailTimer;
    };

    std::vector<Bullet> bullets;
    GLuint vao, vbo, ebo;
    GLuint trailVao, trailVbo;
    gl::ShaderProgram bulletShader;
    gl::ShaderProgram trailShader;

    Projectile() = default;
    ~Projectile() {
        if (vao != 0) glDeleteVertexArrays(1, &vao);
        if (vbo != 0) glDeleteBuffers(1, &vbo);
        if (ebo != 0) glDeleteBuffers(1, &ebo);
        if (trailVao != 0) glDeleteVertexArrays(1, &trailVao);
        if (trailVbo != 0) glDeleteBuffers(1, &trailVbo);
    }

    void load(gl::GLWindow *win) {
#ifndef __EMSCRIPTEN__
    const char *vertexShader = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        
        void main() {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";

    const char *fragmentShader = R"(
        #version 330 core
        out vec4 FragColor;
        
        uniform float alpha;
        
        void main() {
            FragColor = vec4(1.0, 0.1, 0.0, alpha);
        }
    )";

    const char *trailVertexShader = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in float aAlpha;
        
        out float trailAlpha;
        
        uniform mat4 view;
        uniform mat4 projection;
        
        void main() {
            gl_Position = projection * view * vec4(aPos, 1.0);
            gl_PointSize = 12.0;
            trailAlpha = aAlpha;
        }
    )";

    const char *trailFragmentShader = R"(
        #version 330 core
        in float trailAlpha;
        out vec4 FragColor;
        
        void main() {
            vec2 coord = gl_PointCoord - vec2(0.5);
            float dist = length(coord);
            if (dist > 0.5) discard;
            
            float fade = 1.0 - smoothstep(0.0, 0.5, dist);
            vec3 color = vec3(1.0, 0.2, 0.0);
            FragColor = vec4(color, trailAlpha * fade);
        }
    )";
#else
    const char *vertexShader = R"(#version 300 es
        precision highp float;
        layout(location = 0) in vec3 aPos;
        
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        
        void main() {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";

    const char *fragmentShader = R"(#version 300 es
        precision highp float;
        out vec4 FragColor;
        
        uniform float alpha;
        
        void main() {
            FragColor = vec4(1.0, 0.1, 0.0, alpha);
        }
    )";

    const char *trailVertexShader = R"(#version 300 es
        precision highp float;
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in float aAlpha;
        
        out float trailAlpha;
        
        uniform mat4 view;
        uniform mat4 projection;
        
        void main() {
            gl_Position = projection * view * vec4(aPos, 1.0);
            gl_PointSize = 12.0;
            trailAlpha = aAlpha;
        }
    )";

    const char *trailFragmentShader = R"(#version 300 es
        precision highp float;
        in float trailAlpha;
        out vec4 FragColor;
        
        void main() {
            vec2 coord = gl_PointCoord - vec2(0.5);
            float dist = length(coord);
            if (dist > 0.5) discard;
            
            float fade = 1.0 - smoothstep(0.0, 0.5, dist);
            vec3 color = vec3(1.0, 0.2, 0.0);
            FragColor = vec4(color, trailAlpha * fade);
        }
    )";
#endif

        if(!bulletShader.loadProgramFromText(vertexShader, fragmentShader)) {
            throw mx::Exception("Failed to load bullet shader");
        }

        if(!trailShader.loadProgramFromText(trailVertexShader, trailFragmentShader)) {
            throw mx::Exception("Failed to load trail shader");
        }

    float length = 0.5f;
    float width = 0.05f;
    
    float vertices[] = {
        -width, -width, 0.0f,
         width, -width, 0.0f,
         width,  width, 0.0f,
        -width,  width, 0.0f,
        -width, -width, -length,
         width, -width, -length,
         width,  width, -length,
        -width,  width, -length
    };
    
    unsigned int indices[] = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
        3, 2, 6, 6, 7, 3,
        0, 1, 5, 5, 4, 0,
        1, 2, 6, 6, 5, 1,
        0, 3, 7, 7, 4, 0
    };

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    
    glBindVertexArray(vao);
    
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);

    glGenVertexArrays(1, &trailVao);
    glGenBuffers(1, &trailVbo);
    
    glBindVertexArray(trailVao);
    glBindBuffer(GL_ARRAY_BUFFER, trailVbo);
    glBufferData(GL_ARRAY_BUFFER, 1000 * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
    
    std::cout << "Laser projectile system initialized\n";
}

void fire(const glm::vec3& position, const glm::vec3& direction) {
    Bullet bullet;
    bullet.position = position;
    bullet.direction = glm::normalize(direction);
    bullet.speed = 100.0f;  
    bullet.lifetime = 0.0f;
    bullet.maxLifetime = 10.0f;
    bullet.active = true;
    bullet.trailTimer = 0.0f;
    bullets.push_back(bullet);
    std::cout << "Laser bolt fired from (" << position.x << ", " << position.y << ", " << position.z 
              << ") in direction (" << direction.x << ", " << direction.y << ", " << direction.z << ")\n";
}

void update(float deltaTime, Objects& objects, Explosion& explosion, Pillar& pillars) {
    for (auto& bullet : bullets) {
        if (!bullet.active) continue;

        glm::vec3 oldPosition = bullet.position;
        bullet.position += bullet.direction * bullet.speed * deltaTime;
        bullet.lifetime += deltaTime;
        bullet.trailTimer += deltaTime;

        if (bullet.trailTimer >= 0.02f) {
            TrailPoint trailPoint;
            trailPoint.position = bullet.position;
            trailPoint.lifetime = 0.0f;
            trailPoint.maxLifetime = 0.5f;
            bullet.trail.push_back(trailPoint);
            bullet.trailTimer = 0.0f;
        }

        for (auto& point : bullet.trail) {
            point.lifetime += deltaTime;
        }

        bullet.trail.erase(
            std::remove_if(bullet.trail.begin(), bullet.trail.end(),
                [](const TrailPoint& p) { return p.lifetime >= p.maxLifetime; }),
            bullet.trail.end()
        );

        for (const auto& pillar : pillars.pillars) {
            int steps = 5;
            bool hitPillar = false;
            for (int i = 0; i <= steps; ++i) {
                float t = static_cast<float>(i) / steps;
                glm::vec3 checkPos = oldPosition + (bullet.position - oldPosition) * t;
                
                glm::vec2 bulletPos2D(checkPos.x, checkPos.z);
                glm::vec2 pillarPos2D(pillar.position.x, pillar.position.z);
                
                float distance = glm::length(bulletPos2D - pillarPos2D);
                
                if (distance < pillar.radius && checkPos.y > 0.0f && checkPos.y < pillar.height) {
                    explosion.createExplosion(checkPos, 500);
                    bullet.active = false;
                    std::cout << "Bullet hit pillar at (" << checkPos.x << ", " << checkPos.y << ", " << checkPos.z << ")\n";
                    hitPillar = true;
                    break;
                }
            }
            
            if (hitPillar) break;
        }

        if (!bullet.active) continue;

        int steps = 5;
        for (int i = 0; i <= steps; ++i) {
            float t = static_cast<float>(i) / steps;
            glm::vec3 checkPos = oldPosition + (bullet.position - oldPosition) * t;
            
            float hitIndex = -1;
            if (objects.checkCollision(checkPos, hitIndex)) {
                explosion.createExplosion(checkPos, 1000);
                objects.removeObject(static_cast<int>(hitIndex));
                bullet.active = false;
                std::cout << "Bullet hit object " << static_cast<int>(hitIndex) << "!\n";
                break;
            }
        }

        if (!bullet.active) continue;

        if (bullet.lifetime >= bullet.maxLifetime) {
            bullet.active = false;
            std::cout << "Bullet dissolved after " << bullet.lifetime << " seconds\n";
        }
    }

    bullets.erase(
        std::remove_if(bullets.begin(), bullets.end(),
            [](const Bullet& b) { return !b.active; }),
        bullets.end()
    );
}

    void draw(gl::GLWindow *win, const glm::mat4& view, const glm::mat4& projection) {
        if (bullets.empty()) return;

#ifndef __EMSCRIPTEN__
        glEnable(GL_PROGRAM_POINT_SIZE);
#endif
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        trailShader.useProgram();
        
        GLuint viewLoc = glGetUniformLocation(trailShader.id(), "view");
        GLuint projectionLoc = glGetUniformLocation(trailShader.id(), "projection");
        
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        for (const auto& bullet : bullets) {
            if (!bullet.active || bullet.trail.empty()) continue;

            std::vector<float> trailData;
            for (const auto& point : bullet.trail) {
                float fadeProgress = point.lifetime / point.maxLifetime;
                float alpha = (1.0f - fadeProgress) * 0.6f;

                trailData.push_back(point.position.x);
                trailData.push_back(point.position.y);
                trailData.push_back(point.position.z);
                trailData.push_back(alpha);
            }

            if (!trailData.empty()) {
                glBindVertexArray(trailVao);
                glBindBuffer(GL_ARRAY_BUFFER, trailVbo);
                glBufferSubData(GL_ARRAY_BUFFER, 0, trailData.size() * sizeof(float), trailData.data());
                glDrawArrays(GL_POINTS, 0, bullet.trail.size());
            }
        }

        bulletShader.useProgram();
        
        viewLoc = glGetUniformLocation(bulletShader.id(), "view");
        projectionLoc = glGetUniformLocation(bulletShader.id(), "projection");
        
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(vao);

        for (const auto& bullet : bullets) {
            if (!bullet.active) continue;

            float fadeProgress = bullet.lifetime / bullet.maxLifetime;
            float alpha = 1.0f - fadeProgress;

            glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
            glm::vec3 right = glm::normalize(glm::cross(up, bullet.direction));
            if (glm::length(right) < 0.001f) {
                right = glm::vec3(1.0f, 0.0f, 0.0f);
            }
            up = glm::normalize(glm::cross(bullet.direction, right));
            
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, bullet.position);
            
            glm::mat4 rotation = glm::mat4(1.0f);
            rotation[0] = glm::vec4(right, 0.0f);
            rotation[1] = glm::vec4(up, 0.0f);
            rotation[2] = glm::vec4(bullet.direction, 0.0f);
            
            model = model * rotation;
            model = glm::scale(model, glm::vec3(1.5f, 1.5f, 2.0f)); 

            GLuint modelLoc = glGetUniformLocation(bulletShader.id(), "model");
            GLuint alphaLoc = glGetUniformLocation(bulletShader.id(), "alpha");
            
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform1f(alphaLoc, alpha);

            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        }

        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
};

class Crosshair {
public:
    GLuint vao, vbo;
    gl::ShaderProgram crosshairShader;

    Crosshair() = default;
    ~Crosshair() {
        if (vao != 0) glDeleteVertexArrays(1, &vao);
        if (vbo != 0) glDeleteBuffers(1, &vbo);
    }

    void load(gl::GLWindow *win) {
#ifndef __EMSCRIPTEN__
        const char *vertexShader = R"(
            #version 330 core
            layout(location = 0) in vec2 aPos;
            
            void main() {
                gl_Position = vec4(aPos, 0.0, 1.0);
            }
        )";

        const char *fragmentShader = R"(
            #version 330 core
            out vec4 FragColor;
            
            void main() {
                FragColor = vec4(1.0, 1.0, 1.0, 0.8);
            }
        )";
#else
        const char *vertexShader = R"(#version 300 es
            precision highp float;
            layout(location = 0) in vec2 aPos;
            
            void main() {
                gl_Position = vec4(aPos, 0.0, 1.0);
            }
        )";

        const char *fragmentShader = R"(#version 300 es
            precision highp float;
            out vec4 FragColor;
            
            void main() {
                FragColor = vec4(1.0, 1.0, 1.0, 0.8);
            }
        )";
#endif

        if(!crosshairShader.loadProgramFromText(vertexShader, fragmentShader)) {
            throw mx::Exception("Failed to load crosshair shader");
        }

        float size = 0.02f;
        float gap = 0.01f;
        float vertices[] = {
            -size - gap, 0.0f,
            -gap, 0.0f,
            gap, 0.0f,
            size + gap, 0.0f,
            0.0f, -size - gap,
            0.0f, -gap,
            0.0f, gap,
            0.0f, size + gap
        };

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glBindVertexArray(0);
    }

    void draw() {
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        crosshairShader.useProgram();
        glBindVertexArray(vao);
        glLineWidth(2.0f);
        glDrawArrays(GL_LINES, 0, 8);
        glBindVertexArray(0);
        
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
    }
};

class Wall : public gl::GLObject {
public:
    Wall() = default;
    ~Wall() {
        if (vao != 0) glDeleteVertexArrays(1, &vao);
        if (vbo != 0) glDeleteBuffers(1, &vbo);
        if (ebo != 0) glDeleteBuffers(1, &ebo);
    }

    struct WallSegment {
        glm::vec3 start;
        glm::vec3 end;
        float height;
    };

    std::vector<WallSegment> walls;
    GLuint vao = 0, vbo = 0, ebo = 0;
    GLuint textureId = 0;
    gl::ShaderProgram wallShader;

    void load(gl::GLWindow *win) override {
#ifndef __EMSCRIPTEN__
        const char *vertexShader = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos;
            layout(location = 1) in vec2 aTexCoord;
            layout(location = 2) in vec3 aNormal;
            
            out vec2 TexCoord;
            out vec3 Normal;
            out vec3 FragPos;
            
            uniform mat4 model;
            uniform mat4 view;
            uniform mat4 projection;
            
            void main() {
                gl_Position = projection * view * model * vec4(aPos, 1.0);
                FragPos = vec3(model * vec4(aPos, 1.0));
                Normal = mat3(transpose(inverse(model))) * aNormal;
                TexCoord = aTexCoord;
            }
        )";

        const char *fragmentShader = R"(
            #version 330 core
            out vec4 FragColor;
            
            in vec2 TexCoord;
            in vec3 Normal;
            in vec3 FragPos;
            
            uniform sampler2D wallTexture;
            uniform vec3 lightPos;
            uniform vec3 viewPos;
            
            void main() {
                float ambientStrength = 0.5;
                vec3 ambient = ambientStrength * vec3(1.0);
                
                vec3 norm = normalize(Normal);
                vec3 lightDir = normalize(lightPos - FragPos);
                float diff = max(dot(norm, lightDir), 0.0);
                
                float distance = length(lightPos - FragPos);
                float attenuation = 1.0 / (1.0 + 0.005 * distance + 0.0001 * distance * distance);
                
                vec3 diffuse = diff * attenuation * vec3(1.0);
                
                vec4 texColor = texture(wallTexture, TexCoord);
                vec3 result = (ambient + diffuse) * vec3(texColor);
                FragColor = vec4(result, texColor.a);
            }
        )";
#else
        const char *vertexShader = R"(#version 300 es
            precision highp float;
            layout(location = 0) in vec3 aPos;
            layout(location = 1) in vec2 aTexCoord;
            layout(location = 2) in vec3 aNormal;
            
            out vec2 TexCoord;
            out vec3 Normal;
            out vec3 FragPos;
            
            uniform mat4 model;
            uniform mat4 view;
            uniform mat4 projection;
            
            void main() {
                gl_Position = projection * view * model * vec4(aPos, 1.0);
                FragPos = vec3(model * vec4(aPos, 1.0));
                Normal = mat3(transpose(inverse(model))) * aNormal;
                TexCoord = aTexCoord;
            }
        )";

        const char *fragmentShader = R"(#version 300 es
            precision highp float;
            out vec4 FragColor;
            
            in vec2 TexCoord;
            in vec3 Normal;
            in vec3 FragPos;
            
            uniform sampler2D wallTexture;
            uniform vec3 lightPos;
            uniform vec3 viewPos;
            
            void main() {
                float ambientStrength = 0.5;
                vec3 ambient = ambientStrength * vec3(1.0);
                
                vec3 norm = normalize(Normal);
                vec3 lightDir = normalize(lightPos - FragPos);
                float diff = max(dot(norm, lightDir), 0.0);
                
                float distance = length(lightPos - FragPos);
                float attenuation = 1.0 / (1.0 + 0.005 * distance + 0.0001 * distance * distance);
                
                vec3 diffuse = diff * attenuation * vec3(1.0);
                
                vec4 texColor = texture(wallTexture, TexCoord);
                vec3 result = (ambient + diffuse) * vec3(texColor);
                FragColor = vec4(result, texColor.a);
            }
        )";
#endif

        if(!wallShader.loadProgramFromText(vertexShader, fragmentShader)) {
            throw mx::Exception("Failed to load wall shader");
        }

        float size = 50.0f;
        float wallHeight = 5.0f;
        float wallThickness = 0.2f; 
        
        walls.push_back({glm::vec3(-size, 0.0f, -size), glm::vec3(size, 0.0f, -size), wallHeight});  
        walls.push_back({glm::vec3(-size, 0.0f, size), glm::vec3(size, 0.0f, size), wallHeight});    
        walls.push_back({glm::vec3(-size, 0.0f, -size), glm::vec3(-size, 0.0f, size), wallHeight});  
        walls.push_back({glm::vec3(size, 0.0f, -size), glm::vec3(size, 0.0f, size), wallHeight});    

        std::vector<float> vertices;
        std::vector<unsigned int> indices;
        unsigned int indexOffset = 0;

        for (const auto& wall : walls) {
            glm::vec3 dir = glm::normalize(wall.end - wall.start);
            glm::vec3 normalInward = glm::vec3(dir.z, 0.0f, -dir.x);
            glm::vec3 normalOutward = -normalInward;
            
            float length = glm::length(wall.end - wall.start);
            float texRepeat = length / 5.0f;

            glm::vec3 innerStart = wall.start + normalInward * wallThickness;
            glm::vec3 innerEnd = wall.end + normalInward * wallThickness;
            
            vertices.insert(vertices.end(), {
                innerStart.x, 0.0f, innerStart.z, 0.0f, 0.0f, normalInward.x, normalInward.y, normalInward.z,
                innerEnd.x, 0.0f, innerEnd.z, texRepeat, 0.0f, normalInward.x, normalInward.y, normalInward.z,
                innerEnd.x, wall.height, innerEnd.z, texRepeat, 1.0f, normalInward.x, normalInward.y, normalInward.z,
                innerStart.x, wall.height, innerStart.z, 0.0f, 1.0f, normalInward.x, normalInward.y, normalInward.z
            });

            indices.insert(indices.end(), {
                indexOffset + 0, indexOffset + 1, indexOffset + 2,
                indexOffset + 2, indexOffset + 3, indexOffset + 0
            });
            indexOffset += 4;

            glm::vec3 outerStart = wall.start - normalInward * wallThickness;
            glm::vec3 outerEnd = wall.end - normalInward * wallThickness;
            
            vertices.insert(vertices.end(), {
                outerEnd.x, 0.0f, outerEnd.z, 0.0f, 0.0f, normalOutward.x, normalOutward.y, normalOutward.z,
                outerStart.x, 0.0f, outerStart.z, texRepeat, 0.0f, normalOutward.x, normalOutward.y, normalOutward.z,
                outerStart.x, wall.height, outerStart.z, texRepeat, 1.0f, normalOutward.x, normalOutward.y, normalOutward.z,
                outerEnd.x, wall.height, outerEnd.z, 0.0f, 1.0f, normalOutward.x, normalOutward.y, normalOutward.z
            });

            indices.insert(indices.end(), {
                indexOffset + 0, indexOffset + 1, indexOffset + 2,
                indexOffset + 2, indexOffset + 3, indexOffset + 0
            });
            indexOffset += 4;

            vertices.insert(vertices.end(), {
                innerStart.x, wall.height, innerStart.z, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                innerEnd.x, wall.height, innerEnd.z, texRepeat, 0.0f, 0.0f, 1.0f, 0.0f,
                outerEnd.x, wall.height, outerEnd.z, texRepeat, 0.2f, 0.0f, 1.0f, 0.0f,
                outerStart.x, wall.height, outerStart.z, 0.0f, 0.2f, 0.0f, 1.0f, 0.0f
            });

            indices.insert(indices.end(), {
                indexOffset + 0, indexOffset + 1, indexOffset + 2,
                indexOffset + 2, indexOffset + 3, indexOffset + 0
            });
            indexOffset += 4;
        }

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);
        
        glBindVertexArray(vao);
        
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
        glEnableVertexAttribArray(2);
        
        SDL_Surface* wallSurface = png::LoadPNG(win->util.getFilePath("data/ground.png").c_str());
        if(!wallSurface) {
            throw mx::Exception("Failed to load wall texture");
        }

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, wallSurface->w, wallSurface->h,
                    0, GL_RGBA, GL_UNSIGNED_BYTE, wallSurface->pixels);
        
        SDL_FreeSurface(wallSurface);

        numIndices = indices.size();
    }

    void draw(gl::GLWindow *win) override {
    }

    void event(gl::GLWindow *win, SDL_Event &e) override {
    }

    void draw(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos) {
        wallShader.useProgram();
        
        glm::mat4 model = glm::mat4(1.0f);
        
        GLuint modelLoc = glGetUniformLocation(wallShader.id(), "model");
        GLuint viewLoc = glGetUniformLocation(wallShader.id(), "view");
        GLuint projectionLoc = glGetUniformLocation(wallShader.id(), "projection");
        GLuint lightPosLoc = glGetUniformLocation(wallShader.id(), "lightPos");
        GLuint viewPosLoc = glGetUniformLocation(wallShader.id(), "viewPos");
        
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3f(lightPosLoc, 0.0f, 15.0f, 0.0f);
        glUniform3f(viewPosLoc, cameraPos.x, cameraPos.y, cameraPos.z);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glUniform1i(glGetUniformLocation(wallShader.id(), "wallTexture"), 0);
        
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    bool checkCollision(const glm::vec3& position, float radius) {
        for (const auto& wall : walls) {
            glm::vec3 wallDir = wall.end - wall.start;
            glm::vec3 pointToStart = position - wall.start;
            
            float wallLength = glm::length(wallDir);
            wallDir = glm::normalize(wallDir);
            
            float t = glm::dot(pointToStart, wallDir);
            t = glm::clamp(t, 0.0f, wallLength);
            
            glm::vec3 closestPoint = wall.start + wallDir * t;
            closestPoint.y = position.y; 
            
            float distance = glm::length(position - closestPoint);
            
            if (distance < radius) {
                return true;
            }
        }
        return false;
    }

private:
    size_t numIndices = 0;
};


class Game : public gl::GLObject {
public:
    Game() = default;
    virtual ~Game() override {}

    void load(gl::GLWindow *win) override {
        font.loadFont(win->util.getFilePath("data/font.ttf"), 20);
        game_floor.load(win);
        game_walls.load(win);    
        game_pillars.load(win);  
        game_objects.load(win);
        projectiles.load(win);
        explosion.load(win); 
        crosshair.load(win); 
        
        SDL_SetRelativeMouseMode(SDL_TRUE);
        lastX = win->w / 2.0f;
        lastY = win->h / 2.0f;
        
        yaw = -90.0f; 
        pitch = 0.0f;
        
        updateCameraVectors();
    }
    
    void updateCameraVectors() {
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFront = glm::normalize(front);
        game_floor.setCameraFront(cameraFront);
    }

    void draw(gl::GLWindow *win) override {
        glClearColor(0.53f, 0.81f, 0.92f, 1.0f); 
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glEnable(GL_DEPTH_TEST); 
        
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; 
        lastUpdateTime = currentTime;
        
        update(deltaTime);
        game_floor.draw(win);
        
        glm::mat4 view = glm::lookAt(
            glm::vec3(game_floor.getCameraPosition().x, game_floor.getCameraPosition().y, game_floor.getCameraPosition().z),
            glm::vec3(game_floor.getCameraPosition().x + game_floor.getCameraFront().x, game_floor.getCameraPosition().y + game_floor.getCameraFront().y, game_floor.getCameraPosition().z + game_floor.getCameraFront().z),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 
                                            static_cast<float>(win->w) / static_cast<float>(win->h),
                                                                                       0.1f, 100.0f);
        
        game_walls.draw(view, projection, game_floor.getCameraPosition());    
        game_pillars.draw(view, projection, game_floor.getCameraPosition());  
        game_objects.draw(win, view, projection, game_floor.getCameraPosition());
        projectiles.draw(win, view, projection);
        explosion.draw(view, projection); 
        crosshair.draw(); 
        
        win->text.setColor({255, 255, 255, 255});
        win->text.printText_Solid(font, 25.0f, 25.0f, 
                               "3D Room - [Escape to Release Mouse] WASD to move, Mouse to look around, Left Click to shoot");
        
        if (showFPS) {
            float fps = 1.0f / deltaTime;
            win->text.printText_Solid(font,   25.0f, 65.0f, 
                                  "FPS: " + std::to_string(static_cast<int>(fps)));
            win->text.printText_Solid(font, 25.0f, 95.0f, 
                                  "Active Bullets: " + std::to_string(projectiles.bullets.size()));
        }
    }
    
    void event(gl::GLWindow *win, SDL_Event &e) override {
        if (e.type == SDL_MOUSEMOTION && mouseCapture) {
            float xoffset = e.motion.xrel * mouseSensitivity;
            float yoffset = e.motion.yrel * mouseSensitivity;
            
            yaw += xoffset;
            pitch -= yoffset;
            
            if(pitch > 89.0f) pitch = 89.0f;
            if(pitch < -89.0f) pitch = -89.0f;
            
            updateCameraVectors();
        }
        else if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.sym == SDLK_ESCAPE) {
                mouseCapture = !mouseCapture;
                if (mouseCapture) {
                    SDL_SetRelativeMouseMode(SDL_TRUE);
                    SDL_WarpMouseInWindow(win->getWindow(), lastX, lastY);
                }
                else {
                    SDL_SetRelativeMouseMode(SDL_FALSE);
                }
            }
            else if (e.key.keysym.sym == SDLK_f) {
                showFPS = !showFPS;
            }
        }
        else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT && mouseCapture) {
            glm::vec3 firePosition = game_floor.getCameraPosition();
            glm::vec3 fireDirection = cameraFront;
            projectiles.fire(firePosition, fireDirection);
        }
    }
    
    void update(float deltaTime) {
        this->deltaTime = deltaTime;
        
        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        
        glm::vec3 cameraPos = game_floor.getCameraPosition();
        glm::vec3 cameraFront = game_floor.getCameraFront();
        
        const float cameraSpeed = 0.1f;
        const float minHeight = 1.7f;
        const float playerRadius = 0.5f;
        
        glm::vec3 horizontalFront = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));
        glm::vec3 cameraRight = glm::normalize(glm::cross(horizontalFront, glm::vec3(0.0f, 1.0f, 0.0f)));
        
        isSprinting = keys[SDL_SCANCODE_LSHIFT];
        isCrouching = keys[SDL_SCANCODE_LCTRL];
        
        glm::vec3 newPos = cameraPos;
        
        if (keys[SDL_SCANCODE_W]) {
            newPos += (isSprinting ? cameraSpeed * 2.0f : cameraSpeed) * horizontalFront;
        }
        if (keys[SDL_SCANCODE_S]) {
            newPos -= cameraSpeed * horizontalFront;
        }
        if (keys[SDL_SCANCODE_A]) {
            newPos -= cameraSpeed * cameraRight;
        }
        if (keys[SDL_SCANCODE_D]) {
            newPos += cameraSpeed * cameraRight;
        }
        
        if (!game_walls.checkCollision(newPos, playerRadius) && 
            !game_pillars.checkCollision(newPos, playerRadius)) {
            cameraPos = newPos;
        }
        
        if (keys[SDL_SCANCODE_SPACE]) {
            if (cameraPos.y <= minHeight + 0.01f) {
                jumpVelocity = 0.3f;
            }
        }
        
        cameraPos.y += jumpVelocity * deltaTime * 60.0f;
        jumpVelocity -= gravity * deltaTime * 60.0f;
        
        float currentMinHeight = isCrouching ? 0.8f : minHeight;
        if (cameraPos.y < currentMinHeight) {
            cameraPos.y = currentMinHeight;
            jumpVelocity = 0.0f;
        }
        
        game_floor.setCameraPosition(cameraPos);
        game_floor.update(deltaTime);
        game_objects.update(deltaTime);
        projectiles.update(deltaTime, game_objects, explosion, game_pillars); 
        explosion.update(deltaTime, game_pillars); 
    }
    
private:
    mx::Font font;
    Uint32 lastUpdateTime = SDL_GetTicks();
    Floor game_floor;
    Wall game_walls;       
    Pillar game_pillars;   
    Objects game_objects;
    Projectile projectiles;
    Explosion explosion; 
    Crosshair crosshair;
    glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    float lastX = 0.0f, lastY = 0.0f;
    float yaw = -90.0f; 
    float pitch = 0.0f;
    bool mouseCapture = true;
    float mouseSensitivity = 0.15f;
    bool showFPS = true;
    float deltaTime =  0.0f;
    float jumpVelocity = 0.0f;  
    const float gravity = 0.015f;
    bool isCrouching = false;  
    bool isSprinting = false;
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("Room", tw, th) {
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
        main_w =&main_window;
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