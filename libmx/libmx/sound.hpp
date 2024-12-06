#ifndef __SOUND__HPP_H__
#define __SOUND__HPP_H__
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
        int loadMusic(const std::string &filename);
        int playMusic(int id, int value = 0);
    private:
        bool init_ = false;
        std::vector<Mix_Music *> files;
    };
}




#endif
#endif