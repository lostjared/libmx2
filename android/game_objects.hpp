#ifndef __GAME_OBJ_H__
#define __GAME_OBJ_H__

#include "mx.hpp"
#include"gl.hpp"
#include"loadpng.hpp"
#include"model.hpp"
#include<memory>
#include<vector>

#define CHECK_GL_ERROR() \
    { GLenum err = glGetError(); \
      if (err != GL_NO_ERROR) \
        printf("OpenGL Error: %d at %s:%d\n", err, __FILE__, __LINE__); }


class GameObject {
public:
    glm::vec3 Position, Size;
    glm::vec3 Velocity;
    GLfloat Rotation;
    GLuint texture;
    bool owner = false;
    gl::ShaderProgram* shaderProgram; 
    mx::Model cube;
    GameObject(const GameObject&) = delete;
    GameObject& operator=(const GameObject&) = delete;
    GameObject(GameObject&&) = default;
    GameObject& operator=(GameObject&&) = default;




    GameObject() : Position(0, 0, 0), Size(1, 1, 1), Velocity(0), Rotation(0), shaderProgram(nullptr) {}
    virtual ~GameObject() {}

    void setShaderProgram(gl::ShaderProgram* shaderProgram) {
        this->shaderProgram = shaderProgram;
    }

    virtual void load(gl::GLWindow *win, bool gen_texture = true) {
        if (!shaderProgram) {
            throw mx::Exception("Shader program not set for GameObject");
        }
        if(!cube.openModel(win->util.getFilePath("cube.mxmod"))) {
            throw mx::Exception("Could not load model cube.mxmod");
        }

        shaderProgram->useProgram();
        cube.setShaderProgram(shaderProgram, "texture1");
        glBindVertexArray(0);

        if(gen_texture) {    
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);

            SDL_Surface *surface = png::LoadPNG(win->util.getFilePath("texture.png").c_str());
            if (!surface) {
                throw std::runtime_error("Failed to load texture image");
            }
            GLenum format = (surface->format->BytesPerPixel == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_2D, 0, format, surface->w, surface->h, 0, format, GL_UNSIGNED_BYTE, surface->pixels);
            glGenerateMipmap(GL_TEXTURE_2D);
            SDL_FreeSurface(surface);
            owner = true;
            std::vector<GLuint> textures{texture};
            cube.setTextures(textures);
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
        cube.drawArrays();
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

   void processInput(mx::Controller &stick, const Uint8 *state, float deltaTime) {
        float speed = 5.0f;
        float analogThreshold = 8000.0f; 
        float analogInput = stick.getAxis(SDL_CONTROLLER_AXIS_LEFTX); 
        if (state[SDL_SCANCODE_LEFT]) {
            Position.x -= speed * deltaTime;
        }
        if (state[SDL_SCANCODE_RIGHT]) {
            Position.x += speed * deltaTime;
        }
        if (analogInput < -analogThreshold) {
            Position.x -= speed * deltaTime * (std::abs(analogInput) / 32768.0f); 
        }
        if (analogInput > analogThreshold) {
            Position.x += speed * deltaTime * (std::abs(analogInput) / 32768.0f); 
        }
        Position.x = glm::clamp(Position.x, -5.0f, 5.0f);
    }

    void eventProc(SDL_Event &e) {
        
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
        cube.drawArrays();
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
        cube.drawArrays();
        glBindVertexArray(0);
    }
};

#endif