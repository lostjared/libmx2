#ifndef _ANI_H__
#define _ANI_H__
#ifndef __EMSCRIPTEN__
#include"config.h"
#else
#include"config.hpp"
#endif
#include"mx.hpp"
#include"gl.hpp"

#ifdef __EMSCRIPTEN__
#include<emscripten/emscripten.h>
#include<emscripten/bind.h>

using namespace emscripten;

#endif

extern const char *vSource;
extern const char *fSource;
extern const char *anifSource;
extern const char *anifSource2;
extern const char *anifSource3;
extern const char *anifSource4;
extern const char *anifSource5;
extern const char *anifSource6;
extern const char *anifSource7;
extern const char *anifSource8;
extern const char *anifSource9;
extern const char *anifSource10;
extern float the_time;
extern gl::GLSprite sprite;
static constexpr int MAX_SHADER = 11;
extern gl::ShaderProgram shader[MAX_SHADER];
extern size_t shader_index;
extern bool playing;

class Animation : public gl::GLObject {
public:
    
     Animation() = default;
    ~Animation() override {}

    void load(gl::GLWindow *win) override {
        font.loadFont(win->util.getFilePath("data/font.ttf"), 24);
        if(!shader[0].loadProgramFromText(vSource, anifSource)) {
            throw mx::Exception("Could not load shader");
        }
        if(!shader[1].loadProgramFromText(vSource, anifSource2)) {
            throw mx::Exception("Could not load shader");
        }
        if(!shader[2].loadProgramFromText(vSource, anifSource4)) {
            throw mx::Exception("Could not load shader");
        }
        if(!shader[3].loadProgramFromText(vSource, anifSource5)) {
            throw mx::Exception("Could not load shader");
        }
        if(!shader[4].loadProgramFromText(vSource, anifSource3)) {
            throw mx::Exception("Could not load shader");
        }
        if(!shader[5].loadProgramFromText(vSource, anifSource6)) {
            throw mx::Exception("Could not load shader");
        }
        if(!shader[6].loadProgramFromText(vSource, anifSource7)) {
            throw mx::Exception("Could not load shader");
        }
        
        if(!shader[7].loadProgramFromText(vSource, anifSource8)) {
            throw mx::Exception("Could not load shader");
        }
        if(!shader[8].loadProgramFromText(vSource, anifSource8)) {
            throw mx::Exception("Could not load shader");
        }
        if(!shader[9].loadProgramFromText(vSource, anifSource9)) {
            throw mx::Exception("Could not load shader");
        }
        
        if(!shader[10].loadProgramFromText(vSource, anifSource10)) {
            throw mx::Exception("Could not load shader");
        }
        for(int i = 0; i < MAX_SHADER; ++i) {
            shader[i].useProgram();
            shader[i].setUniform("iResolution", glm::vec2(win->w, win->h));
            shader[i].setUniform("time_f", static_cast<float>(SDL_GetTicks()));
        }
        sprite.initSize(win->w, win->h);
        sprite.loadTexture(&shader[shader_index], win->util.getFilePath("data/bg.png"), 0, 0, win->w, win->h);
    }

    void draw(gl::GLWindow *win) override {
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; // Convert to seconds
        lastUpdateTime = currentTime;
        update(deltaTime);
        sprite.setShader(&shader[shader_index]);
        shader[shader_index].useProgram();

        if(playing)
            the_time = static_cast<float>(SDL_GetTicks() / 1000.0f);

        shader[shader_index].setUniform("time_f", the_time);
        sprite.draw();
        win->text.setColor({255,255,255,255});
        win->text.printText_Solid(font, 25, 25, "Acid Cam Shaders Early Demo: https://lostsidedead.biz");
    }
    void event(gl::GLWindow *win, SDL_Event &e) override {
        
        
    }
    void update(float deltaTime) {}
private:
    Uint32 lastUpdateTime = SDL_GetTicks();
    mx::Font font;
    
};

#endif