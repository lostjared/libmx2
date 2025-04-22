#ifndef _PARSER_X_H__P
#define _PARSER_X_H__P

#include"scanner.hpp"

namespace cmd {

    
    class Parser {
        public:
            Parser(scan::Scanner& scanner) : scanner(scanner), current(0) {}

            std::shared_ptr<cmd::Node> parse() {
                tokens_count = scanner.scan();
                return parseSequence();
            }

        private:
            scan::Scanner& scanner;
            uint64_t current;
            uint64_t tokens_count;

            bool isAtEnd() {
                return current >= tokens_count;
            }

            scan::token::Token<char> peek() {
                if (isAtEnd()) return scan::token::Token();
                return scanner[current];
            }

            scan::token::Token<char> advance() {
                if (!isAtEnd()) current++;
                return scanner[current - 1];
            }

            
            std::shared_ptr<cmd::Node> parseSequence() {
                std::vector<std::shared_ptr<cmd::Node>> commands;
                
                while (!isAtEnd()) {
                    commands.push_back(parsePipeline());
                    
            
                    if (peek().getTokenType() == types::TokenType::TT_SYM && 
                        peek().getTokenValue() == ";") {
                        advance(); 
                    } else {
                        break;
                    }
                }
                
                if (commands.size() == 1) {
                    return commands[0];
                }
                return std::make_shared<cmd::Sequence>(commands);
            }

            std::shared_ptr<cmd::Node> parsePipeline() {
                std::vector<std::shared_ptr<cmd::Command>> commands;
                
                commands.push_back(std::dynamic_pointer_cast<cmd::Command>(parseCommand()));
                
                while (peek().getTokenType() == types::TokenType::TT_SYM && 
                    peek().getTokenValue() == "|") {
                    advance(); // Consume pipe
                    commands.push_back(std::dynamic_pointer_cast<cmd::Command>(parseCommand()));
                }
                
                if (commands.size() == 1) {
                    return commands[0];
                }
                return std::make_shared<cmd::Pipeline>(commands);
            }


            std::shared_ptr<cmd::Node> parseCommand() {
                if (peek().getTokenType() != types::TokenType::TT_ID) {
                    throw std::runtime_error("Expected command name");
                }
                
                std::string name = advance().getTokenValue();
                std::vector<std::string> args;
                
                while (!isAtEnd() && 
                    (peek().getTokenType() == types::TokenType::TT_ID ||
                    peek().getTokenType() == types::TokenType::TT_STR ||
                    peek().getTokenType() == types::TokenType::TT_NUM)) {
                    args.push_back(advance().getTokenValue());
                }
                
                std::shared_ptr<cmd::Node> cmd = std::make_shared<cmd::Command>(name, args);
                
                while (peek().getTokenType() == types::TokenType::TT_SYM) {
                    std::string symbol = peek().getTokenValue();
                    if (symbol == "<") {
                        advance(); 
                        if (peek().getTokenType() != types::TokenType::TT_ID && 
                            peek().getTokenType() != types::TokenType::TT_STR) {
                            throw std::runtime_error("Expected filename after <");
                        }
                        std::string file = advance().getTokenValue();
                        cmd = std::make_shared<cmd::Redirection>(cmd, cmd::Redirection::INPUT, file);
                    } else if (symbol == ">") {
                        advance(); 
                        if (peek().getTokenType() != types::TokenType::TT_ID && 
                            peek().getTokenType() != types::TokenType::TT_STR) {
                            throw std::runtime_error("Expected filename after >");
                        }
                        std::string file = advance().getTokenValue();
                        cmd = std::make_shared<cmd::Redirection>(cmd, cmd::Redirection::OUTPUT, file);
                    } else if (symbol == ">>") {
                        advance(); 
                        if (peek().getTokenType() != types::TokenType::TT_ID && 
                            peek().getTokenType() != types::TokenType::TT_STR) {
                            throw std::runtime_error("Expected filename after >>");
                        }
                        std::string file = advance().getTokenValue();
                        cmd = std::make_shared<cmd::Redirection>(cmd, cmd::Redirection::APPEND, file);
                    } else {
                        break;
                    }
                }
                
                return cmd;
            }
        };    
}
#endif