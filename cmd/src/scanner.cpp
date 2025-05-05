#include"scanner.hpp"
#include<set>

namespace scan {

    Scanner::Scanner(const TString &b) : string_buffer{b} {}

    uint64_t Scanner::parseLineNumber() {
        std::string lineNumberStr;
        while (true) {
                auto ch = string_buffer.getch();
                if (!ch.has_value()) {
                    return 1;  
                }
                if (isdigit(*ch)) {
                    lineNumberStr += *ch;
                    break;  
                }
        }
        while (true) {
            auto ch = string_buffer.getch();
            if (!ch.has_value() || !isdigit(*ch)) {
                break; 
            }
            lineNumberStr += *ch;
        }
        return !lineNumberStr.empty() ? std::stoull(lineNumberStr) : 1;
    }

    std::string Scanner::parseFileName() {
        std::string fileName;
        while (true) {
            auto ch = string_buffer.getch();
            if (!ch.has_value() || !isspace(*ch)) {
                if (ch.has_value() && *ch == '\"') break; 
            }
        }
        while (true) {
            auto ch = string_buffer.getch();
            if (!ch.has_value() || *ch == '\"') break; 
            fileName += *ch;
        }
        return fileName.empty() ? "unknown" : fileName; 
    }

    uint64_t Scanner::scan() {
        tokens.clear();
        while(!string_buffer.eof(0)) {
            auto ch = string_buffer.getch();
            if(!ch.has_value()) break;
            
            if (*ch == '\n') {
                TToken token;
                auto pos = string_buffer.cur_line();
                auto filename = string_buffer.cur_file();
                
                token.set_pos(pos);
                token.set_filename(filename);
                token.setToken(types::TokenType::TT_SYM, "\n");
                
                tokens.push_back(token);
                continue;
            }
            
            if(ch.has_value() && *ch == '#') { 
                auto next_char = string_buffer.peekch(1);
                if (next_char.has_value() && isdigit(*next_char)) {
                    uint64_t lineNumber = parseLineNumber();
                    std::string fileName = parseFileName();
                    string_buffer.process_line_directive(lineNumber, fileName);
                    do {
                        ch = string_buffer.getch();
                    } while(ch.has_value() && ch.value()!='\n');
                    string_buffer.currentLine -= 1;
                    continue;           
                } 
                else {
                    do {
                        ch = string_buffer.getch();
                    } while(ch.has_value() && *ch != '\n');
                    
                    if (ch.has_value()) {
                        string_buffer.backward_step(1);
                    }
                    continue;
                }
            } else if(ch.has_value() && *ch == '/') {
                auto ch2 = string_buffer.curch();
                if(ch2.has_value() && *ch2 == '/') {
                    auto ch_ln = string_buffer.curch();
                    do {
                        ch_ln = string_buffer.getch();    
                    } while(ch_ln.has_value() && *ch_ln != '\n');
                    continue;
                } else if(ch2.has_value() && *ch2 == '*') {
                    string_buffer.getch(); 
                    while(true) {
                        auto ch_in = string_buffer.getch();
                        if(!ch_in.has_value()) break;
                        if(*ch_in == '*') {
                            auto ch_ex = string_buffer.curch();
                            if(ch_ex.has_value() && *ch_ex == '/') {
                                string_buffer.getch(); 
                                break;
                            }
                        }
                    }
                    continue;
                }
            }
            if(ch.has_value()) {
                 auto t_ch = token_map.lookup_int8(*ch);
                if(t_ch.has_value()) {
                    switch(*t_ch) {
                        case types::CharType::TT_CHAR: {
                            auto tok = grabId();
                            if(tok.has_value()) {
                                tokens.push_back(*tok);
                            }
                        }
                        break;
                        case types::CharType::TT_DIGIT: {
                            auto tok = grabDigits();
                            if(tok.has_value()) {
                                tokens.push_back(*tok);
                            }
                        }
                        break;
                        case types::CharType::TT_SYMBOL: {
                            auto tok = grabSymbols();
                            if(tok.has_value()) {
                                tokens.push_back(*tok);
                            }
                        }
                        break;
                        case types::CharType::TT_STRING: {
                            auto tok = grabString();
                            if(tok.has_value()) {
                                tokens.push_back(*tok);
                            }
                        }
                        break;
                        case types::CharType::TT_SINGLE: {
                            auto tok = grabSingle();
                            if(tok.has_value())  {
                                tokens.push_back(*tok);
                            }
                        }
                        break;
                        case types::CharType::TT_SPACE:
                            while (true) {
                                auto next = string_buffer.peekch(0);
                                if(next.has_value() && *next == '\n') { string_buffer.getch();  continue; }
                                if (!next.has_value() || token_map.lookup_int8(*next) != types::CharType::TT_SPACE) {
                                    break;
                                }
                                string_buffer.getch(); 
                            }
                            continue;
                        case types::CharType::TT_NULL:
                            continue;
                    }
                }
            } else break;
        }

        return tokens.size();
    }

