/**
 * @file object.hpp
 * @brief Abstract game-object interface for the SDL window loop.
 */
#ifndef __GAME_OBJECT__H_
#define __GAME_OBJECT__H_

#include "SDL.h"
#include <memory>

namespace mx {
    class mxWindow;
}

namespace obj {

    /**
     * @class Object
     * @brief Abstract base class for all drawable game objects.
     *
     * Derived classes are attached to an mx::mxWindow and have their
     * load(), draw(), and event() callbacks invoked each frame.
     */
    class Object {
      public:
        virtual ~Object() = default;

        /**
         * @brief Render the object for the current frame.
         * @param win Pointer to the owning window.
         */
        virtual void draw(mx::mxWindow *win) = 0;

        /**
         * @brief Handle an SDL event.
         * @param win Pointer to the owning window.
         * @param e   The SDL event to process.
         */
        virtual void event(mx::mxWindow *win, SDL_Event &e) = 0;

        /**
         * @brief Load resources needed by the object.
         * @param win Pointer to the owning window.
         */
        virtual void load(mx::mxWindow *win) = 0;
    };
} // namespace obj

#endif