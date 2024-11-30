#include "mx.hpp"
#include "argz.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>
#define WITH_GL
#endif

#include "gl.hpp"
#include "loadpng.hpp"
#include<memory>

#define CHECK_GL_ERROR() \
    { GLenum err = glGetError(); \
      if (err != GL_NO_ERROR) \
        printf("OpenGL Error: %d at %s:%d\n", err, __FILE__, __LINE__); }

GLfloat cubeVertices[] =  {
    // Front face
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // Bottom-left
    0.5f, -0.5f,  0.5f,  1.0f, 0.0f, // Bottom-right
    0.5f,  0.5f,  0.5f,  1.0f, 1.0f, // Top-right
    0.5f,  0.5f,  0.5f,  1.0f, 1.0f, // Top-right
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, // Top-left
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // Bottom-left
    
    // Back face
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, // Bottom-left
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, // Top-left
    0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // Top-right
    0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // Top-right
    0.5f, -0.5f, -0.5f,  1.0f, 0.0f, // Bottom-right
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, // Bottom-left
    
    // Left face
    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // Top-right
    -0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // Top-left
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // Bottom-left
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // Bottom-left
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // Bottom-right
    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // Top-right
    
    // Right face
    0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // Top-left
    0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // Bottom-right
    0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // Top-right
    0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // Bottom-right
    0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // Top-left
    0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // Bottom-left
    
    // Bottom face
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // Top-right
    0.5f, -0.5f, -0.5f,  1.0f, 1.0f, // Top-left
    0.5f, -0.5f,  0.5f,  1.0f, 0.0f, // Bottom-right
    0.5f, -0.5f,  0.5f,  1.0f, 0.0f, // Bottom-right
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // Bottom-left
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // Top-right
    
    // Top face
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, // Top-left
    0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // Bottom-right
    0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // Top-right
    0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // Bottom-right
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, // Top-left
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f  // Bottom-left
};

class GameObject {
public:
    glm::vec3 Position, Size;
    glm::vec3 Velocity;
    GLfloat Rotation;
    GLuint VAO, VBO;
    GLuint texture;
    bool owner = false;
    gl::ShaderProgram* shaderProgram; 

    GameObject(const GameObject&) = delete;
    GameObject& operator=(const GameObject&) = delete;
    GameObject(GameObject&&) = default;
    GameObject& operator=(GameObject&&) = default;


    GameObject() : Position(0, 0, 0), Size(1, 1, 1), Velocity(0), Rotation(0), shaderProgram(nullptr) {}
    virtual ~GameObject() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        if(owner == true) {
            glDeleteTextures(1, &texture);
        }
    }

    void setShaderProgram(gl::ShaderProgram* shaderProgram) {
        this->shaderProgram = shaderProgram;
    }

    virtual void load(gl::GLWindow *win, bool gen_texture = true) {
        if (!shaderProgram) {
            throw mx::Exception("Shader program not set for GameObject");
        }

        shaderProgram->useProgram();
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);

        if(gen_texture) {    
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);

            SDL_Surface *surface = png::LoadPNG(win->util.getFilePath("data/texture.png").c_str());
            if (!surface) {
                throw std::runtime_error("Failed to load texture image");
            }
            GLenum format = (surface->format->BytesPerPixel == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_2D, 0, format, surface->w, surface->h, 0, format, GL_UNSIGNED_BYTE, surface->pixels);
            glGenerateMipmap(GL_TEXTURE_2D);
            SDL_FreeSurface(surface);
            owner = true;
        }
    }

    virtual void draw(gl::GLWindow *win) {
        if (!shaderProgram) {
            throw std::runtime_error("Shader program not set for GameObject");
        }

        shaderProgram->useProgram();

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, Position);
        model = glm::rotate(model, glm::radians(Rotation), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, Size);
        shaderProgram->setUniform("model", model);
        shaderProgram->setUniform("texture1", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }

    virtual void update(float deltaTime) {
        Position += Velocity * deltaTime;
    }
};



class Paddle : public GameObject {
public:
    float CurrentRotation;  
    bool Rotating;          
    float RotationSpeed;    

    Paddle() : CurrentRotation(0.0f), Rotating(false), RotationSpeed(360.0f) {}

    void load(gl::GLWindow *win, bool gen_texture) override {
        GameObject::load(win, true);
        Size = glm::vec3(2.0f, 0.5f, 1.0f);
    }

    void processInput(const Uint8 *state, float deltaTime) {
        float speed = 5.0f;
        if (state[SDL_SCANCODE_LEFT]) {
            Position.x -= speed * deltaTime;
        }
        if (state[SDL_SCANCODE_RIGHT]) {
            Position.x += speed * deltaTime;
        }
        Position.x = glm::clamp(Position.x, -5.0f, 5.0f);
    
    }

