#ifndef __WRAPPER_H__
#define __WRAPPER_H__

#include<iostream>
#include<optional>
#include "tee_stream.hpp"

namespace mx {

    template<typename T>
    class Wrapper {
    public:
        Wrapper() : type{std::nullopt} {}
        Wrapper(const T &t) : type{t} {}
        Wrapper(std::nullopt_t n) : type{n} {}
        Wrapper(const Wrapper<T> &w) : type{w.type} {}
        Wrapper(Wrapper<T> &&w) : type{std::move(w.type)} {}

        Wrapper<T> &operator=(const T &t) {
            this->type = t;
            return *this;
        }

        Wrapper<T> &operator=(std::nullopt_t n) {
            type = n;
            return *this;
        }
        
        Wrapper<T> &operator=(const Wrapper<T> &w) {
            this->type = w.type;
            return *this;
        }

        Wrapper<T> &operator=(Wrapper<T> &&w) {
            this->type = std::move(w.type);
            return *this;
        }

        bool has_value() const {
            if(type.has_value()) 
                return true;

            return false;
        }

        T value() {
            return type.value();
        }

        T expect(const std::string &msg) {
            if(type.has_value())
                return type.value();
            
            mx::system_err << "panic: " << msg << "\n";
            mx::system_err.flush();
            exit(EXIT_FAILURE);
        }

        T unwrap() {
            if(type.has_value())
                return type.value();

            mx::system_err << "mx: Error type is null...\n";
            mx::system_err.flush();
            return T();
        }
    private:
        std::optional<T> type;
    };



}






#endif