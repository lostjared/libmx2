#ifndef __LIBRARY_HPP
#define __LIBRARY_HPP

#include "3rdparty/dylib.hpp"
#include <string>

namespace cmd {
    class Library {
      public:
        Library(const std::string &path)
            : lib(path.c_str(), useDecorations(path) ? dylib::add_filename_decorations : dylib::no_filename_decorations) {}
        ~Library() {}
        template <typename FuncType>
        FuncType getFunction(const std::string &name) {
            return reinterpret_cast<FuncType>(lib.get_symbol(name));
        }
        bool hasSymbol(const std::string &name) {
            return lib.has_symbol(name);
        }

      private:
        static bool endsWithInsensitive(const std::string &text, const std::string &suffix) {
            if (text.size() < suffix.size())
                return false;
            size_t offset = text.size() - suffix.size();
            for (size_t i = 0; i < suffix.size(); ++i) {
                char a = text[offset + i];
                char b = suffix[i];
                if (a >= 'A' && a <= 'Z')
                    a = static_cast<char>(a - 'A' + 'a');
                if (b >= 'A' && b <= 'Z')
                    b = static_cast<char>(b - 'A' + 'a');
                if (a != b)
                    return false;
            }
            return true;
        }

        static bool useDecorations(const std::string &path) {
            if (path.find('/') != std::string::npos || path.find('\\') != std::string::npos) {
                return false;
            }
            if (endsWithInsensitive(path, ".so") || endsWithInsensitive(path, ".dylib") || endsWithInsensitive(path, ".dll")) {
                return false;
            }
            return true;
        }

        dylib lib;
    };
} // namespace cmd

#endif