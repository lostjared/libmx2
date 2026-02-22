#ifndef __WRAPPER_H__
#define __WRAPPER_H__

#include<iostream>
#include<optional>
#include "tee_stream.hpp"
#include<sstream>
#include"exception.hpp"

namespace mx {

    template <typename T>
    concept WrapType = std::is_pointer_v<T>;

    template<WrapType T>
    class Wrapper {
    public:
        Wrapper() : type{std::nullopt} {}
        Wrapper(const T &t) : type{t} {}
        Wrapper(std::nullopt_t n) : type{n} {}
        Wrapper(const Wrapper<T> &w) : type{w.type} {}
        Wrapper(Wrapper<T> &&w) : type{std::move(w.type)} {}
        ~Wrapper() = default;

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
            if(type.has_value() && type.value() != nullptr)
                return type.value();

            std::ostringstream stream; 
            stream << "panic: " << msg;
            throw Exception(stream.str());
        }

        T unwrap() {
            if(type.has_value() && type.value() != nullptr)
                return type.value();

            std::ostringstream stream;
            stream << "mx: panic, Wrapper Error: type is null...";
            throw Exception(stream.str());
            return T();
        }

        T unwrap_or(T value) {
            if(type.has_value() && type.value() != nullptr)
                return type.value();
            return value;
        }

    private:
        std::optional<T> type;
    };
}

#endif