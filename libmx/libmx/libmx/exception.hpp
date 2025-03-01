#ifndef __MX_E__
#define __MX_E__

#include<iostream>
#include<string>


namespace mx {
    class Exception {
    public:
        explicit Exception(const std::string &text_);
        std::string text() const;
    private:
        std::string text_value;
    };
}

#endif
