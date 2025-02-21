#ifndef EXPLODE_HPP
#define EXPLODE_HPP

#include "gl.hpp"
#include "mx.hpp"
#ifdef __EMSCRIPTEN__
#include "gtc/random.hpp"
#else
#include<glm/gtc/random.hpp>
#endif

namespace effect {

    struct Particle {
        glm::vec3 position;
        glm::vec3 velocity; 
        glm::vec4 color;
        float lifetime;
    };

    class Explosion { 
    public:
        Explosion(unsigned int max, bool isBoss); 
        ~Explosion() = default;
        void load(gl::GLWindow *win);
        void setInfo(gl::ShaderProgram *prog, GLuint texture_id);
        void update(float deltaTime);
        void draw(gl::GLWindow *win);
        void event(gl::GLWindow *win, SDL_Event &e);
        bool active() const;
        void trigger(glm::vec3 origin);
        void resetParticle(Particle &particle, glm::vec3 origin);
        unsigned int maxParticles;
        GLuint VAO, VBO;
        std::vector<Particle> particles;
        gl::ShaderProgram *shader_program = nullptr;
        GLuint textureID = 0;
        glm::vec4 particleColor;
        bool is_active = false;
        bool isBoss; 
    };

    class ExplosionEmiter {
    public:
        ExplosionEmiter();
        glm::mat4 projection, view, model;
        gl::ShaderProgram shader_program;
        GLuint texture;

        void load(gl::GLWindow *win);
        void update(gl::GLWindow *win, float deltaTime);
        void draw(gl::GLWindow *win);
        void explode(gl::GLWindow *win, glm::vec3 pos, glm::vec4 particleColor, bool isBoss); 

        std::vector<std::unique_ptr<Explosion>> explosions;
    };

} 

#endif 