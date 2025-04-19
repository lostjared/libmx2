#ifndef __INPUT__H__
#define __INPUT__H__

#include "joystick.hpp"

namespace mx {


    enum class Input_Button {
        BTN_A=0, BTN_B, BTN_X, BTN_Y, BTNL_L, BTN_R,
        BTN_D_UP, BTN_D_DOWN, BTN_D_LEFT, BTN_D_RIGHT, BTN_START,
    };

    class Input : public Controller {
    public:
        Input() = default;
        bool getButton(Input_Button b);
    };

}


#endif


