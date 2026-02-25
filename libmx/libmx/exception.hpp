/**
 * @file exception.hpp
 * @brief Exception type used throughout libmx.
 */
#ifndef __MX_E__
#define __MX_E__

#include<iostream>
#include<string>

namespace mx {

/**
 * @class Exception
 * @brief General-purpose exception class for the mx library.
 *
 * Wraps a human-readable error message that can be retrieved via text().
 * Throw this instead of std::exception wherever a libmx error occurs.
 */
    class Exception {
    public:
        /**
         * @brief Construct an exception with the given message.
         * @param text_ Human-readable description of the error.
         */
        explicit Exception(const std::string &text_);

        /**
         * @brief Retrieve the error message.
         * @return The message string passed at construction.
         */
        std::string text() const;
    private:
        std::string text_value; ///< Stored error message.
    };
}

#endif
