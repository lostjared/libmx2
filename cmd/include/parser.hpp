#ifndef _PARSER_X_H__P
#define _PARSER_X_H__P

#include"scanner.hpp"
#include"ast.hpp"
#include<memory>
#include<iostream> 

namespace cmd {
    
    class Parser {
        public:
            Parser(scan::Scanner& scanner) : scanner(scanner), current(0) {}

            std::shared_ptr<cmd::Node> parse() {
                tokens_count = scanner.scan();
                return parseSequence();
            }

            void dumpTokens() {
                std::cout << "Tokens: " << tokens_count << std::endl;
                for (uint64_t i = 0; i < tokens_count; i++) {
                    auto token = scanner[i];
                    std::cout << i << ": Type=" << static_cast<int>(token.getTokenType()) 
                              << " Value='" << token.getTokenValue() << "'" << std::endl;
                }
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

            std::shared_ptr<cmd::Expression> parseExpression() {
                auto expr = parseTerm();
                
                while (peek().getTokenType() == types::TokenType::TT_SYM && 
                       (peek().getTokenValue() == "+" || peek().getTokenValue() == "-")) {
                    std::string op = advance().getTokenValue();
                    auto right = parseTerm();
                    
                    if (op == "+") {
                        expr = std::make_shared<cmd::BinaryExpression>(
                            expr, cmd::BinaryExpression::ADD, right);
                    } else { 
                        expr = std::make_shared<cmd::BinaryExpression>(
                            expr, cmd::BinaryExpression::SUBTRACT, right);
                    }
                }
                
                return expr;
            }
            
            std::shared_ptr<cmd::Expression> parseTerm() {
                auto expr = parseFactor();
                
                while (peek().getTokenType() == types::TokenType::TT_SYM && 
                       (peek().getTokenValue() == "*" || peek().getTokenValue() == "/" || 
                        peek().getTokenValue() == "%")) {
                    std::string op = advance().getTokenValue();
                    auto right = parseFactor();
                    
                    if (op == "*") {
                        expr = std::make_shared<cmd::BinaryExpression>(
                            expr, cmd::BinaryExpression::MULTIPLY, right);
                    } else if (op == "/") {
                        expr = std::make_shared<cmd::BinaryExpression>(
                            expr, cmd::BinaryExpression::DIVIDE, right);
                    } else { 
                        expr = std::make_shared<cmd::BinaryExpression>(
                            expr, cmd::BinaryExpression::MODULO, right);
                    }
                }
                
                return expr;
            }
            
            std::shared_ptr<cmd::Expression> parseFactor() {
                if (peek().getTokenType() == types::TokenType::TT_SYM) {
                    if (peek().getTokenValue() == "++" || peek().getTokenValue() == "--") {
                        std::string op = advance().getTokenValue();
                        auto expr = parsePrimary();
                        
                        if (op == "++") {
                            return std::make_shared<cmd::UnaryExpression>(
                                expr, cmd::UnaryExpression::INCREMENT, cmd::UnaryExpression::PREFIX);
                        } else { 
                            return std::make_shared<cmd::UnaryExpression>(
                                expr, cmd::UnaryExpression::DECREMENT, cmd::UnaryExpression::PREFIX);
                        }
                    } else if (peek().getTokenValue() == "-") {
                        advance(); 
                        auto expr = parseFactor();
                        return std::make_shared<cmd::UnaryExpression>(
                            expr, cmd::UnaryExpression::NEGATE);
                    }
                }
                
                auto expr = parsePrimary();
                
                
                if (peek().getTokenType() == types::TokenType::TT_SYM) {
                    if (peek().getTokenValue() == "++" || peek().getTokenValue() == "--") {
                        std::string op = advance().getTokenValue();
                        
                        if (op == "++") {
                            return std::make_shared<cmd::UnaryExpression>(
                                expr, cmd::UnaryExpression::INCREMENT, cmd::UnaryExpression::POSTFIX);
                        } else { 
                            return std::make_shared<cmd::UnaryExpression>(
                                expr, cmd::UnaryExpression::DECREMENT, cmd::UnaryExpression::POSTFIX);
                        }
                    }
                }
                
                return expr;
            }
            
            
            std::shared_ptr<cmd::Expression> parsePrimary() {
                if (peek().getTokenType() == types::TokenType::TT_NUM) {
                    double value = std::stod(advance().getTokenValue());
                    return std::make_shared<cmd::NumberLiteral>(value);
                }
                
                if (peek().getTokenType() == types::TokenType::TT_ID) {
                    std::string name = advance().getTokenValue();
                    return std::make_shared<cmd::VariableReference>(name);
                }
                
                if (match("(")) {
                    auto expr = parseExpression();
                    if (!match(")")) {
                        throw std::runtime_error("Expected ')' after expression");
                    }
                    return expr;
                }
                
                throw std::runtime_error("Expected expression");
            }

            std::shared_ptr<cmd::Node> parseVariableDefinition(const std::string& name) {
                advance(); 
                
                if (isAtEnd()) {
                    throw std::runtime_error("Expected value after '='");
                }
            
                if (peek().getTokenType() == types::TokenType::TT_STR) {
                    auto valueNode = std::make_shared<cmd::StringLiteral>(advance().getTokenValue());
                    return std::make_shared<cmd::VariableAssignment>(name, valueNode);
                } else {
                    auto expr = parseExpression();
                    return std::make_shared<cmd::VariableAssignment>(name, expr);
                }
            }
            
            std::shared_ptr<cmd::Node> parseSequence() {
                std::vector<std::shared_ptr<cmd::Node>> commands;
                
                while (!isAtEnd()) {
                    commands.push_back(parsePipeline());
                    match(";"); 
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
                    uint64_t savedPos = current;
                    auto rightCmd = parsePipeline();
                    if (savedPos == current) {
                        throw std::runtime_error("Recursive parsing error in logical AND");
                    }
                    return std::make_shared<cmd::LogicalAnd>(leftCmd, rightCmd);
                }
                
                return leftCmd;
            }

            std::shared_ptr<cmd::Node> parseCommand() {
                if (peek().getTokenType() != types::TokenType::TT_ID) {
                    throw std::runtime_error("Expected command name, let keyword, or variable name");
                }
                
                uint64_t savedPosition = current;
                std::string name = advance().getTokenValue();
                
                if (peek().getTokenType() == types::TokenType::TT_SYM && peek().getTokenValue() == "=") {
                    advance(); 
                    

                    while (!isAtEnd() && peek().getTokenType() == types::TokenType::TT_SYM && peek().getTokenValue() == "") {
                        advance();
                    }
                    if (peek().getTokenType() == types::TokenType::TT_SYM && peek().getTokenValue() == "$") {
                        advance(); 
                        auto cmdNode = parseCommandSubstitution();
                        return std::make_shared<cmd::VariableAssignment>(
                            name, std::make_shared<cmd::CommandSubstitution>(cmdNode));
                    }
                     
                    if (peek().getTokenType() == types::TokenType::TT_STR) {
                        auto valueNode = std::make_shared<cmd::StringLiteral>(advance().getTokenValue());
                        return std::make_shared<cmd::VariableAssignment>(name, valueNode);
                    } else {
                        auto expr = parseExpression();
                        return std::make_shared<cmd::VariableAssignment>(name, expr);
                    }
                }
                
                
                current = savedPosition;
                
                if (peek().getTokenType() == types::TokenType::TT_ID) {
                    name = advance().getTokenValue();
                    
                    if (name == "let" && peek().getTokenType() == types::TokenType::TT_ID) {
                        std::string varName = advance().getTokenValue();
                        
                        if (match("=")) {
                            while (!isAtEnd() && peek().getTokenType() == types::TokenType::TT_SYM && peek().getTokenValue() == "") {
                                advance();
                            }
                            if (peek().getTokenType() == types::TokenType::TT_SYM && peek().getTokenValue() == "$") {
                                advance(); 
                                auto cmdNode = parseCommandSubstitution();
                                return std::make_shared<cmd::VariableAssignment>(
                                    varName, std::make_shared<cmd::CommandSubstitution>(cmdNode));
                            }

                            if (peek().getTokenType() == types::TokenType::TT_STR) {
                                auto valueNode = std::make_shared<cmd::StringLiteral>(advance().getTokenValue());
                                return std::make_shared<cmd::VariableAssignment>(varName, valueNode);
                            }
                            
                            auto expr = parseExpression();
                            return std::make_shared<cmd::VariableAssignment>(varName, expr);
                        }
                    }
                }
                
                std::vector<std::string> args;
                while (!isAtEnd() && 
                       peek().getTokenType() != types::TokenType::TT_SYM && 
                       peek().getTokenValue() != ";" && 
                       peek().getTokenValue() != "|" && 
                       peek().getTokenValue() != ">" && 
                       peek().getTokenValue() != ">>" &&
                       peek().getTokenValue() != "<" &&
                       peek().getTokenValue() != "&&") {
                    
                    if (peek().getTokenType() == types::TokenType::TT_ID || 
                        peek().getTokenType() == types::TokenType::TT_STR || 
                        peek().getTokenType() == types::TokenType::TT_NUM) {
                        args.push_back(advance().getTokenValue());
                    } else {
                        if (peek().getTokenType() == types::TokenType::TT_SYM && peek().getTokenValue() == "$") {
                            advance();
                            if (match("(")) {
                                throw std::runtime_error("Command substitution in arguments not yet supported");
                                
                            } else {
                                
                                args.push_back("$" + advance().getTokenValue());
                            }
                        } else {
                            advance();
                        }
                    }
                }
                

                auto cmd = std::make_shared<cmd::Command>(name, args);
                
                if (!isAtEnd()) {
                    if (peek().getTokenValue() == ">") {
                        advance(); 
                        if (isAtEnd() || peek().getTokenType() != types::TokenType::TT_ID &&
                                         peek().getTokenType() != types::TokenType::TT_STR) {
                            throw std::runtime_error("Expected filename after '>'");
                        }
                        std::string filename = advance().getTokenValue();
                        return std::make_shared<cmd::Redirection>(cmd, cmd::Redirection::OUTPUT, filename);
                    } 
                    else if (peek().getTokenValue() == ">>") {
                        advance(); 
                        if (isAtEnd() || peek().getTokenType() != types::TokenType::TT_ID &&
                                         peek().getTokenType() != types::TokenType::TT_STR) {
                            throw std::runtime_error("Expected filename after '>>'");
                        }
                        std::string filename = advance().getTokenValue();
                        return std::make_shared<cmd::Redirection>(cmd, cmd::Redirection::APPEND, filename);
                    }
                    else if (peek().getTokenValue() == "<") {
                        advance(); 
                        if (isAtEnd() || peek().getTokenType() != types::TokenType::TT_ID &&
                                         peek().getTokenType() != types::TokenType::TT_STR) {
                            throw std::runtime_error("Expected filename after '<'");
                        }
                        std::string filename = advance().getTokenValue();
                        return std::make_shared<cmd::Redirection>(cmd, cmd::Redirection::INPUT, filename);
                    }
                }
                
                return cmd;
            }

            std::shared_ptr<cmd::Node> parseCommandSubstitution() {
                if (!match("(")) {
                    throw std::runtime_error("Expected '(' after '$' for command substitution");
                }
                
                
                uint64_t startPos = current;
                
                
                if (peek().getTokenType() == types::TokenType::TT_ID) {
                    std::string cmdName = advance().getTokenValue();
                    std::vector<std::string> args;
                    
                    while (!isAtEnd() && 
                           !(peek().getTokenType() == types::TokenType::TT_SYM && peek().getTokenValue() == ")")) {
                        if (peek().getTokenType() == types::TokenType::TT_ID || 
                            peek().getTokenType() == types::TokenType::TT_STR || 
                            peek().getTokenType() == types::TokenType::TT_NUM) {
                            args.push_back(advance().getTokenValue());
                        } else {
                            advance(); 
                        }
                    }
                    
                    if (!match(")")) {
                        throw std::runtime_error("Expected ')' to close command substitution");
                    }
                    
                    return std::make_shared<cmd::Command>(cmdName, args);
                }
                
                if (startPos == current) {
                    throw std::runtime_error("Parser not making progress in command substitution");
                }
                
                throw std::runtime_error("Expected command name after $(");
            }
        };     
}
#endif