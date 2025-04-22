#include "types.hpp"


namespace types {

    std::vector<std::string> strTokenType {"Identifier", "Symbols", "String", "Numeric", "NULL"};
    std::vector<std::string> strCharType  {"Characters", "Digits", "Symbols", "String", "Quote", "Space", "NULL"};

    void print_type_TokenType(std::ostream &out, const TokenType &tt) {
       unsigned int t_type = static_cast<unsigned int>(tt);
       if(t_type < strTokenType.size())
            out << strTokenType[t_type];
    }       

    void print_type_CharType(std::ostream &out, const CharType &c) {
        unsigned int t_type = static_cast<unsigned int>(c);
        if(t_type < strCharType.size())
           out << strCharType[t_type];
    }

    std::optional<OperatorType> lookUp(const std::string &op) {
        for(size_t i = 0; i < opStrings.size(); ++i) {
            if(opStrings[i] == op) 
                return static_cast<OperatorType>(i);
        }
        return std::nullopt;
    }

    std::optional<KeywordType> lookUp_Keyword(const std::string &op) {
        for(size_t i = 0; i < kwStr.size(); ++i) {
            if(kwStr[i] == op)
                return static_cast<KeywordType>(i);
        }
        return std::nullopt;
    }

}