    void startRotation() {
        if (!Rotating) {
            Rotating = true;
            CurrentRotation = 0.0f; 
        }
    }

    void update(float deltaTime) override {
        if (Rotating) {
            CurrentRotation += RotationSpeed * deltaTime;
            if (CurrentRotation >= 360.0f) {
                CurrentRotation = 0.0f; 
                Rotating = false;      
            }
        }
    }

    void draw(gl::GLWindow *win) override {
        if (!shaderProgram) {
            throw std::runtime_error("Shader program not set for Paddle");
        }

        shaderProgram->useProgram();

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, Position);
        model = glm::rotate(model, glm::radians(CurrentRotation), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotate along X-axis
        model = glm::scale(model, Size);
        shaderProgram->setUniform("model", model);

        shaderProgram->setUniform("texture1", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }

    void handleTouch(float touchX) {
        Position.x = glm::clamp(touchX, -5.0f, 5.0f); 
    }
};

class Ball : public GameObject {
public:
    float Radius;
    bool Stuck;

    Ball() : Radius(0.25f), Stuck(true) {}

    void load(gl::GLWindow *win, bool gen_texture) override {
        GameObject::load(win, true);
        Size = glm::vec3(Radius * 2.0f);
    }

    void move(float deltaTime) {
        if (!Stuck) {
            Position += Velocity * deltaTime;
            if (Position.x <= -5.0f + Radius) {
                Velocity.x = -Velocity.x;
                Position.x = -5.0f + Radius;
            }
            if (Position.x >= 5.0f - Radius) {
                Velocity.x = -Velocity.x;
                Position.x = 5.0f - Radius;
            }
            if (Position.y >= 3.75f - Radius) {
                Velocity.y = -Velocity.y;
                Position.y = 3.75f - Radius;
            }
        }
    }

};

class Block : public GameObject {
public:
    bool Destroyed;
    bool Rotating;          
    float CurrentRotation;  
    float RotationSpeed;   

    Block() : Destroyed(false), Rotating(false), CurrentRotation(0.0f), RotationSpeed(360.0f) {}

    void load(gl::GLWindow *win, bool gen_texture) override {
        GameObject::load(win, false);
        Size = glm::vec3(1.0f, 0.5f, 1.0f);


    }

    void startRotation() {
        if (!Rotating) {
            Rotating = true;
            CurrentRotation = 0.0f; 
        }
    }

    void update(float deltaTime) override {
        if (Rotating) {
            CurrentRotation += RotationSpeed * deltaTime;
            if (CurrentRotation >= 360.0f) {
                CurrentRotation = 0.0f; 
                Rotating = false;      
                Destroyed = true;
            }
        }
    }

    void draw(gl::GLWindow *win) override {
        if (!shaderProgram) {
            throw std::runtime_error("Shader program not set for Block");
        }

        shaderProgram->useProgram();

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, Position);
        model = glm::rotate(model, glm::radians(CurrentRotation), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotate along X-axis
        model = glm::scale(model, Size);
        shaderProgram->setUniform("model", model);

        shaderProgram->setUniform("texture1", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }
};


class BreakoutGame : public gl::GLObject {
public:
    std::vector<Block> Blocks;
    Paddle PlayerPaddle;
    Ball GameBall;
    glm::mat4 projection;
    gl::ShaderProgram shaderProgram;
    gl::ShaderProgram textShader;
    std::vector<GLuint> Textures;
    mx::Font font;
    int score = 0;
    Uint32 lastTapTime = 0; 
    const Uint32 doubleTapThreshold = 300; 

    BreakoutGame() {}
    ~BreakoutGame() override {
        for(auto &t : Textures) {
            glDeleteTextures(1, &t);
        }
    }
    
    void loadTextures(gl::GLWindow *win) {
        for (int i = 0; i < 4; ++i) {
            GLuint textureID;
            glGenTextures(1, &textureID);
            glBindTexture(GL_TEXTURE_2D, textureID);
            std::string texturePath = win->util.getFilePath("data/texture" + std::to_string(i) + ".png");
            SDL_Surface *surface = png::LoadPNG(texturePath.c_str());
            if (!surface) {
                throw mx::Exception("failed to load texture");
            }
            GLenum format = (surface->format->BytesPerPixel == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_2D, 0, format, surface->w, surface->h, 0, format, GL_UNSIGNED_BYTE, surface->pixels);
            glGenerateMipmap(GL_TEXTURE_2D);
            SDL_FreeSurface(surface);
            Textures.push_back(textureID); 
        }
    }

