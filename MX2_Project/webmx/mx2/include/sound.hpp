#ifndef __SOUND__HPP_H__
#define __SOUND__HPP_H__
#ifdef __EMSCRIPTEN__
#include"config.hpp"
#else
#include"config.h"
#endif
#ifdef WITH_MIXER
#include"SDL.h"
#include"SDL_mixer.h"
#include<vector>
#include<string>

namespace mx {
    class Mixer {
    public:
        Mixer();
        ~Mixer();
        void init();
        int loadWav(const std::string &filename);
        int loadMusic(const std::string &filename);
        int playMusic(int id, int value = 0);
        int playWav(int id, int value = 0, int channel = 0);
        bool isPlaying(int channel) const;
        void cleanup();
        void stopMusic();
    private:
        bool init_ = false;
        std::vector<Mix_Music *> files;
        std::vector<Mix_Chunk *> wav;
    };
}

#endif
#endif