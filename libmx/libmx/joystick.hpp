#ifndef __JOYSTICK__H__
#define __JOYSTICK__H__

#include"SDL.h"
#include<string>
#include<optional>

namespace mx {

    class Joystick {
    public:
        Joystick();
        ~Joystick();
        bool open(int index);
        std::string name() const;
        void close();
        std::optional<SDL_Joystick*> handle();
        SDL_Joystick *unwrap();
        int joystickIndex() const;

        static int joysticks() {
            return SDL_NumJoysticks();
        }

    protected:
        SDL_Joystick *stick = nullptr;
        int index = -1;
    };
}

#endif