    void load(gl::GLWindow *win) override {

        font.loadFont(win->util.getFilePath("data/font.ttf"), 24);

        loadTextures(win);
        if (!shaderProgram.loadProgram(win->util.getFilePath("data/tri.vert"), win->util.getFilePath("data/tri.frag"))) {
            throw mx::Exception("Failed to load shader program");
        }
        if (!textShader.loadProgram(win->util.getFilePath("data/text.vert"), win->util.getFilePath("data/text.frag"))) {
            throw mx::Exception("Failed to load shader program");
        }

        shaderProgram.useProgram();
        projection = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -100.0f, 100.0f); // Adjust near and far planes
        shaderProgram.setUniform("projection", projection);
        glm::mat4 view = glm::mat4(1.0f); 
        shaderProgram.setUniform("view", view);
        PlayerPaddle.setShaderProgram(&shaderProgram);
        GameBall.setShaderProgram(&shaderProgram);
        PlayerPaddle.load(win, true);
        PlayerPaddle.Position = glm::vec3(0.0f, -3.5f, 0.0f);
        GameBall.load(win, true);
        GameBall.Position = PlayerPaddle.Position + glm::vec3(0.0f, 0.5f, 0.0f);
        GameBall.Velocity = glm::vec3(2.5f, 2.5f, 0.0f);
        Blocks.reserve(5 * 11); 

        for (int i = 0; i < 5; ++i) {
            for (int j = 0; j < 11; ++j) {
                Blocks.emplace_back(); 
                Block& block = Blocks.back();
                block.setShaderProgram(&shaderProgram);
                block.load(win, false);
                block.Position = glm::vec3(-5.0f + j * 1.0f, 3.0f - i * 0.6f, 0.0f);
                int randomTextureIndex = rand() % Textures.size();
                block.texture = Textures[randomTextureIndex];
            }
        }
    }


    void update(float deltaTime, gl::GLWindow *win) {
        const Uint8 *state = SDL_GetKeyboardState(NULL);
        PlayerPaddle.processInput(state, deltaTime);
        if (state[SDL_SCANCODE_SPACE]) {
            GameBall.Stuck = false;
        }
        PlayerPaddle.update(deltaTime); 
        GameBall.move(deltaTime);
        for (auto &block : Blocks) {
            block.update(deltaTime);
        }
        doCollisions();
    }
    void draw(gl::GLWindow *win) override {
#ifdef __EMSCRIPTEN__
        static Uint32 lastTime = emscripten_get_now();
        float currentTime = emscripten_get_now();
#else
        static Uint32 lastTime = SDL_GetTicks();
        Uint32 currentTime = SDL_GetTicks();
#endif
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        if (deltaTime > 0.05f) deltaTime = 0.05f; 
        lastTime = currentTime;
        update(deltaTime, win);
        shaderProgram.useProgram();
        PlayerPaddle.draw(win);
        GameBall.draw(win);
        for (auto &block : Blocks) {
            if (!block.Destroyed) {
                block.draw(win);
            }
        }

        glBindVertexArray(0);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        textShader.useProgram();
        SDL_Color white = {255, 255, 255, 255};
        int textWidth = 0, textHeight = 0;
        GLuint textTexture = createTextTexture("Score: " + std::to_string(score), font.wrapper().unwrap(), white, textWidth, textHeight);
        renderText(textTexture, textWidth, textHeight, win->w, win->h);
        glDeleteTextures(1, &textTexture);
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        shaderProgram.useProgram();
    }

    void doCollisions() {
        if (checkCollision(GameBall, PlayerPaddle)) {
            float paddleCenter = PlayerPaddle.Position.x;
            float ballCenter = GameBall.Position.x;
            float distance = ballCenter - paddleCenter;
            float normalizedDistance = distance / (PlayerPaddle.Size.x / 2.0f);
            float speed = glm::length(GameBall.Velocity);
            GameBall.Velocity.x = normalizedDistance * speed;
            GameBall.Velocity.y = abs(GameBall.Velocity.y);
            GameBall.Position.y = PlayerPaddle.Position.y + PlayerPaddle.Size.y / 2.0f + GameBall.Radius;\
            PlayerPaddle.startRotation();
        }

        for (Block &block : Blocks) {
            if (!block.Destroyed && checkCollision(GameBall, block)) {
                block.startRotation();
                score += 10;
                GameBall.Velocity.y = -GameBall.Velocity.y;
                if (GameBall.Position.y > block.Position.y) {
                    GameBall.Position.y += GameBall.Radius;
                } else {
                    GameBall.Position.y -= GameBall.Radius;
                }
            }
        }
        if (GameBall.Position.y <= -3.75f) {
            resetGame();
        }
    }

    void resetGame() {
        GameBall.Position = PlayerPaddle.Position + glm::vec3(0.0f, 0.5f, 0.0f);
        GameBall.Velocity = glm::vec3(2.5f, 2.5f, 0.0f);
        GameBall.Stuck = true;
    }

