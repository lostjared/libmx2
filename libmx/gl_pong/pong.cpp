#include"mx.hpp"
#include"argz.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <GLES3/gl3.h>
#define WITH_GL
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


// Vertices with texture coordinates (Position X, Y, Z, Texture U, V)
GLfloat vertices[] = {
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


GLuint loadTexture(const std::string &path) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    SDL_Surface *surface = png::LoadPNG(path.c_str());
    if (!surface) {
        throw mx::Exception("Failed to load texture: " + path);
    }
    
    GLenum format = (surface->format->BytesPerPixel == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, surface->w, surface->h, 0, format, GL_UNSIGNED_BYTE, surface->pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    SDL_FreeSurface(surface);
    return textureID;
}


class Paddle {
public:
    glm::vec3 position;
    glm::vec3 size;
    GLuint texture;
    float rotationAngle;
    float rotationSpeed;
    bool isRotating;
    
    Paddle(glm::vec3 pos, glm::vec3 sz, const std::string &texturePath)
    : position(pos), size(sz), rotationAngle(0.0f), rotationSpeed(0.0f), isRotating(false) {
        texture = loadTexture(texturePath);
    }
    
    void draw(gl::ShaderProgram &shader, float deltaTime) {
        if (isRotating) {
            rotationAngle += rotationSpeed * deltaTime;
            if (rotationAngle >= 360.0f) {
                rotationAngle = 0.0f;
                isRotating = false;
            }
        }
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        shader.setUniform("texture1", 0);
        
        glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
        model = glm::rotate(model, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, size);
        shader.setUniform("model", model);
        
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    
    void startRotation(float speed) {
        if (!isRotating) {
            rotationSpeed = speed;
            isRotating = true;
        }
    }
};


class Ball {
public:
    glm::vec3 position;
    glm::vec3 velocity;
    float radius;
    float speed;
    GLuint texture;
    
    Ball(glm::vec3 pos, glm::vec3 vel, float r, const std::string &texturePath)
    : position(pos), velocity(vel), radius(r), speed(glm::length(vel)) {
        texture = loadTexture(texturePath);
    }
    
    void draw(gl::ShaderProgram &shader) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        shader.setUniform("texture1", 0);
        
        glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
        model = glm::scale(model, glm::vec3(radius));
        shader.setUniform("model", model);
        
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    
    void resetBall() {
        position = glm::vec3(0.0f, 0.0f, 0.0f);
        float angle = glm::radians(static_cast<float>(rand() % 120 - 60));
        speed = 1.0f;
        
        float vx = cos(angle);
        float vy = sin(angle);
        
        if (std::abs(vx) < 0.5f) {
            vx = (vx < 0) ? -0.5f : 0.5f;
        }
        
        vx *= (rand() % 2 == 0) ? 1.0f : -1.0f;
        
        velocity = glm::normalize(glm::vec3(vx, vy, 0.0f)) * speed;
    }
    
    void update(float deltaTime, Paddle &paddle1, Paddle &paddle2, int &score1, int &score2) {
        position += velocity * deltaTime;
        
        if (position.y + radius > 1.0f) {
            position.y = 1.0f - radius;
            velocity.y = -velocity.y;
        } else if (position.y - radius < -1.0f) {
            position.y = -1.0f + radius;
            velocity.y = -velocity.y;
        }
        
        handlePaddleCollision(paddle1, deltaTime);
        handlePaddleCollision(paddle2, deltaTime);
        
        if ((position.x - radius < paddle1.position.x - paddle1.size.x / 2.0f) &&
            (position.y + radius > paddle1.position.y - paddle1.size.y / 2.0f) &&
            (position.y - radius < paddle1.position.y + paddle1.size.y / 2.0f)) {
            score2++;
            resetBall();
            return;
        }
        
        if ((position.x + radius > paddle2.position.x + paddle2.size.x / 2.0f) &&
            (position.y + radius > paddle2.position.y - paddle2.size.y / 2.0f) &&
            (position.y - radius < paddle2.position.y + paddle2.size.y / 2.0f)) {
            score1++;
            resetBall();
            return;
        }
        
        if (position.x - radius < -1.5f) {
            score2 ++;
            resetBall();
            return;
        }
        if(position.x + radius > 1.5f) {
            score1++;
            resetBall();
        }
    }
    
private:
    float clamp(float value, float min, float max) {
        return std::max(min, std::min(value, max));
    }
    
    void handlePaddleCollision(Paddle &paddle, float deltaTime) {
        float paddleLeft = paddle.position.x - paddle.size.x / 2.0f;
        float paddleRight = paddle.position.x + paddle.size.x / 2.0f;
        float paddleTop = paddle.position.y + paddle.size.y / 2.0f;
        float paddleBottom = paddle.position.y - paddle.size.y / 2.0f;
        float closestX = clamp(position.x, paddleLeft, paddleRight);
        float closestY = clamp(position.y, paddleBottom, paddleTop);
        float distanceX = position.x - closestX;
        float distanceY = position.y - closestY;
        float distanceSquared = (distanceX * distanceX) + (distanceY * distanceY);
        if (distanceSquared < (radius * radius)) {
            float distance = std::sqrt(distanceSquared);
            if (distance == 0.0f) {
                distance = 0.001f;
            }
            float nx = distanceX / distance;
            float ny = distanceY / distance;
            glm::vec3 normal(nx, ny, 0.0f);
            velocity = glm::reflect(velocity, normal);
            position += normal * (radius - distance);
            float impactY = position.y - paddle.position.y;
            velocity.y += impactY * 5.0f;
            float maxVerticalComponent = speed * 0.75f;
            if (std::abs(velocity.y) > maxVerticalComponent) {
                velocity.y = (velocity.y > 0) ? maxVerticalComponent : -maxVerticalComponent;
            }
            velocity = glm::normalize(velocity) * speed;
            paddle.startRotation(360.0f);
        }
    }
};
class PongGame : public gl::GLObject {
public:
    gl::ShaderProgram shaderProgram, textShader;
    GLuint VAO, VBO;
    Paddle paddle1, paddle2;
    Ball ball;
    mx::Font font;
    int score1 = 0, score2 = 0;
    
    PongGame(gl::GLWindow *win)
    : paddle1(glm::vec3(-0.9f, 0.0f, 0.0f), glm::vec3(0.1f, 0.4f, 0.1f), win->util.getFilePath("data/paddle_texture.png")),
    paddle2(glm::vec3(0.9f, 0.0f, 0.0f), glm::vec3(0.1f, 0.4f, 0.1f), win->util.getFilePath("data/paddle_texture.png")),
    ball(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.5f, 0.3f, 0.0f), 0.05f, win->util.getFilePath("data/ball_texture.png")) {}
    
    void load(gl::GLWindow *win) override {
        
        
        font.loadFont(win->util.getFilePath("data/font.ttf"), 24);
        
        shaderProgram.loadProgram(win->util.getFilePath("data/tri.vert"), win->util.getFilePath("data/tri.frag"));
        shaderProgram.useProgram();
        
        textShader.loadProgram(win->util.getFilePath("data/text.vert"), win->util.getFilePath("data/text.frag"));
        
        
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glBindVertexArray(0);
        
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)win->w / win->h, 0.1f, 100.0f);
        shaderProgram.setUniform("projection", projection);
        
        glm::mat4 view = glm::lookAt(
                                     glm::vec3(0.0f, 0.0f, 3.0f),
                                     glm::vec3(0.0f, 0.0f, 0.0f),
                                     glm::vec3(0.0f, 1.0f, 0.0f));
        shaderProgram.setUniform("view", view);
        ball.resetBall();
        
    }
    
    Uint32 lastUpdateTime = SDL_GetTicks();
    
    void draw(gl::GLWindow *win) override {
        shaderProgram.useProgram();
        glBindVertexArray(VAO);
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; // Convert to seconds
        lastUpdateTime = currentTime;
        update(deltaTime);
        paddle1.draw(shaderProgram, 15 / 1000.0f);
        paddle2.draw(shaderProgram, 15 / 1000.0f);
        ball.draw(shaderProgram);
        glBindVertexArray(0);
        
        glDisable(GL_DEPTH_TEST);
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        textShader.useProgram();
        SDL_Color white = {255, 255, 255, 255};
        int textWidth = 0, textHeight = 0;
        GLuint textTexture = createTextTexture("Player 1: " + std::to_string(score1) + " : Player 2: " + std::to_string(score2), font.wrapper().unwrap(), white, textWidth, textHeight);
        renderText(textTexture, textWidth, textHeight, win->w, win->h);
        glDeleteTextures(1, &textTexture);
        
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        shaderProgram.useProgram();
        glBindVertexArray(VAO);
    }
    
    void event(gl::GLWindow *win, SDL_Event &e) override {
        
    }
    
    void update(float deltaTime) {
        ball.update(deltaTime, paddle1, paddle2, score1, score2);
        const Uint8 *state = SDL_GetKeyboardState(NULL);
        
        if (state[SDL_SCANCODE_UP] && paddle1.position.y + paddle1.size.y / 2 < 1.0f) {
            paddle1.position.y += 0.02f;
        }
        if (state[SDL_SCANCODE_DOWN] && paddle1.position.y - paddle1.size.y / 2 > -1.0f) {
            paddle1.position.y -= 0.02f;
        }
        float paddleSpeed = 0.015f;
        
        if (ball.position.y > paddle2.position.y + paddle2.size.y / 4 &&
            paddle2.position.y + paddle2.size.y / 2 < 1.0f) {
            paddle2.position.y += paddleSpeed;
        }
        if (ball.position.y < paddle2.position.y - paddle2.size.y / 4 &&
            paddle2.position.y - paddle2.size.y / 2 > -1.0f) {
            paddle2.position.y -= paddleSpeed;
        }
        
        if (ball.position.x - ball.radius < paddle1.position.x + paddle1.size.x / 2 &&
            ball.position.y < paddle1.position.y + paddle1.size.y / 2 &&
            ball.position.y > paddle1.position.y - paddle1.size.y / 2) {
            ball.velocity.x = -ball.velocity.x;
        }
        
        if (ball.position.x + ball.radius > paddle2.position.x - paddle2.size.x / 2 &&
            ball.position.y < paddle2.position.y + paddle2.size.y / 2 &&
            ball.position.y > paddle2.position.y - paddle2.size.y / 2) {
            ball.velocity.x = -ball.velocity.x;
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
        float y = 0.9f;
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
        
        const int frameDelay = 1000 / 60;
        Uint32 frameStart = SDL_GetTicks();
        int frameTime = SDL_GetTicks() - frameStart;
        
        if (frameDelay > frameTime) {
            SDL_Delay(frameDelay - frameTime);
        }
    }
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("3D Pong", tw, th) {
        setPath(path);
        setObject(new PongGame(this));
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
    }
private:
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
    Argz<std::string> parser(argc, argv);
    parser.addOptionSingle('h', "Display help message")
        .addOptionSingleValue('p', "assets path")
        .addOptionDoubleValue('P', "path", "assets path")
        .addOptionSingleValue('r',"Resolution WidthxHeight")
        .addOptionDoubleValue('R',"resolution", "Resolution WidthxHeight");
    Argument<std::string> arg;
    std::string path;
    int value = 0;
    int tw = 960, th = 720;
    try {
        while((value = parser.proc(arg)) != -1) {
            switch(value) {
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
                    if(pos == std::string::npos)  {
                        mx::system_err << "Error invalid resolution use WidthxHeight\n";
                        mx::system_err.flush();
                        exit(EXIT_FAILURE);
                    }
                    std::string left, right;
                    left = arg.arg_value.substr(0, pos);
                    right = arg.arg_value.substr(pos+1);
                    tw = atoi(left.c_str());
                    th = atoi(right.c_str());
                }
                    break;
            }
        }
    } catch (const ArgException<std::string>& e) {
        mx::system_err << e.text() << "\n";
    }
    if(path.empty()) {
        mx::system_out << "mx: No path provided trying default current directory.\n";
        path = ".";
    }
    try {
        MainWindow main_window(path, tw, th);
        main_window.loop();
    } catch(const mx::Exception &e) {
        mx::system_err << "mx: Exception: " << e.text() << "\n";
        mx::system_err.flush();
        exit(EXIT_FAILURE);
    }
#endif
    return 0;
}