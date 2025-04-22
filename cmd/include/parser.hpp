#ifndef _PARSER_X_H__P
#define _PARSER_X_H__P

#include"scanner.hpp"
#include"ast.hpp"
#include<memory>

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

            bool match(const std::string& symbol) {
                if (isAtEnd()) return false;
                if (peek().getTokenType() != types::TokenType::TT_SYM) return false;
                if (peek().getTokenValue() != symbol) return false;
                advance();
                return true;
            }

            std::shared_ptr<cmd::Node> parseVariableDefinition(const std::string& name) {
                advance(); 
                
                if (isAtEnd()) {
                    throw std::runtime_error("Expected value after '='");
                }
                
                
                std::shared_ptr<cmd::Node> valueNode;
                if (peek().getTokenType() == types::TokenType::TT_STR) {
                    valueNode = std::make_shared<cmd::StringLiteral>(advance().getTokenValue());
                } else if (peek().getTokenType() == types::TokenType::TT_NUM) {
                    valueNode = std::make_shared<cmd::NumberLiteral>(std::stod(advance().getTokenValue()));
                } else if (peek().getTokenType() == types::TokenType::TT_ID) {
                    std::string varRefName = advance().getTokenValue();
                    valueNode = std::make_shared<cmd::VariableReference>(varRefName);
                } else {
                    throw std::runtime_error("Expected string, number, or variable after '='");
                }
                
                return std::make_shared<cmd::VariableAssignment>(name, valueNode);
            }
            
            std::shared_ptr<cmd::Node> parseSequence() {
                std::vector<std::shared_ptr<cmd::Node>> commands;
                
                while (!isAtEnd()) {
                    commands.push_back(parsePipeline());
                }
                
                if (commands.size() == 1) {
                    return commands[0];
                }

                return std::make_shared<cmd::Sequence>(commands);
            }

            std::shared_ptr<cmd::Node> parsePipeline() {
                auto leftCmd = parseCommand();
                
                if (match("|")) {
                    std::vector<std::shared_ptr<cmd::Command>> commands;
                    if (auto cmd = std::dynamic_pointer_cast<cmd::Command>(leftCmd)) {
                        commands.push_back(cmd);
                    } else {
                        throw std::runtime_error("Expected command before pipe");
                    }
                    while (true) {
                        auto rightCmd = parseCommand();
                        if (auto cmd = std::dynamic_pointer_cast<cmd::Command>(rightCmd)) {
                            commands.push_back(cmd);
                        } else {
                            throw std::runtime_error("Expected command after pipe");
                        }
                        
                        if (!match("|")) {
                            break;
                        }
                    }
                    
                    return std::make_shared<cmd::Pipeline>(commands);
                } else if (match("&&")) {
                    auto rightCmd = parsePipeline(); 
                    return std::make_shared<cmd::LogicalAnd>(leftCmd, rightCmd);
                }
                
                return leftCmd;
            }

            std::shared_ptr<cmd::Node> parseCommand() {
                if (peek().getTokenType() != types::TokenType::TT_ID) {
                    throw std::runtime_error("Expected command name or let keyword");
                }
                
                std::string name = advance().getTokenValue();
                
                if (name == "let" && peek().getTokenType() == types::TokenType::TT_ID) {
                    std::string varName = advance().getTokenValue();
                    
                    if (!match("=")) {
                        throw std::runtime_error("Expected '=' after variable name in let statement");
                    }
                    
                    if (isAtEnd()) {
                        throw std::runtime_error("Expected value after '='");
                    }
                    
                    std::shared_ptr<cmd::Node> valueNode;
                    if (peek().getTokenType() == types::TokenType::TT_STR) {
                        valueNode = std::make_shared<cmd::StringLiteral>(advance().getTokenValue());
                    } else if (peek().getTokenType() == types::TokenType::TT_NUM) {
                        valueNode = std::make_shared<cmd::NumberLiteral>(std::stod(advance().getTokenValue()));
                    } else if (peek().getTokenType() == types::TokenType::TT_ID) {
                        std::string varRefName = advance().getTokenValue();
                        valueNode = std::make_shared<cmd::VariableReference>(varRefName);
                    } else {
                        throw std::runtime_error("Expected string, number, or variable after '='");
                    }
                    
                    return std::make_shared<cmd::VariableAssignment>(varName, valueNode);
                }
                
                std::vector<std::string> args;
                
                while (!isAtEnd() && 
                    (peek().getTokenType() == types::TokenType::TT_ID ||
                     peek().getTokenType() == types::TokenType::TT_STR ||
                     peek().getTokenType() == types::TokenType::TT_NUM ||
                     peek().getTokenType() == types::TokenType::TT_ARG ||  // Add this line
                     (peek().getTokenType() == types::TokenType::TT_SYM && 
                      peek().getTokenValue().size() > 0 &&
                      peek().getTokenValue()[0] == '-'))) {  
                    
                    auto token = advance();
                    std::string value = token.getTokenValue();
                    
                    if (name == "print" && token.getTokenType() == types::TokenType::TT_ID) {
                        args.push_back(std::string("${") + value + "}");
                    }
                    else if (token.getTokenType() == types::TokenType::TT_ID && value.size() > 1 && value[0] == '$') {
                        std::string varName = value.substr(1);
                        args.push_back(std::string("${") + varName + "}");
                    } else {
                        args.push_back(value);
                    }
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