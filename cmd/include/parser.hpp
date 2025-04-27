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
                    auto rightCmd = parsePipeline(); 
                    return std::make_shared<cmd::LogicalAnd>(leftCmd, rightCmd);
                }
                
                return leftCmd;
            }

            std::shared_ptr<cmd::Node> parseCommand() {
                if (peek().getTokenType() != types::TokenType::TT_ID) {
                    throw std::runtime_error("Expected command name, let keyword, or variable name");
                }
                
                std::string name = advance().getTokenValue();
                
                
                if (peek().getTokenType() == types::TokenType::TT_SYM && peek().getTokenValue() == "=") {
                    return parseVariableDefinition(name);
                }
                
                if (name == "let" && peek().getTokenType() == types::TokenType::TT_ID) {
                    std::string varName = advance().getTokenValue();
                    
                    if (!match("=")) {
                        throw std::runtime_error("Expected '=' after variable name in let statement");
                    }
                    
                    if (peek().getTokenType() == types::TokenType::TT_STR) {
                        auto valueNode = std::make_shared<cmd::StringLiteral>(advance().getTokenValue());
                        return std::make_shared<cmd::VariableAssignment>(varName, valueNode);
                    } else {
                        auto expr = parseExpression();
                        return std::make_shared<cmd::VariableAssignment>(varName, expr);
                    }
                }
                
                std::vector<std::string> args;
                
                while (!isAtEnd() && 
                    (peek().getTokenType() == types::TokenType::TT_ID ||
                     peek().getTokenType() == types::TokenType::TT_STR ||
                     peek().getTokenType() == types::TokenType::TT_NUM ||
                     peek().getTokenType() == types::TokenType::TT_ARG ||
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