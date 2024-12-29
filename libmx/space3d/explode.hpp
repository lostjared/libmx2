#ifndef __EXPLODE_H__
#define __EXPLODE_H__

#include"mx.hpp"
#include"gl.hpp"

namespace effect {

class Explosion : public gl::GLObject {
public:
    struct Particle {
        glm::vec3 position;
        glm::vec3 velocity;
        float lifetime;
        glm::vec4 color;
    };
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
private:
    unsigned int maxParticles;
    unsigned int VAO, VBO;
    gl::ShaderProgram *program = nullptr;
    unsigned int textureID;
    std::vector<Particle> particles;
    void resetParticle(Particle &particle, glm::vec3 origin);

public:
    gl::ShaderProgram shader_program;
};

}

#endif