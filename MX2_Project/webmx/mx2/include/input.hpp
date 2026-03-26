/**
 * @file input.hpp
 * @brief High-level game controller input abstraction built on Controller.
 *
 * Input extends Controller to offer a named-button query API using the
 * Input_Button enum, mapping logical buttons to SDL_GameControllerButton values.
 */
#ifndef __INPUT__H__
#define __INPUT__H__

#include "joystick.hpp"

namespace mx {

    /**
     * @enum Input_Button
     * @brief Logical button identifiers for a standard gamepad.
     */
    enum class Input_Button {
        BTN_A = 0,   ///< A / Cross button.
        BTN_B,       ///< B / Circle button.
        BTN_X,       ///< X / Square button.
        BTN_Y,       ///< Y / Triangle button.
        BTNL_L,      ///< Left shoulder button.
        BTN_R,       ///< Right shoulder button.
        BTN_D_UP,    ///< D-pad up.
        BTN_D_DOWN,  ///< D-pad down.
        BTN_D_LEFT,  ///< D-pad left.
        BTN_D_RIGHT, ///< D-pad right.
        BTN_START,   ///< Start / Menu button.
        BTN_BACK,    ///< Back / Select button.
    };

    /**
     * @class Input
     * @brief Convenience wrapper that queries controller buttons by logical name.
     *
     * Inherits Controller, adding getButton(Input_Button) to allow polling
     * gamepad input without knowing the underlying SDL button codes.
     */
    class Input : public Controller {
      public:
        /** @brief Default constructor. */
        Input() = default;
        ~Input() = default;

        Input(const Input &) = delete;
        Input &operator=(const Input &) = delete;
        Input(Input &&) = delete;
        Input &operator=(Input &&) = delete;

        /**
         * @brief Query whether a logical gamepad button is currently pressed.
         * @param b Logical button identifier.
         * @return @c true if the button is held down.
         */
        bool getButton(Input_Button b);
    };

} // namespace mx

#endif
