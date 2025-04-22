#ifndef _TYPES_X__
#define _TYPES_X__
  
#include<iostream>
#include<vector>
#include<string>
#include<optional>

namespace types {
    enum class TokenType { TT_ID, TT_ARG, TT_SYM, TT_STR, TT_NUM, TT_NULL };
    enum class CharType { TT_CHAR, TT_DIGIT, TT_SYMBOL, TT_STRING, TT_SINGLE, TT_SPACE, TT_NULL };
    enum class OperatorType {
        OP_INC,             // '++'
        OP_DEC,             // '--'
        OP_LSHIFT_ASSIGN,   // '<<='
        OP_RSHIFT_ASSIGN,   // '>>='
        OP_PLUS_ASSIGN,     // '+='
        OP_MINUS_ASSIGN,    // '-='
        OP_MUL_ASSIGN,      // '*='
        OP_DIV_ASSIGN,      // '/='
        OP_MOD_ASSIGN,      // '%='
        OP_AND_ASSIGN,      // '&='
        OP_OR_ASSIGN,       // '|='
        OP_XOR_ASSIGN,      // '^='
        OP_LSHIFT,          // '<<'
        OP_RSHIFT,          // '>>'
        OP_EQ,              // '=='
        OP_NEQ,             // '!='
        OP_LE,              // '<='
        OP_GE,              // '>='
        OP_AND_AND,         // '&&'
        OP_OR_OR,           // '||'
        OP_ARROW,           // '->'
        OP_PLUS,            // '+'
        OP_MINUS,           // '-'
        OP_MUL,             // '*'
        OP_DIV,             // '/'
        OP_MOD,             // '%'
        OP_ASSIGN,          // '='
        OP_AND,             // '&'
        OP_OR,              // '|'
        OP_XOR,             // '^'
        OP_NOT,             // '!'
        OP_LT,              // '<'
        OP_GT,              // '>'
        OP_LPAREN,          // '('
        OP_RPAREN,          // ')'
        OP_LBRACKET,        // '['
        OP_RBRACKET,        // ']'
        OP_LBRACE,          // '{'
        OP_RBRACE,          // '}'
        OP_COMMA,           // ','
        OP_SEMICOLON,       // ';'
        OP_COLON,           // ':'
        OP_DOT,             // '.'
        OP_QUESTION,        // '?'
        OP_HASH,            // '#' 
        OP_SCOPE,           // '::' 
        OP_TILDE,            // '~'q
        OP_DOLLAR,          // '$'
        OP_AT,              // '@'
    };

    inline std::vector<std::string> opName = {
        "Increment",           // OP_INC             -> '++'
        "Decrement",           // OP_DEC             -> '--'
        "Left Shift Assign",   // OP_LSHIFT_ASSIGN   -> '<<='
        "Right Shift Assign",  // OP_RSHIFT_ASSIGN   -> '>>='
        "Plus Assign",         // OP_PLUS_ASSIGN     -> '+='
        "Minus Assign",        // OP_MINUS_ASSIGN    -> '-='
        "Multiply Assign",     // OP_MUL_ASSIGN      -> '*='
        "Divide Assign",       // OP_DIV_ASSIGN      -> '/='
        "Modulo Assign",       // OP_MOD_ASSIGN      -> '%='
        "Bitwise And Assign",  // OP_AND_ASSIGN      -> '&='
        "Bitwise Or Assign",   // OP_OR_ASSIGN       -> '|='
        "Bitwise Xor Assign",  // OP_XOR_ASSIGN      -> '^='
        "Left Shift",          // OP_LSHIFT          -> '<<'
        "Right Shift",         // OP_RSHIFT          -> '>>'
        "Equal",               // OP_EQ              -> '=='
        "Not Equal",           // OP_NEQ             -> '!='
        "Less Than Or Equal",  // OP_LE              -> '<='
        "Greater Than Or Equal", // OP_GE            -> '>='
        "Logical And",         // OP_AND_AND         -> '&&'
        "Logical Or",          // OP_OR_OR           -> '||'
        "Arrow",               // OP_ARROW           -> '->'
        "Plus",                // OP_PLUS            -> '+'
        "Minus",               // OP_MINUS           -> '-'
        "Multiply",            // OP_MUL             -> '*'
        "Divide",              // OP_DIV             -> '/'
        "Modulo",              // OP_MOD             -> '%'
        "Assign",              // OP_ASSIGN          -> '='
        "Bitwise And",         // OP_AND             -> '&'
        "Bitwise Or",          // OP_OR              -> '|'
        "Bitwise Xor",         // OP_XOR             -> '^'
        "Not",                 // OP_NOT             -> '!'
        "Less Than",           // OP_LT              -> '<'
        "Greater Than",        // OP_GT              -> '>'
        "Left Parenthesis",    // OP_LPAREN          -> '('
        "Right Parenthesis",   // OP_RPAREN          -> ')'
        "Left Bracket",        // OP_LBRACKET        -> '['
        "Right Bracket",       // OP_RBRACKET        -> ']'
        "Left Brace",          // OP_LBRACE          -> '{'
        "Right Brace",         // OP_RBRACE          -> '}'
        "Comma",               // OP_COMMA           -> ','
        "Semicolon",           // OP_SEMICOLON       -> ';'
        "Colon",               // OP_COLON           -> ':'
        "Dot",                 // OP_DOT             -> '.'
        "Question Mark",       // OP_QUESTION        -> '?'
        "Hash",                // OP_HASH            -> '#'
        "Scope",               // OP_SCOPE           -> '::'
        "Tilde"                // OP_TILDE           -> '~'
        "Dollar",              // OP_DOLLAR          -> '$'
        "AT",                  // OP_AT              -> '@'
    };

