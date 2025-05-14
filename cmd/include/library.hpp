#ifndef __LIBRARY_HPP_H_
#define __LIBRARY_HPP_H_

#include <string>
#include <stdexcept>
#include "3rdparty/dylib.hpp"

namespace cmd {
    class Library {
    public:
        Library(const std::string& path) : lib(path) {}
        template<typename FuncType>
        FuncType getFunction(const std::string& name) {
            return lib.get_function<FuncType>(name);
        }
        bool hasSymbol(const std::string& name) const {
            return lib.has_symbol(name);
        }
    private:
        dylib lib;
    };
}

#endif