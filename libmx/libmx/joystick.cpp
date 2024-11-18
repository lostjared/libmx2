#include"joystick.hpp"


namespace mx {

    Joystick::Joystick() : stick{nullptr}, index{-1} {}
     
    Joystick::~Joystick() {
        close();
    }
    
    bool Joystick::open(int index) {
        stick = SDL_JoystickOpen(index);
        if(!stick) {
            this->index = -1;
            stick = nullptr;
            return false;
        }
        this->index = index;
        return true;
    }
    
    std::string Joystick::name() const {
        if(stick) {
            return SDL_JoystickName(stick);
        }
        return "Joystick Not Opened.\n";
    }
    
    void Joystick::close() {
        if(stick)
            SDL_JoystickClose(stick);
        
        stick = nullptr;
        index = -1;
    }
    
    SDL_Joystick *Joystick::handle() { return stick; }

    int Joystick::joystickIndex() const {
        return index;
    }

}