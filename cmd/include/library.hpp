#ifndef __LIBRARY_HPP
#define __LIBRARY_HPP

#include <string>
#include "3rdparty/dylib.hpp"

namespace cmd {
    class Library {
    public:
        Library(const std::string& path) : lib(path) {}
        ~Library() {}

        template<typename FuncType>
        FuncType getFunction(const std::string& name) {
            return reinterpret_cast<FuncType>(lib.get_symbol(name));
        }

        bool hasSymbol(const std::string& name) {
            return lib.has_symbol(name);
        }

    private:
        dylib lib;
    };
}

#endif