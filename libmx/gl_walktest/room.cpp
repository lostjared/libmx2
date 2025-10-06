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
        
            if(modelTypes[i] == 0) { 
                objects[i].model = &saturn;  
                float scale = 0.4f + ((rand() % 1000) / 1000.0f) * 0.4f;
                objects[i].scale = glm::vec3(scale, scale, scale);
                objects[i].rotationSpeed = 5.0f + ((rand() % 1000) / 1000.0f) * 10.0f;
            } 
            else { 
                objects[i].model = &bird;    
                float scale = 0.3f + ((rand() % 1000) / 1000.0f) * 0.2f;
                objects[i].scale = glm::vec3(scale, scale, scale);
                objects[i].rotationSpeed = 20.0f + ((rand() % 1000) / 1000.0f) * 40.0f;
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

    void update(float deltaTime) {
        for(auto &obj : objects) {
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
    struct Bullet {
        glm::vec3 position;
        glm::vec3 direction;
        float speed;
        float lifetime;
        float maxLifetime;
        bool active;
    };

    std::vector<Bullet> bullets;
    GLuint vao, vbo;
    gl::ShaderProgram bulletShader;

    Projectile() = default;
    ~Projectile() {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
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
                gl_PointSize = 10.0;
            }
        )";

        const char *fragmentShader = R"(
            #version 330 core
            out vec4 FragColor;
            
            uniform float alpha;
            uniform vec3 bulletColor;
            
            void main() {
                vec2 coord = gl_PointCoord - vec2(0.5);
                float dist = length(coord);
                if (dist > 0.5) discard;
                
                float fade = 1.0 - (dist * 2.0);
                FragColor = vec4(bulletColor, alpha * fade);
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
                gl_PointSize = 10.0;
            }
        )";

        const char *fragmentShader = R"(#version 300 es
            precision highp float;
            out vec4 FragColor;
            
            uniform float alpha;
            uniform vec3 bulletColor;
            
            void main() {
                vec2 coord = gl_PointCoord - vec2(0.5);
                float dist = length(coord);
                if (dist > 0.5) discard;
                
                float fade = 1.0 - (dist * 2.0);
                FragColor = vec4(bulletColor, alpha * fade);
            }
        )";
#endif

        if(!bulletShader.loadProgramFromText(vertexShader, fragmentShader)) {
            throw mx::Exception("Failed to load bullet shader");
        }

        float vertex[] = { 0.0f, 0.0f, 0.0f };

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertex), vertex, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glBindVertexArray(0);
    }

    void fire(const glm::vec3& position, const glm::vec3& direction) {
        Bullet bullet;
        bullet.position = position;
        bullet.direction = glm::normalize(direction);
        bullet.speed = 50.0f;
        bullet.lifetime = 0.0f;
        bullet.maxLifetime = 3.0f; 
        bullet.active = true;
        bullets.push_back(bullet);
    }

    void update(float deltaTime) {
        for (auto& bullet : bullets) {
            if (!bullet.active) continue;

            bullet.position += bullet.direction * bullet.speed * deltaTime;
            bullet.lifetime += deltaTime;

            if (bullet.lifetime >= bullet.maxLifetime) {
                bullet.active = false;
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
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        bulletShader.useProgram();
        
        GLuint viewLoc = glGetUniformLocation(bulletShader.id(), "view");
        GLuint projectionLoc = glGetUniformLocation(bulletShader.id(), "projection");
        GLuint colorLoc = glGetUniformLocation(bulletShader.id(), "bulletColor");
        
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3f(colorLoc, 1.0f, 0.8f, 0.0f); 

        glBindVertexArray(vao);

        for (const auto& bullet : bullets) {
            if (!bullet.active) continue;

            float fadeProgress = bullet.lifetime / bullet.maxLifetime;
            float alpha = 1.0f - fadeProgress;

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, bullet.position);

            GLuint modelLoc = glGetUniformLocation(bulletShader.id(), "model");
            GLuint alphaLoc = glGetUniformLocation(bulletShader.id(), "alpha");
            
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform1f(alphaLoc, alpha);

            glDrawArrays(GL_POINTS, 0, 1);
        }

        glBindVertexArray(0);
        glDisable(GL_BLEND);
    }
};

class Game : public gl::GLObject {
public:
    Game() = default;
    virtual ~Game() override {}

