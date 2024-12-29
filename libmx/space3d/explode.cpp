#include "explode.hpp"
#ifdef __EMSCRIPTEN__
#include "gtc/random.hpp"
#else
#include<glm/gtc/random.hpp>
#endif

namespace effect {

#ifndef __EMSCRIPTEN__

const char *s_vSource = R"(#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;

out vec4 particleColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    gl_PointSize = 25.0; 
    particleColor = aColor;
})";

const char *s_fSource = R"(#version 330 core

in vec4 particleColor;
out vec4 FragColor;

uniform sampler2D sprite;

void main() {
    vec4 texColor = texture(sprite, gl_PointCoord);
    FragColor = texColor * particleColor;
})";

#else

const char *s_vSource = R"(#version 300 es

layout (location = 0) precision highp float;in vec3 aPos;
layout (location = 1) in vec4 aColor;

out vec4 particleColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    gl_PointSize = 25.0; 
    particleColor = aColor;
})";

const char *s_fSource = R"(#version 300 es
precision highp float;
in vec4 particleColor;
out vec4 FragColor;

uniform sampler2D sprite;

void main() {
    vec4 texColor = texture(sprite, gl_PointCoord);
    FragColor = texColor * particleColor;
})";

#endif
 

    Explosion::Explosion(unsigned int max) : maxParticles(max), VAO(0), VBO(0) {

    }

    void Explosion::load(gl::GLWindow *win) {
        particles.resize(maxParticles);
        shader_program.loadProgramFromText(s_vSource, s_fSource);

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, maxParticles * sizeof(Particle), nullptr, GL_DYNAMIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, position));
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, color));
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void Explosion::setInfo(gl::ShaderProgram *prog, GLuint texture_id) {
        this->program = prog;
        this->textureID = texture_id;
    }
        
    void Explosion::update(float deltaTime) {
        for (auto &particle : particles) {
                if (particle.lifetime > 0.0f) {
                    particle.lifetime -= deltaTime;
                    particle.position += particle.velocity * deltaTime;
                    particle.color.a = particle.lifetime; 
                }
        }
    }

    void Explosion::draw(gl::GLWindow *win) {
        glDisable(GL_DEPTH_TEST);
        shader_program.useProgram();
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, particles.size() * sizeof(Particle), particles.data());
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glActiveTexture(GL_TEXTURE0);
        shader_program.setUniform("sprite", 0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glDrawArrays(GL_POINTS, 0, particles.size());

        glBindVertexArray(0);
        glUseProgram(0);
    }

    void Explosion::event(gl::GLWindow *win, SDL_Event &e) {}

    void Explosion::trigger(glm::vec3 origin) {
        for (auto &particle : particles) {
            resetParticle(particle, origin);
        }
    }

    void Explosion::resetParticle(Particle &particle, glm::vec3 origin) {
        particle.position = origin;
        particle.velocity = glm::sphericalRand(1.0f);
        particle.lifetime = glm::linearRand(0.5f, 2.0f);
        particle.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f); 
    }
}         