    std::optional<TToken> Scanner::grabId() {
        auto ch = string_buffer.backward_step(1);
        TToken token;
        auto filename = string_buffer.cur_file();
        auto pos = string_buffer.cur_line();
        if(ch.has_value()) {
            auto ch_t = token_map.lookup_int8(*ch);
            decltype(token.getTokenValue()) tok_value;
            while(true) {
                ch = string_buffer.getch();
                if(!ch.has_value()) break;
                ch_t = token_map.lookup_int8(*ch);
                if(!ch_t.has_value() || (*ch_t != types::CharType::TT_CHAR && *ch_t != types::CharType::TT_DIGIT)) break;
                tok_value += *ch;    
            }
            token.set_pos(pos);
            token.set_filename(filename);
            token.setToken(types::TokenType::TT_ID, tok_value);
            string_buffer.backward_step(1);
            return token;
        }
        return std::nullopt;
    }

    std::optional<TToken> Scanner::grabDigits() {
        auto ch = string_buffer.backward_step(1);
        TToken token;
        auto pos = string_buffer.cur_line();
        auto filename = string_buffer.cur_file();
        if(ch.has_value()) {
            auto ch_t = token_map.lookup_int8(*ch);
            decltype(token.getTokenValue()) tok_value;
            int decimal = 0;
            
            while(true) {
                ch = string_buffer.getch();
                if(!ch.has_value()) break;
                ch_t = token_map.lookup_int8(*ch);
                if(!ch_t.has_value() || (*ch_t != types::CharType::TT_DIGIT && *ch != '.')) break;
                if(*ch == '.') {
                    ++decimal;
                    if(decimal > 1) {
                        std::ostringstream stream;
                        auto lp = string_buffer.cur_line();
                        stream << "File: " << string_buffer.cur_file() << " Line: " << lp.first << " Col: " << lp.second << " -> Too may precision points for floating point number only one is allowed.";
                        throw ScanExcept(stream.str());
                    }
                }
                tok_value += *ch;    
            }

            string_buffer.backward_step(1);
            token.set_pos(pos);
            token.set_filename(filename);
            token.setToken(types::TokenType::TT_NUM, tok_value);
            return token;
        }
        return std::nullopt;
    }

    bool Scanner::is_c_sym(const StringType  &str) {
        static const std::set<std::string> c_op = {
            "++", "--", ">>=", "<<=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", 
            "==", "!=", ">=", "<=", ">>", "<<", "&&", "||", "->", ".", "&", "*", "+", 
            "-", "~", "!", "/", "%", ">", "<", "=", "^", "|", "?", ":", ";", ",", 
            "(", ")", "[", "]", "{", "}", "->", ".","::", "$"
        };
        return c_op.find(str) != c_op.end();
    }