    inline std::vector<std::string> opStrings = {
        "++",       // OP_INC
        "--",       // OP_DEC
        "<<=",      // OP_LSHIFT_ASSIGN
        ">>=",      // OP_RSHIFT_ASSIGN
        "+=",       // OP_PLUS_ASSIGN
        "-=",       // OP_MINUS_ASSIGN
        "*=",       // OP_MUL_ASSIGN
        "/=",       // OP_DIV_ASSIGN
        "%=",       // OP_MOD_ASSIGN
        "&=",       // OP_AND_ASSIGN
        "|=",       // OP_OR_ASSIGN
        "^=",       // OP_XOR_ASSIGN
        "<<",       // OP_LSHIFT
        ">>",       // OP_RSHIFT
        "==",       // OP_EQ
        "!=",       // OP_NEQ
        "<=",       // OP_LE
        ">=",       // OP_GE
        "&&",       // OP_AND_AND
        "||",       // OP_OR_OR
        "->",       // OP_ARROW
        "+",        // OP_PLUS
        "-",        // OP_MINUS
        "*",        // OP_MUL
        "/",        // OP_DIV
        "%",        // OP_MOD
        "=",        // OP_ASSIGN
        "&",        // OP_AND
        "|",        // OP_OR
        "^",        // OP_XOR
        "!",        // OP_NOT
        "<",        // OP_LT
        ">",        // OP_GT
        "(",        // OP_LPAREN
        ")",        // OP_RPAREN
        "[",        // OP_LBRACKET
        "]",        // OP_RBRACKET
        "{",        // OP_LBRACE
        "}",        // OP_RBRACE
        ",",        // OP_COMMA
        ";",        // OP_SEMICOLON
        ":",        // OP_COLON
        ".",        // OP_DOT
        "?",        // OP_QUESTION
        "#",        // OP_HASH
        "::",
        "~",         // OP_TILDE
        "$",
        "@",

    };

    enum class KeywordType { KW_LET, KW_PROC, KW_DEFINE, KW_IF, KW_ELSE, KW_SWITCH, KW_WHILE, KW_FOR, KW_RETURN, KW_BREAK, KW_CONTINUE };
    inline std::vector<std::string> kwStr { "let", "proc", "define", "if", "else", "switch", "while", "for", "return", "break", "continue" };

    
    extern std::vector<std::string> strTokenType;
    extern std::vector<std::string> sstrCharType;
    void print_type_TokenType(std::ostream &out, const TokenType &tt);
    void print_type_CharType(std::ostream &out, const CharType &c);
    std::optional<OperatorType> lookUp(const std::string &op);
    std::optional<KeywordType> lookUp_Keyword(const std::string &op);
}

#endif