    bool checkCollision(Ball &ball, GameObject &object) {
        glm::vec3 center(ball.Position);
        glm::vec3 aabb_half_extents(object.Size.x / 2.0f, object.Size.y / 2.0f, 0.0f);
        glm::vec3 aabb_center(
            object.Position.x,
            object.Position.y,
            0.0f
        );
        glm::vec3 difference = center - aabb_center;
        glm::vec3 clamped = glm::clamp(difference, -aabb_half_extents, aabb_half_extents);
        glm::vec3 closest = aabb_center + clamped;
        difference = closest - center;
        return glm::length(difference) < ball.Radius;
    }

    virtual void event(gl::GLWindow *win, SDL_Event &e) override {
        if (e.type == SDL_FINGERDOWN) {
            Uint32 currentTapTime = SDL_GetTicks(); 

            if (currentTapTime - lastTapTime <= doubleTapThreshold) {
                if (GameBall.Stuck) {
                    GameBall.Stuck = false; 
                    std::cout << "Double-tap detected: Ball released!\n";
                }
            }
            lastTapTime = currentTapTime;
        } else if (e.type == SDL_FINGERMOTION) {
            float normalizedX = e.tfinger.x;
            float gameX = (normalizedX * 10.0f) - 5.0f;
            PlayerPaddle.handleTouch(gameX);
        }
    }

    SDL_Surface* flipSurface(SDL_Surface* surface) {
        SDL_LockSurface(surface);
        Uint8* pixels = (Uint8*)surface->pixels;
        int pitch = surface->pitch;
        Uint8* tempRow = new Uint8[pitch];
        for (int y = 0; y < surface->h / 2; ++y) {
            Uint8* row1 = pixels + y * pitch;
            Uint8* row2 = pixels + (surface->h - y - 1) * pitch;
            memcpy(tempRow, row1, pitch);
            memcpy(row1, row2, pitch);
            memcpy(row2, tempRow, pitch);
        }
        
        delete[] tempRow;
        SDL_UnlockSurface(surface);
        return surface;
    }
    
    GLuint createTextTexture(const std::string &text, TTF_Font *font, SDL_Color color, int &textWidth, int &textHeight) {
        SDL_Surface *surface = TTF_RenderText_Blended(font, text.c_str(), color);
        if (!surface) {
            mx::system_err << "Failed to create text surface: " << TTF_GetError() << std::endl;
            return 0;
        }
        
        surface = flipSurface(surface);
        
        textWidth = surface->w;
        textHeight = surface->h;
        
        if (surface->format->format != SDL_PIXELFORMAT_RGBA32) {
            SDL_Surface *converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
            if (!converted) {
                mx::system_err << "Failed to convert surface format: " << SDL_GetError() << std::endl;
                SDL_FreeSurface(surface);
                return 0;
            }
            SDL_FreeSurface(surface);
            surface = converted;
        }
        
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textWidth, textHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
        
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            mx::system_err << "OpenGL Error: " << error << " while creating texture." << std::endl;
            SDL_FreeSurface(surface);
            return 0;
        }
        SDL_FreeSurface(surface);
        return texture;
    }
    
    void renderText(GLuint texture, int textWidth, int textHeight, int screenWidth, int screenHeight) {
        float x = -0.5f * (float)textWidth / screenWidth * 2.0f;
        float y = 1.0f;
        float w = (float)textWidth / screenWidth * 2.0f;
        float h = (float)textHeight / screenHeight * 2.0f;
        
        GLfloat vertices[] = {
            x,     y,      0.0f, 0.0f, 1.0f,
            x + w, y,      0.0f, 1.0f, 1.0f,
            x,     y - h,  0.0f, 0.0f, 0.0f,
            x + w, y - h,  0.0f, 1.0f, 0.0f
        };
        
        GLuint VBO, VAO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        textShader.useProgram();
        textShader.setUniform("textTexture", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        
        glBindVertexArray(0);
        glDeleteBuffers(1, &VBO);
        glDeleteVertexArrays(1, &VAO);
    }
};

class MainWindow : public gl::GLWindow {
public:
   

    MainWindow(std::string path, int tw, int th) : gl::GLWindow("Breakout Game", tw, th) {
        setPath(path);
        setObject(new BreakoutGame());
        object->load(this);
    }

    ~MainWindow() override {}

    virtual void event(SDL_Event &e) override {
        object->event(this, e);
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
    int value = 0;
    int tw = 960, th = 720;
    try {
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
                    std::string left, right;
                    left = arg.arg_value.substr(0, pos);
                    right = arg.arg_value.substr(pos + 1);
                    tw = atoi(left.c_str());
                    th = atoi(right.c_str());
                }
                break;
            }
        }
    } catch (const ArgException<std::string>& e) {
        mx::system_err << e.text() << "\n";
    }
    if (path.empty()) {
        mx::system_out << "mx: No path provided trying default current directory.\n";
        path = ".";
    }
    try {
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
