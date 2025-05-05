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
                return parseScript();
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
                    }
                    else if (peek().getTokenValue() == "-") {    
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

            bool isVariableName(const std::string &text) {
                if (text.empty()) return false;
                if (!isalpha(text[0])) return false;
                for (size_t i = 1; i < text.size(); ++i) {
                    if (!isalnum(text[i]) && text[i] != '_') return false;
                }
                return true;    
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
                
                throw std::runtime_error("Expected expression: " + peek().getTokenValue() + " On Line: " + std::to_string(peek().get_pos().first));
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
                
                while (!isAtEnd() && 
                       !(peek().getTokenType() == types::TokenType::TT_SYM && peek().getTokenValue() == ")")) {
                    commands.push_back(parsePipeline());
                    match(";"); 
                }
                
                if (commands.size() == 1) {
                    return commands[0];
                }

                return std::make_shared<cmd::Sequence>(commands);
            }

            std::shared_ptr<cmd::Node> parsePipeline() {
                std::shared_ptr<cmd::Node> left = parseCommand();
                
                while (!isAtEnd() && peek().getTokenType() == types::TokenType::TT_SYM) {
                    if (peek().getTokenValue() == ">") {
                        advance(); 
                        
                        std::string filename;
                        if (peek().getTokenType() == types::TokenType::TT_ID ||
                            peek().getTokenType() == types::TokenType::TT_STR) {
                            filename = advance().getTokenValue();
                            
                            
                            if (filename.size() >= 2 && 
                                ((filename.front() == '"' && filename.back() == '"') || 
                                 (filename.front() == '\'' && filename.back() == '\''))) {
                                filename = filename.substr(1, filename.size() - 2);
                            }
                        } else {
                            throw std::runtime_error("Expected filename after '>' operator");
                        }
                        
                        
                        left = std::make_shared<cmd::Redirection>(left, cmd::Redirection::OUTPUT, filename);
                    }
                    else if (peek().getTokenValue() == ">>") {
                        advance(); 
                        std::string filename;
                        if (peek().getTokenType() == types::TokenType::TT_ID ||
                            peek().getTokenType() == types::TokenType::TT_STR) {
                            filename = advance().getTokenValue();
                        
                            if (filename.size() >= 2 && 
                                ((filename.front() == '"' && filename.back() == '"') || 
                                 (filename.front() == '\'' && filename.back() == '\''))) {
                                filename = filename.substr(1, filename.size() - 2);
                            }
                        } else {
                            throw std::runtime_error("Expected filename after '>>' operator");
                        }
                    
                        left = std::make_shared<cmd::Redirection>(left, cmd::Redirection::APPEND, filename);
                    }
                    else if (peek().getTokenValue() == "<") {
                        advance(); 
                    
                        std::string filename;
                        if (peek().getTokenType() == types::TokenType::TT_ID ||
                            peek().getTokenType() == types::TokenType::TT_STR) {
                            filename = advance().getTokenValue();
                            
                    
                            if (filename.size() >= 2 && 
                                ((filename.front() == '"' && filename.back() == '"') || 
                                 (filename.front() == '\'' && filename.back() == '\''))) {
                                filename = filename.substr(1, filename.size() - 2);
                            }
                        } else {
                            throw std::runtime_error("Expected filename after '<' operator");
                        }
                        
                    
                        left = std::make_shared<cmd::Redirection>(left, cmd::Redirection::INPUT, filename);
                    }
                    else if (peek().getTokenValue() == "|") {
                        advance(); 
                        auto right = parseCommand();
                        auto rightCmd = std::dynamic_pointer_cast<cmd::Command>(right);
                        if (!rightCmd) {
                            throw std::runtime_error("Expected command after '|' operator");
                        }
                        auto leftPipe = std::dynamic_pointer_cast<cmd::Pipeline>(left);
                        if (leftPipe) {
                            leftPipe->commands.push_back(rightCmd);
                            left = leftPipe;
                        } 
                        else {
                            auto leftCmd = std::dynamic_pointer_cast<cmd::Command>(left);
                            if (!leftCmd) {
                                throw std::runtime_error("Left side of pipe must be a command");
                            }
                            
                            std::vector<std::shared_ptr<cmd::Command>> commands;
                            commands.push_back(leftCmd);
                            commands.push_back(rightCmd);
                            left = std::make_shared<cmd::Pipeline>(std::move(commands));
                        }
                    }
                    else {
                        break;
                    }
                }
                
                return left;
            }

            std::shared_ptr<cmd::Node> parseCommand() {
                if (peek().getTokenType() != types::TokenType::TT_ID) {
                    throw std::runtime_error("Expected command name, let keyword, or variable name found: " + peek().getTokenValue() + " on Line: " + std::to_string(peek().get_pos().first));
                }
                
                if (peek().getTokenValue() == "if") {
                    return parseIfStatement();
                }
                
                uint64_t savedPosition = current;
                std::string name = advance().getTokenValue();
                bool isTestCommand = (name == "test" || name == "[");
                
                if (peek().getTokenType() == types::TokenType::TT_SYM && peek().getTokenValue() == "=") {
                    advance(); 
                    

                    while (!isAtEnd() && peek().getTokenType() == types::TokenType::TT_SYM && peek().getTokenValue() == "") {
                        advance();
                    }
                    if (peek().getTokenType() == types::TokenType::TT_SYM && peek().getTokenValue() == "$") {
                        advance(); 
                        auto cmdNode = parseCommandSubstitution();
                        auto cmdSubst = std::make_shared<cmd::CommandSubstitution>(cmdNode, true);
                        return std::make_shared<cmd::VariableAssignment>(name, cmdSubst);
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
                                    varName, std::make_shared<cmd::CommandSubstitution>(cmdNode, true));
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
                
                std::vector<Argument> args;
                while (!isAtEnd() && 
                       (peek().getTokenType() != types::TokenType::TT_SYM || peek().getTokenValue() == "-" ||
                        (isTestCommand && (peek().getTokenValue() == "=" || peek().getTokenValue() == "!=")) 
                       ) && 
                       peek().getTokenValue() != ";" && 
                       peek().getTokenValue() != "|" && 
                       peek().getTokenValue() != ">" && 
                       peek().getTokenValue() != ">>" &&
                       peek().getTokenValue() != "<" &&
                       peek().getTokenValue() != "&&") {
                    
                    if (isTestCommand && peek().getTokenType() == types::TokenType::TT_SYM && 
                        (peek().getTokenValue() == "=" || peek().getTokenValue() == "!=")) {
                        std::string value = advance().getTokenValue();
                        args.push_back({value, ARG_LITERAL});
                    }
                    else if ((peek().getTokenType() == types::TokenType::TT_SYM && peek().getTokenValue() == "-") ||
                             (peek().getTokenType() == types::TokenType::TT_ARG && peek().getTokenValue() == "--")) {
                        std::string value = advance().getTokenValue(); 
                        if (isTestCommand && peek().getTokenType() == types::TokenType::TT_ID) {
                            value += advance().getTokenValue(); 
                        }
                        args.push_back({value, ARG_LITERAL});
                    }
                    else if (peek().getTokenType() == types::TokenType::TT_ID) {
                        std::string value = advance().getTokenValue();
                        args.push_back({value, ARG_VARIABLE});
                    }
                    else if (peek().getTokenType() == types::TokenType::TT_STR) {
                        std::string value = advance().getTokenValue();
                        if (value.size() >= 2 && 
                            ((value.front() == '"' && value.back() == '"') || 
                             (value.front() == '\'' && value.back() == '\''))) {
                            value = value.substr(1, value.size() - 2);
                        }
                        args.push_back({value, ARG_LITERAL});
                    }
                    else if (peek().getTokenType() == types::TokenType::TT_NUM) {
                        std::string value = advance().getTokenValue();
                        args.push_back({value, ARG_LITERAL});
                    }
                    else if (peek().getTokenType() == types::TokenType::TT_ARG) {
                        std::string value = advance().getTokenValue();
                        args.push_back({value, ARG_LITERAL}); 
                    }
                    else {
                        if (peek().getTokenType() == types::TokenType::TT_SYM && peek().getTokenValue() == "$") {
                            advance();
                            if (match("(")) {
                                throw std::runtime_error("Command substitution in arguments not yet supported");
                            } else {
                                std::string varName = advance().getTokenValue();
                                args.push_back({"$" + varName, ARG_VARIABLE});
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
                        if (isAtEnd() || (peek().getTokenType() != types::TokenType::TT_ID &&
                                         peek().getTokenType() != types::TokenType::TT_STR)) {
                            throw std::runtime_error("Expected filename after '>'");
                        }
                        std::string filename = advance().getTokenValue();
                        return std::make_shared<cmd::Redirection>(cmd, cmd::Redirection::OUTPUT, filename);
                    } 
                    else if (peek().getTokenValue() == ">>") {
                        advance(); 
                        if (isAtEnd() || (peek().getTokenType() != types::TokenType::TT_ID &&
                                         peek().getTokenType() != types::TokenType::TT_STR)) {
                            throw std::runtime_error("Expected filename after '>>'");
                        }
                        std::string filename = advance().getTokenValue();
                        return std::make_shared<cmd::Redirection>(cmd, cmd::Redirection::APPEND, filename);
                    }
                    else if (peek().getTokenValue() == "<") {
                        advance(); 
                        if (isAtEnd() || (peek().getTokenType() != types::TokenType::TT_ID &&
                                         peek().getTokenType() != types::TokenType::TT_STR)) {
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
                uint64_t openParenPos = current - 1;
                int parenCount = 1;
                uint64_t searchPos = current;
                
                while (searchPos < tokens_count && parenCount > 0) {
                    auto token = scanner[searchPos];
                    if (token.getTokenType() == types::TokenType::TT_SYM) {
                        if (token.getTokenValue() == "(") parenCount++;
                        else if (token.getTokenValue() == ")") parenCount--;
                    }
                    searchPos++;
                    if (parenCount == 0) break;
                }
                
                if (parenCount > 0) {
                    throw std::runtime_error("Unmatched '(' in command substitution");
                }
                
                auto commandNode = parseSequence();
                if (!match(")")) {
                    current = openParenPos;
                    throw std::runtime_error("Expected ')' to close command substitution");
                }
                
                return commandNode;
            }

            std::shared_ptr<cmd::Node> parseIfStatement() {
                std::vector<cmd::IfStatement::Branch> branches;
                std::shared_ptr<cmd::Node> elseAction = nullptr;
                
                advance(); 
                auto firstCondition = parsePipeline();
                
                match(";");
                match("\n"); 
                
                if (isAtEnd() || peek().getTokenType() != types::TokenType::TT_ID || 
                    peek().getTokenValue() != "then") {
                    throw std::runtime_error("Expected 'then' after if condition found: " + peek().getTokenValue() + " on Line: " + std::to_string(peek().get_pos().first));
                }
                advance(); 
                match(";");
                match("\n");
                
                std::vector<std::shared_ptr<cmd::Node>> firstActions;
                while (!isAtEnd() && 
                       (peek().getTokenType() != types::TokenType::TT_ID || 
                        (peek().getTokenValue() != "elif" && 
                         peek().getTokenValue() != "else" && 
                         peek().getTokenValue() != "fi"))) {
                    firstActions.push_back(parseStatement());
                    
                    while (match(";") || match("\n")) {
                    
                    }
                }
                
                branches.push_back({firstCondition, std::make_shared<cmd::Sequence>(firstActions)});
                
                while (!isAtEnd() && 
                       peek().getTokenType() == types::TokenType::TT_ID &&
                       peek().getTokenValue() == "elif") {
                    advance(); 
                    
                    auto elifCondition = parsePipeline();
                    
                    match(";");
                    match("\n");
                    
                    if (isAtEnd() || peek().getTokenType() != types::TokenType::TT_ID || 
                        peek().getTokenValue() != "then") {
                        throw std::runtime_error("Expected 'then' after elif condition");
                    }
                    advance(); 
                    
                    match(";");
                    match("\n");
                    
                    std::vector<std::shared_ptr<cmd::Node>> elifActions;
                    
                    while (!isAtEnd() && 
                           (peek().getTokenType() != types::TokenType::TT_ID || 
                            (peek().getTokenValue() != "elif" && 
                             peek().getTokenValue() != "else" && 
                             peek().getTokenValue() != "fi"))) {
                        elifActions.push_back(parseStatement());
                        
                        while (match(";") || match("\n")) {
                        
                        }
                    }
                    
                    branches.push_back({elifCondition, std::make_shared<cmd::Sequence>(elifActions)});
                }
                
                if (!isAtEnd() && 
                    peek().getTokenType() == types::TokenType::TT_ID &&
                    peek().getTokenValue() == "else") {
                    advance(); 
                    
                    match(";");
                    match("\n");
                    
                    std::vector<std::shared_ptr<cmd::Node>> elseActions;
                    
                    while (!isAtEnd() && 
                           (peek().getTokenType() != types::TokenType::TT_ID || 
                            peek().getTokenValue() != "fi")) {
                        elseActions.push_back(parseStatement());
                        
                        while (match(";") || match("\n")) {
                        
                        }
                    }
                    
                    elseAction = std::make_shared<cmd::Sequence>(elseActions);
                }
                
                if (isAtEnd() || 
                    peek().getTokenType() != types::TokenType::TT_ID ||
                    peek().getTokenValue() != "fi") {
                    throw std::runtime_error("Expected 'fi' to close if statement");
                }
                advance(); 
                
                return std::make_shared<cmd::IfStatement>(branches, elseAction);
            }

            std::shared_ptr<cmd::Node> parseWhileStatement() {
                advance(); 
                
                auto condition = parsePipeline();
                
                match(";");
                match("\n");
                
                if (isAtEnd() || peek().getTokenType() != types::TokenType::TT_ID || 
                    peek().getTokenValue() != "do") {
                    throw std::runtime_error("Expected 'do' after while condition");
                }
                advance(); 
                
                match(";");
                match("\n");
                
                std::vector<std::shared_ptr<cmd::Node>> bodyStatements;
                while (!isAtEnd() && 
                       (peek().getTokenType() != types::TokenType::TT_ID || 
                        peek().getTokenValue() != "done")) {
                    bodyStatements.push_back(parseStatement());
                    
                    while (match(";") || match("\n")) {
                        
                    }
                }
                
                if (isAtEnd() || peek().getTokenType() != types::TokenType::TT_ID || 
                    peek().getTokenValue() != "done") {
                    throw std::runtime_error("Expected 'done' to close while statement");
                }
                advance();
                
                return std::make_shared<cmd::WhileStatement>(
                    condition, std::make_shared<cmd::Sequence>(bodyStatements));
            }

            std::shared_ptr<cmd::Node> parseForStatement() {
                advance(); 
                if (peek().getTokenType() != types::TokenType::TT_ID) {
                    throw std::runtime_error("Expected variable name after 'for'");
                }
                std::string variableName = advance().getTokenValue();
                if (peek().getTokenType() != types::TokenType::TT_ID || 
                    peek().getTokenValue() != "in") {
                    throw std::runtime_error("Expected 'in' after variable name in for loop");
                }
                advance(); 
                
                std::vector<Argument> values;
                while (!isAtEnd() && 
                       peek().getTokenValue() != ";" && 
                       peek().getTokenValue() != "do") {
                    
                    if (peek().getTokenType() == types::TokenType::TT_ID) {
                        std::string value = advance().getTokenValue();
                        Argument arg;
                        arg.value = value;
                        arg.type = ARG_VARIABLE;
                        values.push_back(arg);
                    } 
                    else if (peek().getTokenType() == types::TokenType::TT_STR) {
                        std::string value = advance().getTokenValue();
                        if (value.size() >= 2 && 
                            ((value.front() == '"' && value.back() == '"') || 
                             (value.front() == '\'' && value.back() == '\''))) {
                            value = value.substr(1, value.size() - 2);
                        }
                        Argument arg;
                        arg.value = value;
                        arg.type = ARG_LITERAL; 
                        values.push_back(arg);
                    }
                    else if (peek().getTokenType() == types::TokenType::TT_NUM) {
                        std::string value = advance().getTokenValue();
                        Argument arg;
                        arg.value = value;
                        arg.type = ARG_LITERAL; 
                        values.push_back(arg);
                    }
                    else {
                        break;
                    }
                }
                
                if (values.empty()) {
                    throw std::runtime_error("Expected at least one value in for loop");
                }
                
                
                match(";");
                
                
                if (isAtEnd() || peek().getTokenType() != types::TokenType::TT_ID || 
                    peek().getTokenValue() != "do") {
                    throw std::runtime_error("Expected 'do' after values in for loop");
                }
                advance(); 
                
                match(";");
                match("\n");
                
                
                std::vector<std::shared_ptr<cmd::Node>> bodyStatements;
                while (!isAtEnd() && 
                       (peek().getTokenType() != types::TokenType::TT_ID || 
                        peek().getTokenValue() != "done")) {
                    bodyStatements.push_back(parseStatement());
                    
                    while (match(";") || match("\n")) {
                    }
                }
                
                if (isAtEnd() || peek().getTokenType() != types::TokenType::TT_ID || 
                    peek().getTokenValue() != "done") {
                    throw std::runtime_error("Expected 'done' to close for loop");
                }
                advance();                 
                
                return std::make_shared<cmd::ForStatement>(
                    variableName, 
                    values,
                    std::make_shared<cmd::Sequence>(bodyStatements));
            }

            std::shared_ptr<cmd::Node> parseCommandDefinition() {
                advance();
                if (peek().getTokenType() != types::TokenType::TT_ID) {
                    throw std::runtime_error("Expected command name after 'define'");
                }
                std::string commandName = advance().getTokenValue();
                if (!match("(")) {
                    throw std::runtime_error("Expected '(' after command name");
                }
                
                std::vector<std::string> parameters;
                bool firstParam = true;
                
                while (!isAtEnd() && peek().getTokenValue() != ")") {
                    if (!firstParam) {
                        if (!match(",")) {
                            throw std::runtime_error("Expected ',' between parameters");
                        }
                    } else if (peek().getTokenValue() != ")") {
                        firstParam = false;
                    }
                    
                    if (peek().getTokenType() != types::TokenType::TT_ID) {
                        throw std::runtime_error("Expected parameter name");
                    }
                    
                    parameters.push_back(advance().getTokenValue());
                }
                
                if (!match(")")) {
                    throw std::runtime_error("Expected ')' after parameter list");
                }
                
                
                if (peek().getTokenType() != types::TokenType::TT_ID || peek().getTokenValue() != "begin") {
                    throw std::runtime_error("Expected 'begin' after parameter list");
                }
                advance();
                
                match(";"); 
                match("\n"); 
                
                
                std::vector<std::shared_ptr<cmd::Node>> bodyStatements;
                
                while (!isAtEnd() && 
                       !(peek().getTokenType() == types::TokenType::TT_ID && 
                         peek().getTokenValue() == "end")) {
                    bodyStatements.push_back(parseStatement());
                    
                    while (match(";") || match("\n")) {
                    }
                }
                
                if (isAtEnd() || peek().getTokenType() != types::TokenType::TT_ID || peek().getTokenValue() != "end") {
                    throw std::runtime_error("Expected 'end' to close command definition");
                }
                advance();
                
                auto body = std::make_shared<cmd::Sequence>(bodyStatements);
                return std::make_shared<cmd::CommandDefinition>(commandName, parameters, body);
            }

            std::shared_ptr<cmd::Node> parseScript() {
                std::vector<std::shared_ptr<cmd::Node>> statements;
                while (!isAtEnd()) {
                    while (!isAtEnd() && 
                          (match(";") || match("\n"))) {
                    }
                    
                    if (!isAtEnd()) {
                        statements.push_back(parseStatement());
                        while (!isAtEnd() && 
                              (match(";") || match("\n"))) {
                            
                        }
                    }
                }
                
                if (statements.size() == 1) {
                    return statements[0];
                }
                
                return std::make_shared<cmd::Sequence>(statements);
            }

            std::shared_ptr<cmd::Node> parseStatement() {
                if (peek().getTokenType() == types::TokenType::TT_ID) {
                    if (peek().getTokenValue() == "if") {
                        return parseIfStatement();
                    }
                    else if (peek().getTokenValue() == "while") {
                        return parseWhileStatement();
                    }
                    else if (peek().getTokenValue() == "for") {
                        return parseForStatement();
                    }
                    else if (peek().getTokenValue() == "break") {
                        advance(); 
                        return std::make_shared<cmd::Break>();
                    }
                    else if (peek().getTokenValue() == "continue") {
                        advance(); 
                        return std::make_shared<cmd::Continue>();
                    }
                    else if (peek().getTokenValue() == "define") {
                        return parseCommandDefinition();
                    }
                    else if (peek().getTokenValue() == "return") {
                        advance(); 
                        auto expr = parseExpression();
                        return std::make_shared<cmd::Return>(expr);
                    }
                }
                return parsePipeline();
            }
        };     
}
#endif