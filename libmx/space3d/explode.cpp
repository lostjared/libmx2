#include "explode.hpp"

#ifdef __EMSCRIPTEN__
#else
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
    gl_PointSize = 10.0;
    particleColor = aColor;
})";

const char *s_fSource = R"(#version 330 core

in vec4 particleColor;
out vec4 FragColor;

uniform sampler2D sprite;

void main() {
    float dist = length(gl_PointCoord - vec2(0.5));
    if (dist > 0.3) {
        discard;
    }
    vec4 texColor = texture(sprite, gl_PointCoord);
    FragColor = texColor * particleColor;
})";

#else

const char *s_vSource = R"(#version 300 es

// Declare high precision for floating-point types
precision highp float;

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;

out vec4 particleColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    gl_PointSize = 10.0;
    particleColor = aColor;
})";


const char *s_fSource = R"(#version 300 es

// Declare medium precision for floating-point types
precision mediump float;

in vec4 particleColor;
out vec4 FragColor;

uniform sampler2D sprite;

void main() {
    float dist = length(gl_PointCoord - vec2(0.5));
    if (dist > 0.3) {
        discard;
    }
    vec4 texColor = texture(sprite, gl_PointCoord);
    FragColor = texColor * particleColor;
})";

#endif

    Explosion::Explosion(unsigned int max, bool isBoss) : maxParticles(max), VAO(0), VBO(0), isBoss(isBoss) { // Modified constructor

    }

    void Explosion::load(gl::GLWindow *win) {
        particles.resize(maxParticles);

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
        this->shader_program = prog;
        this->textureID = texture_id;
    }

    void Explosion::update(float deltaTime) {
        is_active = false;
        for (auto &particle : particles) {
            if (particle.lifetime > 0.0f) {
                particle.lifetime -= deltaTime;
                particle.position += particle.velocity * deltaTime;
                particle.color.a = particle.lifetime;
                is_active = true;
            }
        }
    }

    void Explosion::draw(gl::GLWindow *win) {
#ifndef __EMSCRIPTEN__
        glEnable(GL_PROGRAM_POINT_SIZE);
#endif
        glDisable(GL_DEPTH_TEST);
        shader_program->useProgram();
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, particles.size() * sizeof(Particle), particles.data());
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        shader_program->setUniform("sprite", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glDrawArrays(GL_POINTS, 0, particles.size());

        glBindVertexArray(0);
        glUseProgram(0);
    }

    void Explosion::event(gl::GLWindow *win, SDL_Event &e) {}

    bool Explosion::active() const {

        return is_active;
    }

    void Explosion::trigger(glm::vec3 origin) {
        is_active = true;
        for (auto &particle : particles) {
            resetParticle(particle, origin);
        }
    }

    void Explosion::resetParticle(Particle &particle, glm::vec3 origin) {
        particle.position = origin;
        float velocityScale = 4.0f;
        float lifetimeScale = 0.5f;

        if (isBoss) {
            velocityScale = 8.0f; 
            lifetimeScale = 1.0f; 
        }

        particle.velocity = glm::sphericalRand(velocityScale);
        particle.lifetime = glm::linearRand(lifetimeScale, lifetimeScale * 4.0f);
        particle.color = particleColor;
    }

    ExplosionEmiter::ExplosionEmiter() : shader_program{}, texture{0} {}

    void ExplosionEmiter::load(gl::GLWindow *win) {
        if (!shader_program.loadProgramFromText(s_vSource, s_fSource)) {
            throw mx::Exception("Couldn't load Explosion Shader");
        }
        projection = glm::perspective(glm::radians(45.0f), (float)win->w / (float)win->h, 0.1f, 100.0f);
        view = glm::lookAt(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::mat4(1.0f);
        shader_program.useProgram();
        shader_program.setUniform("projection", projection);
        shader_program.setUniform("view", view);
        shader_program.setUniform("model", model);
        texture = gl::loadTexture(win->util.getFilePath("data/star.png"));
    }

    void ExplosionEmiter::update(gl::GLWindow *win, float deltaTime) {
        for (int i = static_cast<int>(explosions.size()) - 1; i >= 0; i--) {
            explosions[i]->update(deltaTime);
            if (!explosions[i]->active()) {
                explosions.erase(explosions.begin() + i);
            }
        }
    }
    void ExplosionEmiter::draw(gl::GLWindow *win) {
        for (auto &e : explosions) {
            if (e->active())
                e->draw(win);
        }
    }

    void ExplosionEmiter::explode(gl::GLWindow *win, glm::vec3 pos, glm::vec4 particleColor, bool isBoss) { 
        if(isBoss)
            explosions.push_back(std::make_unique<Explosion>(500, isBoss));
        else
            explosions.push_back(std::make_unique<Explosion>(100, isBoss));

        explosions.back()->setInfo(&shader_program, texture);
        explosions.back()->particleColor = particleColor;
        explosions.back()->load(win);
        explosions.back()->trigger(pos);
    }

} 