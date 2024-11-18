#ifndef __JOYSTICK__H__
#define __JOYSTICK__H__

#include"SDL.h"
#include<string>
namespace mx {

    class Joystick {
    public:
        Joystick();
        ~Joystick();
        bool open(int index);
        std::string name() const;
        void close();
        SDL_Joystick *handle();
        int joystickIndex() const;
    protected:
        SDL_Joystick *stick = nullptr;
        int index = -1;
    };
}

#endif