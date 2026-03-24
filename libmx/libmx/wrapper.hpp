/**
 * @file wrapper.hpp
 * @brief Type-safe nullable pointer wrapper inspired by Rust's Option<T>.
 *
 * Wrapper<T> holds an optional pointer value and provides panic-on-null
 * accessors similar to Rust's unwrap() / expect() semantics.
 */
#ifndef __WRAPPER_H__
#define __WRAPPER_H__

#include "exception.hpp"
#include "tee_stream.hpp"
#include <format>
#include <iostream>
#include <optional>

namespace mx {

    /**
     * @concept WrapType
     * @brief Constrains Wrapper<T> to pointer types only.
     */
    template <typename T>
    concept WrapType = std::is_pointer_v<T>;

    /**
     * @class Wrapper
     * @brief A nullable smart wrapper around a raw pointer.
     *
     * Stores an optional pointer.  Accessors throw mx::Exception if the
     * pointer is null (unwrap, expect) or return a fallback (unwrap_or).
     *
     * @tparam T A raw pointer type (must satisfy WrapType concept).
     */
    template <WrapType T>
    class Wrapper {
      public:
        /** @brief Default constructor — initialises to nullopt (no value). */
        Wrapper() : type{std::nullopt} {}

        /**
         * @brief Construct from a raw pointer.
         * @param t Pointer to wrap.
         */
        Wrapper(const T &t) : type{t} {}

        /**
         * @brief Construct in the empty (nullopt) state.
         * @param n std::nullopt.
         */
        Wrapper(std::nullopt_t n) : type{n} {}

        /** @brief Copy constructor. */
        Wrapper(const Wrapper<T> &w) : type{w.type} {}

        /** @brief Move constructor. */
        Wrapper(Wrapper<T> &&w) : type{std::move(w.type)} {}

        ~Wrapper() = default;

        /**
         * @brief Assign from a raw pointer.
         * @param t Pointer to store.
         * @return Reference to this.
         */
        Wrapper<T> &operator=(const T &t) {
            this->type = t;
            return *this;
        }

        /**
         * @brief Reset to the empty (nullopt) state.
         * @param n std::nullopt.
         * @return Reference to this.
         */
        Wrapper<T> &operator=(std::nullopt_t n) {
            type = n;
            return *this;
        }

        /** @brief Copy-assign from another Wrapper. */
        Wrapper<T> &operator=(const Wrapper<T> &w) {
            this->type = w.type;
            return *this;
        }

        /** @brief Move-assign from another Wrapper. */
        Wrapper<T> &operator=(Wrapper<T> &&w) {
            this->type = std::move(w.type);
            return *this;
        }

        /**
         * @brief Check whether a non-null value is held.
         * @return @c true if a non-null pointer is stored.
         */
        bool has_value() const {
            if (type.has_value())
                return true;

            return false;
        }

        /**
         * @brief Access the stored pointer without null checking.
         * @return The stored pointer (may be nullptr if constructed from nullopt).
         */
        T value() {
            return type.value();
        }

        /**
         * @brief Unwrap with a custom panic message on null.
         * @param msg Message to include in the thrown mx::Exception.
         * @return The stored pointer.
         * @throws mx::Exception if the value is null or absent.
         */
        T expect(const std::string &msg) {
            if (type.has_value() && type.value() != nullptr)
                return type.value();

            throw Exception(std::format("panic: {}", msg));
        }

        /**
         * @brief Unwrap the stored pointer, panicking on null.
         * @return The stored pointer.
         * @throws mx::Exception if the value is null or absent.
         */
        T unwrap() {
            if (type.has_value() && type.value() != nullptr)
                return type.value();

            throw Exception("mx: panic, Wrapper Error: type is null...");
            return T();
        }

        /**
         * @brief Return the stored pointer or a fallback if null.
         * @param value Fallback pointer returned when no value is held.
         * @return The stored pointer, or @p value if null/absent.
         */
        T unwrap_or(T value) {
            if (type.has_value() && type.value() != nullptr)
                return type.value();
            return value;
        }

      private:
        std::optional<T> type; ///< Internal optional storage.
    };
} // namespace mx

#endif