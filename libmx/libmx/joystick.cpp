#include"joystick.hpp"
#include<optional>
#include"tee_stream.hpp"

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
        SDL_JoystickEventState(SDL_ENABLE);
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
    
    std::optional<SDL_Joystick *> Joystick::handle() {
        if(stick)
            return stick; 

        return std::nullopt;
    }

    SDL_Joystick *Joystick::unwrap() const {    
        if(stick) {
            return stick;
        }
        mx::system_err << "mx: panic Invalid Joystick.\n";
        mx::system_err.flush();
        exit(EXIT_FAILURE);
        return 0;
    }

    int Joystick::joystickIndex() const {
        return index;
    }

}