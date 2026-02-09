#include"input.hpp"

namespace mx {
    static const int keys[] = { 
        SDL_SCANCODE_A, SDL_SCANCODE_S, SDL_SCANCODE_Z, SDL_SCANCODE_X, 
        SDL_SCANCODE_Q, SDL_SCANCODE_W, SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, 
        SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_RETURN, SDL_SCANCODE_ESCAPE
    };

    static const SDL_GameControllerButton ctrl[] = { 
        SDL_CONTROLLER_BUTTON_A, SDL_CONTROLLER_BUTTON_B, SDL_CONTROLLER_BUTTON_X, SDL_CONTROLLER_BUTTON_Y, 
        SDL_CONTROLLER_BUTTON_LEFTSHOULDER, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, 
        SDL_CONTROLLER_BUTTON_DPAD_UP, SDL_CONTROLLER_BUTTON_DPAD_DOWN, 
        SDL_CONTROLLER_BUTTON_DPAD_LEFT, SDL_CONTROLLER_BUTTON_DPAD_RIGHT, 
        SDL_CONTROLLER_BUTTON_START, SDL_CONTROLLER_BUTTON_BACK
    };

    bool Input::getButton(Input_Button b) {
        const Uint8 *keystate = SDL_GetKeyboardState(0);
        int btn = static_cast<int>(b);
        if(keystate[keys[btn]]) {
            return true;
        }
        if(active() && Controller::getButton(ctrl[btn])) {
            return true;
        }
        return false;
    }
}