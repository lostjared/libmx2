/**
 * @file mx.hpp
 * @brief Central SDL2 application window class and related includes.
 *
 * Pulls in all core libmx subsystems and provides mxWindow, which manages
 * the SDL2 window, renderer, event loop, and optional mixer.
 */
#ifndef __MX__H___
#define __MX__H___

#ifdef __EMSCRIPTEN__
#include"config.hpp"
#else
#include"config.h"
#endif

#include"SDL.h"
#include"SDL_ttf.h"
#include<iostream>
#include<string>
#include"util.hpp"
#include"object.hpp"
#include"font.hpp"
#include"tee_stream.hpp"
#include"texture.hpp"
#include"exception.hpp"
#include"joystick.hpp"
#include"wrapper.hpp"
#include"loadpng.hpp"
#include"input.hpp"
#include<memory>

#ifdef WITH_MIXER
#include"sound.hpp"
#endif

namespace mx {

/**
 * @class mxWindow
 * @brief Main SDL2 application window.
 *
 * Creates and manages the SDL2 window and renderer.  Derived classes must
 * implement event() and draw() to provide application-specific behaviour.
 * The window runs a blocking loop() that drives the frame and event cycle.
 */
    class mxWindow {
    public:
        std::unique_ptr<obj::Object> object; ///< Optional attached game object.

        mxWindow() = delete;

        /**
         * @brief Create the SDL2 window.
         * @param name  Window title.
         * @param w     Window width in pixels.
         * @param h     Window height in pixels.
         * @param full  If @c true, start in fullscreen mode.
         */
        mxWindow(const std::string &name, int w, int h, bool full = false);

        /** @brief Destructor — destroys the SDL2 window and renderer. */
        virtual ~mxWindow();

        mxWindow(const mxWindow&) = delete;
        mxWindow& operator=(const mxWindow&) = delete;
        mxWindow(mxWindow&&) = delete;
        mxWindow& operator=(mxWindow&&) = delete;

        /**
         * @brief Process a single SDL event (pure virtual).
         * @param e The SDL event to handle.
         */
        virtual void event(SDL_Event &e) = 0;

        /**
         * @brief Draw the current frame (pure virtual).
         * @param ren Active SDL renderer.
         */
        virtual void draw(SDL_Renderer *ren) = 0;

        /**
         * @brief Create a streaming SDL_Texture of the given size.
         * @param w Width in pixels.
         * @param h Height in pixels.
         * @return New SDL_Texture (caller owns it).
         */
        SDL_Texture *createTexture(int w, int h);

        /**
         * @brief Enable or disable the main loop.
         * @param a @c true to keep running, @c false to stop.
         */
        void setActive(bool a) { active = a; }

        /** @brief Run the event+draw loop until setActive(false) is called. */
        void loop();

        /** @brief Process one iteration of events and drawing. */
        void proc();

        /** @brief Stop the loop on the next cycle (sets active = false). */
        void destroy() { setActive(false); }

        /**
         * @brief Prepend the asset directory to all relative file paths.
         * @param path Asset directory path.
         */
        void setPath(const std::string &path) { util.path = path; }

        /**
         * @brief Set the window icon from an SDL_Surface.
         * @param icon Surface to use as the icon.
         */
        void setIcon(SDL_Surface *icon);

        /**
         * @brief Set the window icon from an image file.
         * @param icon Image file path.
         */
        void setIcon(const std::string &icon);

        /**
         * @brief Toggle fullscreen mode.
         * @param full @c true to enable fullscreen.
         */
        void setFullScreen(bool full);

        /** @brief Request the SDL event loop to quit. */
        void quit();

        mxUtil util;               ///< Utility/asset helper.
        Text text;                 ///< 2D text renderer.
        SDL_Renderer *renderer = nullptr; ///< Active SDL renderer.
        int width = 0;             ///< Window width in pixels.
        int height = 0;            ///< Window height in pixels.

        /**
         * @brief Attach an obj::Object to receive draw/event callbacks.
         * @param o Pointer to the object (window does not take ownership).
         */
        void setObject(obj::Object *o);

        SDL_Window *window = nullptr; ///< Underlying SDL window handle.

        /**
         * @brief Change the window title bar text.
         * @param title New title string.
         */
        void setWindowTitle(const std::string &title);

#ifdef WITH_MIXER
        Mixer mixer; ///< Audio mixer (only when built with WITH_MIXER).
#endif
    protected:
        /**
         * @brief Internal helper: create and initialise the SDL window+renderer.
         * @param name  Title string.
         * @param w     Width.
         * @param h     Height.
         * @param full  Fullscreen flag.
         */
        void create_window(const std::string &name, int w, int h, bool full);
        bool active = false; ///< Controls the main loop.
        SDL_Event e;         ///< Reusable SDL event structure.
    };
}

#endif
