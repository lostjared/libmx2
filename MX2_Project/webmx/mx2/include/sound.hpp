/**
 * @file sound.hpp
 * @brief SDL_mixer audio subsystem wrapper (requires WITH_MIXER build flag).
 *
 * The Mixer class manages loading and playback of both background music
 * (Mix_Music) and short sound effects (Mix_Chunk).
 */
#ifndef __SOUND__HPP_H__
#define __SOUND__HPP_H__
#ifdef __EMSCRIPTEN__
#include "config.hpp"
#else
#include "config.h"
#endif
#ifdef WITH_MIXER
#include "SDL.h"
#include "SDL_mixer.h"
#include <string>
#include <vector>

namespace mx {

    /**
     * @class Mixer
     * @brief SDL_mixer–based audio manager.
     *
     * Provides facilities for loading WAV sound chunks and streamed music
     * tracks, playing them on SDL_mixer channels, and querying playback state.
     * Only available when the library is built with WITH_MIXER defined.
     */
    class Mixer {
      public:
        /** @brief Initialise SDL_mixer and open the audio device. */
        Mixer();

        /** @brief Halt all playback and free all loaded audio resources. */
        ~Mixer();

        Mixer(const Mixer &) = delete;
        Mixer &operator=(const Mixer &) = delete;
        Mixer(Mixer &&) = delete;
        Mixer &operator=(Mixer &&) = delete;

        /** @brief Open the audio device (called automatically by constructor). */
        void init();

        /**
         * @brief Load a WAV file as a sound effect chunk.
         * @param filename Path to the WAV file.
         * @return Index used to reference this chunk in playWav().
         */
        int loadWav(const std::string &filename);

        /**
         * @brief Load an audio file as streaming background music.
         * @param filename Path to the music file (OGG, MP3, WAV, etc.).
         * @return Index used to reference this track in playMusic().
         */
        int loadMusic(const std::string &filename);

        /**
         * @brief Start playing a previously loaded music track.
         * @param id    Music index returned by loadMusic().
         * @param value Number of additional loops (0 = play once, -1 = infinite).
         * @return 0 on success, negative on failure.
         */
        int playMusic(int id, int value = 0);

        /**
         * @brief Play a previously loaded WAV chunk.
         * @param id      Chunk index returned by loadWav().
         * @param value   Number of additional loops (0 = play once).
         * @param channel SDL_mixer channel to play on (0 = first available).
         * @return The channel used for playback, or -1 on failure.
         */
        int playWav(int id, int value = 0, int channel = 0);

        /**
         * @brief Query whether a mixer channel is currently playing.
         * @param channel SDL_mixer channel number.
         * @return @c true if the channel is active.
         */
        bool isPlaying(int channel) const;

        /** @brief Free all loaded audio chunks and music tracks. */
        void cleanup();

        /** @brief Stop background music playback immediately. */
        void stopMusic();

      private:
        bool init_ = false;             ///< Whether the audio device is open.
        std::vector<Mix_Music *> files; ///< Loaded music tracks.
        std::vector<Mix_Chunk *> wav;   ///< Loaded WAV chunks.
    };
} // namespace mx

#endif
#endif