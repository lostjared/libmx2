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
        //throw mx::Exception("Invalid Joystick");
        return 0;
    }

    int Joystick::joystickIndex() const {
        return index;
    }

    bool Joystick::getButton(int button) {
        if(stick != nullptr && SDL_JoystickGetButton(stick, button))
            return true;

        return false;
    }

    Uint8 Joystick::getHat(int hat) {
        if(stick) {
            return SDL_JoystickGetHat(stick, hat);
        }
        return 0;
    }

    Sint16 Joystick::getAxis(int axis) {
        if(stick) {
            return SDL_JoystickGetAxis(stick, axis);
        }
        return 0;
    }

    int Joystick::numButtons() {
        if(stick)
            return SDL_JoystickNumButtons(stick);
        return 0;
    }

    int Joystick::numHats() {
        if(stick)
            return SDL_JoystickNumHats(stick);
        return 0;
    }
    int Joystick::numAxes() {
        if(stick) 
            return SDL_JoystickNumAxes(stick);
        return 0;
    }

    Controller::Controller() : stick{nullptr}, index{-1} {}
     
    Controller::~Controller() {
        close();
    }
    
    bool Controller::open(int index) {
        stick = SDL_GameControllerOpen(index);
        if(!stick) {
            this->index = -1;
            stick = nullptr;
            return false;
        }
        this->index = index;
        SDL_GameControllerEventState(SDL_ENABLE);
        return true;
    }
    
    std::string Controller::name() const {
        if(stick) {
            return SDL_GameControllerName(stick);
        }
        return "Controller Not Opened.\n";
    }
    
    void Controller::close() {
        if(stick)
            SDL_GameControllerClose(stick);
        
        stick = nullptr;
        index = -1;
    }
    
    std::optional<SDL_GameController *> Controller::handle() {
        if(stick)
            return stick; 

        return std::nullopt;
    }

    SDL_GameController *Controller::unwrap() const {    
        if(stick) {
            return stick;
        }
        //throw Exception("unwrap: Invalid Controller");
        return 0;
    }

    int Controller::controllerIndex() const {
        return index;
    }

    bool Controller::getButton(SDL_GameControllerButton button) {
        if(stick != nullptr && SDL_GameControllerGetButton(stick, button))
            return true;

        return false;
    }
    Uint8 Controller::getHat(int hat) {
        if (stick) {
            SDL_Joystick *joystick = SDL_GameControllerGetJoystick(stick);
            if (joystick) {
                return SDL_JoystickGetHat(joystick, hat);
            }
        }
        return 0;
    }

    Sint16 Controller::getAxis(SDL_GameControllerAxis axis) {
        if(stick) {
            return SDL_GameControllerGetAxis(stick, axis);
        }
        return 0;
    }

    bool Controller::connectEvent(SDL_Event &e) {
       if (e.type == SDL_CONTROLLERDEVICEADDED) {
            //std::cout << "Controller attached for Player " << e.cdevice.which << "\n";
            if (open(e.cdevice.which)) {
                //std::cout << "Connected Controller " << name() << "\n";
            } else {
                //std::cout << "Failed to open controller: " << SDL_GetError() << ".\n";
            }
            return true;
        }
        if (e.type == SDL_CONTROLLERDEVICEREMOVED) {
            //std::cout << "Controller disconnected.\n";
            close();
            return true;
        }
        return false;
    }
}