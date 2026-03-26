/**
 * @file joystick.hpp
 * @brief SDL joystick and game controller RAII wrappers.
 *
 * Joystick wraps SDL_Joystick for generic axis/hat/button devices.
 * Controller wraps SDL_GameController for the higher-level game controller
 * API (standard button mappings, triggers, etc.).
 */
#ifndef __JOYSTICK__H__
#define __JOYSTICK__H__

#include "SDL.h"
#include "wrapper.hpp"
#include <optional>
#include <string>

namespace mx {

    /**
     * @class Joystick
     * @brief RAII wrapper for a raw SDL_Joystick device.
     *
     * Opens an SDL joystick by index and provides axis, hat, and button
     * read-back.  The handle is automatically closed on destruction.
     */
    class Joystick {
      public:
        /** @brief Default constructor — device not yet opened. */
        Joystick();
        /** @brief Destructor — closes the joystick if open. */
        ~Joystick();

        Joystick(const Joystick &) = delete;
        Joystick &operator=(const Joystick &) = delete;
        Joystick(Joystick &&) = delete;
        Joystick &operator=(Joystick &&) = delete;

        /**
         * @brief Open the joystick at the given device index.
         * @param index SDL joystick index (0-based).
         * @return @c true on success.
         */
        bool open(int index);

        /**
         * @brief Return the joystick's name as reported by SDL.
         * @return Device name string.
         */
        std::string name() const;

        /** @brief Close the joystick handle if open. */
        void close();

        /**
         * @brief Return the underlying SDL_Joystick handle as an optional.
         * @return std::optional containing the handle, or std::nullopt.
         */
        std::optional<SDL_Joystick *> handle();

        /**
         * @brief Unwrap the SDL_Joystick pointer, asserting it is open.
         * @return Raw SDL_Joystick pointer.
         */
        SDL_Joystick *unwrap() const;

        /**
         * @brief Return the device index this joystick was opened with.
         * @return Device index.
         */
        int joystickIndex() const;

        /**
         * @brief Return the number of connected joysticks.
         * @return SDL_NumJoysticks() result.
         */
        static int joysticks() { return SDL_NumJoysticks(); }

        /** @return Number of buttons on this joystick. */
        int numButtons();
        /** @return Number of hats on this joystick. */
        int numHats();
        /** @return Number of axes on this joystick. */
        int numAxes();

        /**
         * @brief Test whether a button is currently pressed.
         * @param button Button index.
         * @return @c true if pressed.
         */
        bool getButton(int button);

        /**
         * @brief Read the position of a POV hat.
         * @param hat Hat index.
         * @return SDL hat position bitmask.
         */
        Uint8 getHat(int hat);

        /**
         * @brief Read the current axis value.
         * @param axis Axis index.
         * @return Axis value in the range [-32768, 32767].
         */
        Sint16 getAxis(int axis);

        /**
         * @brief Return a Wrapper around the SDL_Joystick handle.
         * @return Wrapper holding the pointer or nullopt.
         */
        mx::Wrapper<SDL_Joystick *> wrapper() const {
            if (stick)
                return stick;
            return std::nullopt;
        }

      protected:
        SDL_Joystick *stick = nullptr; ///< Underlying SDL joystick handle.
        int index = -1;                ///< Device index (-1 if not open).
    };

    /**
     * @class Controller
     * @brief RAII wrapper for SDL_GameController (standard layout mapping).
     *
     * Provides button, hat, and axis queries using SDL's gamecontroller API,
     * which normalises button layouts across different physical devices.
     */
    class Controller {
      public:
        /** @brief Default constructor — device not yet opened. */
        Controller();
        /** @brief Destructor — closes the controller if open. */
        ~Controller();

        Controller(const Controller &) = delete;
        Controller &operator=(const Controller &) = delete;
        Controller(Controller &&) = delete;
        Controller &operator=(Controller &&) = delete;

        /**
         * @brief Open a game controller by device index.
         * @param index SDL joystick index of the controller.
         * @return @c true on success.
         */
        bool open(int index);

        /**
         * @brief Handle a device-added/removed event to maintain connection state.
         * @param e SDL event to inspect.
         * @return @c true if the event was a controller connect/disconnect.
         */
        bool connectEvent(SDL_Event &e);

        /**
         * @brief Return the controller's name as reported by SDL.
         * @return Device name string.
         */
        std::string name() const;

        /** @brief Close the controller handle if open. */
        void close();

        /**
         * @brief Return the underlying SDL_GameController handle as an optional.
         * @return std::optional containing the handle, or std::nullopt.
         */
        std::optional<SDL_GameController *> handle();

        /**
         * @brief Unwrap the SDL_GameController pointer, asserting it is open.
         * @return Raw SDL_GameController pointer.
         */
        SDL_GameController *unwrap() const;

        /**
         * @brief Return the device index this controller was opened with.
         * @return Device index.
         */
        int controllerIndex() const;

        /**
         * @brief Return the number of connected joysticks/controllers.
         * @return SDL_NumJoysticks() result.
         */
        static int joysticks() { return SDL_NumJoysticks(); }

        /**
         * @brief Test whether a mapped button is currently pressed.
         * @param button SDL_GameControllerButton constant.
         * @return @c true if pressed.
         */
        bool getButton(SDL_GameControllerButton button);

        /**
         * @brief Read the position of a POV hat.
         * @param hat Hat index.
         * @return SDL hat position bitmask.
         */
        Uint8 getHat(int hat);

        /**
         * @brief Read the current axis value.
         * @param axis SDL_GameControllerAxis constant.
         * @return Axis value in the range [-32768, 32767].
         */
        Sint16 getAxis(SDL_GameControllerAxis axis);

        /**
         * @brief Return a Wrapper around the SDL_GameController handle.
         * @return Wrapper holding the pointer or nullopt.
         */
        mx::Wrapper<SDL_GameController *> wrapper() const {
            if (stick)
                return stick;
            return std::nullopt;
        }

        /**
         * @brief Check whether the controller is currently usable.
         * @return @c true if the controller is open and has a valid index.
         */
        bool active() const {
            if (index >= 0 && stick != nullptr)
                return true;
            return false;
        }

      protected:
        SDL_GameController *stick = nullptr; ///< Underlying SDL game controller handle.
        int index = -1;                      ///< Device index (-1 if not open).
    };
} // namespace mx

#endif