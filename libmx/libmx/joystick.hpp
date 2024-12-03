#ifndef __JOYSTICK__H__
#define __JOYSTICK__H__

#include"SDL.h"
#include<string>
#include<optional>
#include"wrapper.hpp"

namespace mx {

    class Joystick {
    public:
        Joystick();
        ~Joystick();
        bool open(int index);
        std::string name() const;
        void close();
        std::optional<SDL_Joystick*> handle();
        SDL_Joystick *unwrap() const;
        int joystickIndex() const;
        static int joysticks() {
            return SDL_NumJoysticks();
        }
        int numButtons();
        int numHats();
        int numAxes();

        bool getButton(int button);
        Uint8 getHat(int hat);
        Sint16 getAxis(int axis);

        mx::Wrapper<SDL_Joystick *> wrapper() const {
            if(stick)
                return stick;
            return std::nullopt;
        }
    protected:
        SDL_Joystick *stick = nullptr;
        int index = -1;
    };

    class Controller {
    public:
        Controller();
        ~Controller();
        bool open(int index);
        bool connectEvent(SDL_Event &e);
        std::string name() const;
        void close();
        std::optional<SDL_GameController*> handle();
        SDL_GameController *unwrap() const;
        int controllerIndex() const;
        static int joysticks() {
            return SDL_NumJoysticks();
        }
        bool getButton(SDL_GameControllerButton button);
        Uint8 getHat(int hat);
        Sint16 getAxis(SDL_GameControllerAxis axis);
        mx::Wrapper<SDL_GameController *> wrapper() const {
            if(stick)
                return stick;
            return std::nullopt;
        }
        bool active() const { if(index >= 0 && stick != nullptr) return true; return false; }
    protected:
        SDL_GameController *stick = nullptr;
        int index = -1;
    };
}

#endif