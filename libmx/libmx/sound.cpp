#ifdef __EMSCRIPTEN__
#include"config.hpp"
#else
#include"config.h"
#endif

#ifdef WITH_MIXER
#include"sound.hpp"
#include"mx.hpp"
namespace mx {

        Mixer::Mixer() : init_{false} {

        }
        Mixer::~Mixer() {
           cleanup();
        }

        void Mixer::cleanup() {
            if(init_ == true) {
                stopMusic();
                for(auto &i : files) {
                    if(i != nullptr) {
                        Mix_FreeMusic(i);
                        i = nullptr;
                    }
                }
                for(auto &i : wav) {
                    if(i != nullptr) {
                        Mix_FreeChunk(i);
                        i = nullptr;
                    }
                }
                files.clear();
                wav.clear();
                Mix_CloseAudio();
                Mix_Quit();
                init_ = false;
            }

        }
        
        void Mixer::stopMusic() {
            if (Mix_QuerySpec(nullptr, nullptr, nullptr) != 0) {
                Mix_HaltMusic();
                Mix_HaltChannel(-1);
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
        int Mixer::loadWav(const std::string &filename) {
            Mix_Chunk *m = Mix_LoadWAV(filename.c_str());
            if(!m) {
                Mix_CloseAudio();
                throw mx::Exception("Error loading file: " + std::string(Mix_GetError()));
                return -1;
            }
            wav.push_back(m);
            return static_cast<int>(wav.size()-1);
        }

        int Mixer::playWav(int id, int value, int channel) {
            if(id >= 0 && id < static_cast<int>(wav.size())) {
                return Mix_PlayChannel(channel, wav.at(id), value);
            }
            return -1;
        }

        bool Mixer::isPlaying(int channel) const {
            if(Mix_Playing(channel))
                return true;
            return false;
        }
}

#endif