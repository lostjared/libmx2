#ifndef __GAME__H__
#define __GAME__H__

#include "game_objects.hpp"

class BreakoutGame : public gl::GLObject {
public:
    std::vector<Block> Blocks;
    Paddle PlayerPaddle;
    Ball GameBall;
    glm::mat4 projection;
    gl::ShaderProgram shaderProgram;
    gl::ShaderProgram textShader;
    std::vector<GLuint> Textures;

    gl::ShaderProgram program;
    gl::GLSprite sprite;

    mx::Font font;
    int score = 0;
    Uint32 lastTapTime = 0; 
    const Uint32 doubleTapThreshold = 300; 
    float gridRotation = 0.0f;
    float gridYRotation = 0.0f; 
    float rotationSpeed = 50.0f; 
    BreakoutGame() {}
    ~BreakoutGame() override {
        for(auto &t : Textures) {
            glDeleteTextures(1, &t);
        }
    }
    void loadTextures(gl::GLWindow *win);
    virtual void load(gl::GLWindow *win) override;
    void update(float deltaTime, gl::GLWindow *win);
    virtual void draw(gl::GLWindow *win) override;
    void doCollisions(gl::GLWindow *win);
    void resetGame();
    bool checkCollision(Ball &ball, GameObject &object);
    virtual void event(gl::GLWindow *win, SDL_Event &e) override;
    GLuint createTextTexture(const std::string &text, TTF_Font *font, SDL_Color color, int &textWidth, int &textHeight);
    void renderText(GLuint texture, int textWidth, int textHeight, int screenWidth, int screenHeight);
private:
    mx::Controller stick;
#ifdef WITH_MIXER
    int ping = -1;
    int clear = -1;
    int die = -1;
#endif
};


#endif
