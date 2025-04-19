#include"exception.hpp"

namespace mx{
    Exception::Exception(const std::string &text_) : text_value{text_} {}
    std::string Exception::text() const { return text_value; }
}