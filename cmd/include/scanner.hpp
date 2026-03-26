#ifndef _SCANNER_H_
#define _SCANNER_H_

#include "string_buffer.hpp"
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace scan {

    using TString = buffer::StringBuffer<char>;
    using StringType = buffer::StringBuffer<char>::string_type;
    using TToken = token::Token<char>;
    using TMap = token::TokenMap<char>;

    class Scanner {
      public:
        Scanner(const TString &b);
        uint64_t scan();
        std::optional<TToken> grabId();
        std::optional<TToken> grabDigits();
        std::optional<TToken> grabSymbols();
        std::optional<TToken> grabString();
        std::optional<TToken> grabSingle();

        TToken &operator[](size_t index);
        size_t size() const;

      private:
        TString string_buffer;
        TMap token_map;
        std::vector<TToken> tokens;

        bool is_c_sym(const StringType &str);
        uint64_t parseLineNumber();
        std::string parseFileName();
    };

    class ScanExcept {
      public:
        ScanExcept() = default;
        ScanExcept(const StringType &why) : msg{why} {}
        StringType why() const { return msg; }

      private:
        StringType msg;
    };
} // namespace scan
std::ostream &operator<<(std::ostream &out, const types::CharType &c);
std::ostream &operator<<(std::ostream &out, const types::TokenType &t);

#endif