    std::optional<TToken> Scanner::grabSymbols() {
        TToken token;
        std::string tok_value;
        string_buffer.backward_step(1);
        auto filename = string_buffer.cur_file();
        auto pos      = string_buffer.cur_line();
        
        auto ch = string_buffer.getch();
        if (!ch.has_value()) return std::nullopt;
        
        if (*ch == '-') {
            auto next = string_buffer.peekch(0);
            if (next.has_value() && *next == '-') {
                string_buffer.getch();
                token.set_pos(pos);
                token.set_filename(filename);
                token.setToken(types::TokenType::TT_ARG, "--"); 
                return token;
            }
            else {
                token.set_pos(pos);
                token.set_filename(filename);
                token.setToken(types::TokenType::TT_SYM, "-");
                return token;
            }
        }
        
        
        tok_value = *ch;
        std::string temp_value = tok_value;
        int look = 0;
        bool found = false;
        
        while (true) {
            auto temp = string_buffer.peekch(look);
            if (temp.has_value() && token_map.lookup_int8(*temp) == types::CharType::TT_SYMBOL) {
                temp_value += *temp;
                if (is_c_sym(temp_value)) {
                    tok_value = temp_value;
                    look++;
                    found = true;
                } else {
                    break;
                }
            } else {
                break;
            }
        }
        
        if (found && look != 0) {
            string_buffer.forward_step(look);
        }
        
        token.set_pos(pos);
        token.set_filename(filename);
        token.setToken(types::TokenType::TT_SYM, tok_value);
        return token;
    }
        
    std::optional<TToken> Scanner::grabString() {
            TToken token;
            std::string tok_value;
            auto pos = string_buffer.cur_line();
            auto filename = string_buffer.cur_file();
            while (true) {
                auto ch = string_buffer.getch();
                if (!ch.has_value()) {
                    std::ostringstream stream;
                    auto cpos = string_buffer.cur_line();
                    stream << "File: " << string_buffer.cur_file() << " Line: " << cpos.first << " Col:"<< cpos.second << " Double quote not terminated.";
                    throw ScanExcept(stream.str());
                } 
                if (*ch == '\\') {
                    auto next_ch = string_buffer.getch();
                    if (!next_ch.has_value()) break;
                    switch (*next_ch) {
                        case 'n': tok_value += '\n'; break;
                        case 't': tok_value += '\t'; break;
                        case 'r': tok_value += '\r'; break;
                        case '\\': tok_value += '\\'; break;
                        case '\"': tok_value += '\"'; break;
                        case '\'': tok_value += '\''; break;
                        default:
                            tok_value += '\\';
                            tok_value += *next_ch;
                            break;
                    }
                } else if (*ch == '\"') {
                    break;
                } else {
                    tok_value += *ch;
                }
            }

    
            token.set_pos(pos);
            token.set_filename(filename);
            token.setToken(types::TokenType::TT_STR, tok_value);
            return token;
        }

        std::optional<TToken> Scanner::grabSingle() {
            TToken token;
            std::string tok_value;
            auto filename = string_buffer.cur_file();
            auto pos = string_buffer.cur_line();
            while (true) {
                auto ch = string_buffer.getch();
                if (!ch.has_value()) {
                    std::ostringstream stream;
                    auto cpos = string_buffer.cur_line();
                    stream << "File: " << string_buffer.cur_file() << " Line: " << cpos.first << " Col:"<< cpos.second << " String quote not terminated.";
                    throw ScanExcept(stream.str());
                } 
                if (*ch == '\\') {
                    auto next_ch = string_buffer.getch();
                    if (!next_ch.has_value()) break;
                    switch (*next_ch) {
                        case 'n': tok_value += '\n'; break;
                        case 't': tok_value += '\t'; break;
                        case 'r': tok_value += '\r'; break;
                        case '\\': tok_value += '\\'; break;
                        case '\"': tok_value += '\"'; break;
                        case '\'': tok_value += '\''; break;
                        default:
                            tok_value += '\\';
                            tok_value += *next_ch;
                            break;
                }
            } else if (*ch == '\'') {
                    break;
            } else {
                tok_value += *ch;
            }
        }

        token.set_pos(pos);
        token.set_filename(filename);
        token.setToken(types::TokenType::TT_STR, tok_value);
        return token;
    }

    TToken &Scanner::operator[](size_t index) {         
        if(index < size()) return tokens[index];
        std::ostringstream stream;
        stream << "Token index: index out of bounds, Scanner Exception: " << index << "\n"; 
        throw ScanExcept(stream.str());

    }
    size_t Scanner::size() const { return tokens.size(); }
}