    void load(gl::GLWindow *win) override {
        font.loadFont(win->util.getFilePath("data/font.ttf"), 20);
        game_floor.load(win);
        game_objects.load(win);
        projectiles.load(win); 
        
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
        
        glm::vec3 cameraFront = glm::normalize(front);
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
        
        game_objects.draw(win, view, projection, game_floor.getCameraPosition());
        projectiles.draw(win, view, projection); 
        
        win->text.setColor({255, 255, 255, 255});
        win->text.printText_Solid(font, 25.0f, 25.0f, 
                               "3D Room - [Escape to Release Mouse] WASD to move, Mouse to look around, Left Click to shoot");
        
        if (showFPS) {
            float fps = 1.0f / deltaTime;
            win->text.printText_Solid(font, 25.0f, 65.0f, 
                                  "FPS: " + std::to_string(static_cast<int>(fps)));
        }
    }
    
    void event(gl::GLWindow *win, SDL_Event &e) override {
        const float cameraSpeed = 0.1f;
        const float minHeight = 1.7f;  
        
        if (e.type == SDL_KEYDOWN) {
            glm::vec3 cameraPos = game_floor.getCameraPosition();
            glm::vec3 cameraFront = game_floor.getCameraFront();
            
            glm::vec3 horizontalFront = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));
            glm::vec3 cameraRight = glm::normalize(glm::cross(horizontalFront, glm::vec3(0.0f, 1.0f, 0.0f)));
            
            switch (e.key.keysym.sym) {
                case SDLK_w:
                    cameraPos += (isSprinting ? cameraSpeed * 2.0f : cameraSpeed) * horizontalFront;
                    break;
                case SDLK_s:
                    cameraPos -= cameraSpeed * horizontalFront;
                    break;
                case SDLK_a:
                    cameraPos -= cameraSpeed * cameraRight;
                    break;
                case SDLK_d:
                    cameraPos += cameraSpeed * cameraRight;
                    break;
                case SDLK_ESCAPE:
                    toggleMouseCapture();
                    break;
                case SDLK_f: 
                    showFPS = !showFPS;
                    break;
                case SDLK_SPACE:  
                    if (cameraPos.y <= minHeight + 0.01f) {  
                        jumpVelocity = 0.3f;  
                    }
                    break;
                case SDLK_LCTRL:
                    isCrouching = true;
                    break;
                case SDLK_LSHIFT:
                    isSprinting = true;
                    break;
            }
            
            cameraPos.y = std::max(cameraPos.y, minHeight);
            
            game_floor.setCameraPosition(cameraPos);
        }
        
        if (e.type == SDL_KEYUP) {
            if (e.key.keysym.sym == SDLK_LCTRL) {
                isCrouching = false;
            }
            if (e.key.keysym.sym == SDLK_LSHIFT) {
                isSprinting = false;
            }
        }
        
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT && mouseCapture) {
            glm::vec3 firePosition = game_floor.getCameraPosition();
            glm::vec3 fireDirection = game_floor.getCameraFront();
            projectiles.fire(firePosition, fireDirection);
        }
        
        if (e.type == SDL_MOUSEMOTION && mouseCapture) {
            float xoffset = e.motion.xrel * mouseSensitivity;
            float yoffset = -e.motion.yrel * mouseSensitivity; 
            
            yaw += xoffset;
            pitch += yoffset;
            
            if (pitch > 89.0f)
                pitch = 89.0f;
            if (pitch < -89.0f)
                pitch = -89.0f;
                
            updateCameraVectors();
        }
    }
    
    void toggleMouseCapture() {
        mouseCapture = !mouseCapture;
        SDL_SetRelativeMouseMode(mouseCapture ? SDL_TRUE : SDL_FALSE);
    }
    
    void update(float deltaTime) {
        this->deltaTime = deltaTime;
        
        glm::vec3 cameraPos = game_floor.getCameraPosition();
        
        cameraPos.y += jumpVelocity * deltaTime * 60.0f;
        jumpVelocity -= gravity * deltaTime * 60.0f;
        
        float currentMinHeight = isCrouching ? 0.8f : 1.7f;
        if (cameraPos.y < currentMinHeight) {
            cameraPos.y = currentMinHeight;
            jumpVelocity = 0.0f;  
        }
        
        game_floor.setCameraPosition(cameraPos);
        game_floor.update(deltaTime);
        game_objects.update(deltaTime);
        projectiles.update(deltaTime); 
    }
    
private:
    mx::Font font;
    Uint32 lastUpdateTime = SDL_GetTicks();
    Floor game_floor;
    Objects game_objects;
    Projectile projectiles; 
    float lastX = 0.0f, lastY = 0.0f;
    float yaw = -90.0f; 
    float pitch = 0.0f;
    bool mouseCapture = true;
    float mouseSensitivity = 0.15f;
    bool showFPS = true;
    float deltaTime = 0.0f;
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