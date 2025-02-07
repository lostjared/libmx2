#ifndef __EXPLODE_H__
#define __EXPLODE_H__

#include"mx.hpp"
#include"gl.hpp"

namespace effect {

class Explosion : public gl::GLObject {
public:
#pragma pack(push, 1)
    struct Particle {
        glm::vec3 position;
        glm::vec3 velocity;
        float lifetime;
        glm::vec4 color;
    };
#pragma pack(pop)
    Explosion(unsigned int maxParticles);
    ~Explosion() override {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }
    void load(gl::GLWindow *win) override;
    void update(float deltaTime);
    void draw(gl::GLWindow *win) override;
    void trigger(glm::vec3 origin);
    void event(gl::GLWindow *win, SDL_Event &e) override;
    void setInfo(gl::ShaderProgram *prog, GLuint texture_id);
    bool active() const;
    void resize(gl::GLWindow *win, int w, int h) override;

private:
    unsigned int maxParticles;
    GLuint VAO, VBO;
    GLuint textureID;
    std::vector<Particle> particles;
    void resetParticle(Particle &particle, glm::vec3 origin);
    bool is_active = false;

public:
    gl::ShaderProgram *shader_program;
};

class ExplosionEmiter {
public:
    ExplosionEmiter() {}
    ~ExplosionEmiter() { 
        if(texture) {
            glDeleteTextures(1, &texture);
        }
    }

    void load(gl::GLWindow *window);
    void update(gl::GLWindow *window, float deltaTime);
    void draw(gl::GLWindow *window);
    void resize(gl::GLWindow *window, int w, int h);
    void explode(gl::GLWindow *win, glm::vec3 pos);

    std::vector<std::unique_ptr<Explosion>> explosions;
    gl::ShaderProgram shader_program;
private:
    glm::mat4 projection, view, model;
    GLuint texture = 0;
};


}

#endif