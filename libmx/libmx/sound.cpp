#ifdef WITH_MIXER
#include"sound.hpp"
#include"mx.hpp"
namespace mx {

        Mixer::Mixer() : init_{false} {

        }
        Mixer::~Mixer() {
            if(init_) {
                for (auto &m : files) {
                    Mix_FreeMusic(m);
                }
                Mix_CloseAudio();
            }
        }
        
        void Mixer::init() {
             if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
                throw mx::Exception("Could not initalize: " + std::string(Mix_GetError()));
            }
            //mx::system_out << "mx: Mixer: initalized...\n";
            init_ = true;
        }

         int Mixer::loadMusic(const std::string &filename) {
            Mix_Music *m = Mix_LoadMUS(filename.c_str());
            if(!m) {
                Mix_CloseAudio();
                throw mx::Exception("Error loading music file: " + filename + ": " +std::string(Mix_GetError()));
                exit(EXIT_FAILURE);
            }
            files.push_back(m);
            return static_cast<int>(files.size()-1);
        }

        int Mixer::playMusic(int id, int value) {
            if(id >= 0 && id < static_cast<int>(files.size())) {
                if (Mix_PlayMusic(files.at(id), value) == -1) {
                    Mix_FreeMusic(files.at(id));
                    Mix_CloseAudio();
                    throw mx::Exception("Play music failed: " + std::string(Mix_GetError()));
                    return -1;
                }
            } else {
                return -1;
            }
            return 0;
        }
}

#endif