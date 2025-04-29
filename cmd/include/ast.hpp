#ifndef __AST_HPP_X_
#define __AST_HPP_X_

//#define DEBUG_MODE 

#include<memory>
#include<functional>
#include<optional>
#include<string>
#include<vector>
#include<unordered_map>
#include<fstream>
#include<iostream>
#include<filesystem>
#include<sstream>
#include"command.hpp"
#include<set>
#include<cmath>

namespace cmd {
    class Node;
    class Expression;
    class AstExecutor;
    class Command;
    class Pipeline;
    class Redirection;
    class Sequence;
    class StringLiteral;
    class NumberLiteral;
    class VariableReference;
    class VariableAssignment;
    class LogicalAnd;
    class BinaryExpression;
    class UnaryExpression;
    class CommandSubstitution;

    class Node {
    public:
        virtual ~Node() = default;
        virtual void print(int indent = 0) const = 0;
        virtual void execute(const AstExecutor& executor) const {}
    };
    
    class Expression : public Node {
    public:
        virtual ~Expression() = default;
        virtual std::string evaluate(const AstExecutor& executor) const = 0;
        virtual double evaluateNumber(const AstExecutor& executor) const = 0;
    };
    
    class Command : public Node {
    public:
        Command(std::string name, std::vector<std::string> args) 
            : name(std::move(name)), args(std::move(args)) {}

        void print(int indent = 0) const override {
            std::cout << std::string(indent, ' ') << "Command: " << name << std::endl;
            for (const auto& arg : args) {
                std::cout << std::string(indent + 2, ' ') << "Arg: " << arg << std::endl;
            }
        }
        std::string name;
        std::vector<std::string> args;
    };

    class Pipeline : public Node {
    public:
        Pipeline(std::vector<std::shared_ptr<Command>> commands) 
            : commands(std::move(commands)) {}

        void print(int indent = 0) const override {
            std::cout << std::string(indent, ' ') << "Pipeline:" << std::endl;
            for (const auto& cmd : commands) {
                cmd->print(indent + 2);
            }
        }

        std::vector<std::shared_ptr<Command>> commands;
    };


    class Redirection : public Node {
    public:
        enum Type { INPUT, OUTPUT, APPEND };

        Redirection(std::shared_ptr<Node> command, Type type, std::string file) 
            : command(std::move(command)), type(type), file(std::move(file)) {}

        void print(int indent = 0) const override {
            std::cout << std::string(indent, ' ') << "Redirection: ";
            if (type == INPUT) std::cout << "INPUT from ";
            else if (type == OUTPUT) std::cout << "OUTPUT to ";
            else std::cout << "APPEND to ";
            std::cout << file << std::endl;
            command->print(indent + 2);
        }

        std::shared_ptr<Node> command;
        Type type;
        std::string file;
    };


    class Sequence : public Node {
    public:
        Sequence(std::vector<std::shared_ptr<Node>> commands) 
            : commands(std::move(commands)) {}

        void print(int indent = 0) const override {
            std::cout << std::string(indent, ' ') << "Sequence:" << std::endl;
            for (const auto& cmd : commands) {
                cmd->print(indent + 2);
            }
        }

        std::vector<std::shared_ptr<Node>> commands;
    };

    using CommandFunction = std::function<int(const std::vector<std::string>&, std::istream&, std::ostream&)>;

    class CommandRegistry {
    public:
    
        void registerCommand(const std::string& name, CommandFunction func) {
            commands[name] = func;
        }
        
        int executeCommand(const std::string& name, const std::vector<std::string>& args, 
                          std::istream& input, std::ostream& output) {
            auto it = commands.find(name);
            if (it != commands.end()) {
                return it->second(args, input, output);
            } else {
                output << "Command not found: " << name << std::endl;
                return 1; 
            }
        }
        
    private:
        std::unordered_map<std::string, CommandFunction> commands;
    };

    class StringLiteral : public Node {
    public:
        StringLiteral(const std::string& value) : value(value) {}
        std::string value;
        
        void print(int indent = 0) const override {
            std::string spaces(indent, ' ');
            std::cout << spaces << "StringLiteral: " << value << std::endl;
        }
    };

    class NumberLiteral : public Expression {
    public:
        NumberLiteral(double value) : value(value) {}
        
        void print(int indent = 0) const override {
            std::string spaces(indent, ' ');
            std::cout << spaces << "NumberLiteral: " << value << std::endl;
        }
        
        std::string evaluate(const AstExecutor& executor) const override {
            return std::to_string(value);
        }
        
        double evaluateNumber(const AstExecutor& executor) const override {
            return value;
        }
        
        double value;
    };

    class VariableReference : public Expression {
    public:
        VariableReference(const std::string& name) : name(name) {}
        
        void print(int indent = 0) const override {
            std::string spaces(indent, ' ');
            std::cout << spaces << "VariableReference: " << name << std::endl;
        }
        
        std::string evaluate(const AstExecutor& executor) const override;
        double evaluateNumber(const AstExecutor& executor) const override;
        
        std::string name;
    };

    class VariableAssignment : public Node {
    public:
        VariableAssignment(const std::string& name, std::shared_ptr<Node> value) 
            : name(name), value(value) {}
        
        void print(int indent = 0) const override {
            std::string spaces(indent, ' ');
            std::cout << spaces << "VariableAssignment: " << name << " =" << std::endl;
            value->print(indent + 2);
        }
        
        std::string name;
        std::shared_ptr<Node> value;
    };

    class LogicalAnd : public Node {
    public:
        LogicalAnd(std::shared_ptr<Node> left, std::shared_ptr<Node> right) 
            : left(std::move(left)), right(std::move(right)) {}

        void print(int indent = 0) const override {
            std::string spaces(indent, ' ');
            std::cout << spaces << "LogicalAnd:" << std::endl;
            left->print(indent + 2);
            right->print(indent + 2);
        }

        std::shared_ptr<Node> left;
        std::shared_ptr<Node> right;
    };

    class AstExecutor {
    public:
        AstExecutor() {
            registry.registerCommand("echo", cmd::echoCommand);
            registry.registerCommand("cat", cmd::catCommand);
            registry.registerCommand("grep", cmd::grepCommand);
            registry.registerCommand("exit", cmd::exitCommand);
            registry.registerCommand("print", cmd::printCommand);
            registry.registerCommand("cd", cmd::cdCommand);
            registry.registerCommand("ls", cmd::listCommand);
            registry.registerCommand("dir", cmd::listCommand);
            registry.registerCommand("find", cmd::findCommand);
            registry.registerCommand("sort", cmd::sortCommand);
            registry.registerCommand("pwd", cmd::pwdCommand);
            registry.registerCommand("mkdir", cmd::mkdirCommand);
            registry.registerCommand("cp", cmd::cpCommand);
            registry.registerCommand("mv", cmd::mvCommand);
            registry.registerCommand("touch", cmd::touchCommand);
            registry.registerCommand("head", cmd::headCommand);
            registry.registerCommand("tail", cmd::tailCommand);
            registry.registerCommand("wc", cmd::wcCommand);
            registry.registerCommand("chmod", cmd::chmodCommand);
            registry.registerCommand("sed", cmd::sedCommand);
            registry.registerCommand("printf", cmd::printfCommand);
            registry.registerCommand("debug_set", cmd::debugSet);
            registry.registerCommand("debug_get", cmd::debugGet);
            registry.registerCommand("debug_list", cmd::debugList);
            registry.registerCommand("debug_clear", cmd::debugClear);
            registry.registerCommand("debug_search", cmd::debugSearch);
            registry.registerCommand("debug_dump", cmd::dumpVariables);
        }

        std::string getPath() const {
            return std::filesystem::current_path().string();
        }

        void setPath(const std::string& newPath) {
            if (std::filesystem::exists(newPath)) {
                path = newPath;
                std::filesystem::current_path(path);
            } else {
                throw std::runtime_error("Path does not exist: " + newPath);
            }
        }
        
        void addCommand(const std::string& name, CommandFunction func) {
            registry.registerCommand(name, func);
        }

        void execute(std::istream &defaultInput, std::ostream &defaultOutput, const std::shared_ptr<cmd::Node>& node) {
            
            try {
                #ifdef DEBUG_MODE
                if (std::dynamic_pointer_cast<cmd::Command>(node)) {
                    std::cout << "DEBUG: Node is a Command" << std::endl;
                } else if (std::dynamic_pointer_cast<cmd::Sequence>(node)) {
                    std::cout << "DEBUG: Node is a Sequence" << std::endl;
                } else if (std::dynamic_pointer_cast<cmd::Pipeline>(node)) {
                    std::cout << "DEBUG: Node is a Pipeline" << std::endl;
                } else if (std::dynamic_pointer_cast<cmd::Redirection>(node)) {
                    std::cout << "DEBUG: Node is a Redirection" << std::endl;
                } else if (std::dynamic_pointer_cast<cmd::VariableAssignment>(node)) {
                    std::cout << "DEBUG: Node is a VariableAssignment" << std::endl;
                } else {
                    std::cout << "DEBUG: Node is of UNKNOWN type" << std::endl;
                }
                #endif
                executeNode(node, defaultInput, defaultOutput);
            } catch (const std::exception& e) {
                defaultOutput << "Error: " << e.what() << std::endl;
            }
        }

        void setVariable(const std::string& name, const std::string& value) {
            state::GameState *gameState = state::getGameState();
            gameState->setVariable(name, value);
        }
        
        std::optional<std::string> getVariable(const std::string& name) const {
            state::GameState *gameState = state::getGameState();
            try {
                return gameState->getVariable(name);
            } catch (const state::StateException &e) {
                return std::nullopt;
            }
            return std::nullopt;
        }

        std::string expandVariables(const std::string& input, 
                                    std::set<std::string> expandingVars = {}) const {
            if (input.empty()) return input;
            
            std::string result = input;
            
            if (std::isalpha(input[0]) || input[0] == '_') {
                bool isValidVarName = true;
                for (char c : input) {
                    if (!std::isalnum(c) && c != '_') {
                        isValidVarName = false;
                        break;
                    }
                }
                
                if (isValidVarName) {
                    auto value = getVariable(input);
                    if (value) {
                        return *value;
                    }
                }
            }
            
            size_t pos = 0;
            while ((pos = result.find("${", pos)) != std::string::npos) {
                size_t end = result.find("}", pos);
                if (end == std::string::npos) break;
                std::string varName = result.substr(pos + 2, end - pos - 2);
                if (expandingVars.find(varName) != expandingVars.end()) {
                    result.replace(pos, end - pos + 1, "[circular reference]");
                    pos += 20;
                    throw std::runtime_error("Circular reference detected for variable: " + varName); 
                    continue;
                }   
                auto value = getVariable(varName);
                if (value) {
                    std::set<std::string> newExpandingVars = expandingVars;
                    newExpandingVars.insert(varName);
                    std::string expandedValue = expandVariables(*value, newExpandingVars);
                    result.replace(pos, end - pos + 1, expandedValue);
                    pos += expandedValue.length();
                } else {
                    result.replace(pos, end - pos + 1, "");
                }
            }
            
            
            pos = 0;
            while ((pos = result.find("$", pos)) != std::string::npos) {
                if (pos + 1 < result.size() && result[pos + 1] == '{') {
                    pos++;
                    continue;
                }
                
                size_t end = pos + 1;
                while (end < result.size() && (std::isalnum(result[end]) || result[end] == '_')) {
                    end++;
                }
                
                if (end > pos + 1) {
                    std::string varName = result.substr(pos + 1, end - pos - 1);
                    if (expandingVars.find(varName) != expandingVars.end()) {
                        result.replace(pos, end - pos, "[circular reference]");
                        pos += 20;
                        continue;
                    }
                    auto value = getVariable(varName);
                    if (value) {
                        std::set<std::string> newExpandingVars = expandingVars;
                        newExpandingVars.insert(varName);
                        std::string expandedValue = expandVariables(*value, newExpandingVars);
                        result.replace(pos, end - pos, expandedValue);
                        pos += expandedValue.length();
                    } else {
                        result.replace(pos, end - pos, "");
                    }
                } else {
                    pos++;
                }
            }
            
            return result;
        }

        int getLastExitStatus() const {
            return lastExitStatus;
        }
        
        void executeInternal(std::istream& input, std::ostream& output, std::shared_ptr<Node> node) const {
            std::istream* oldInput = this->input;
            std::ostream* oldOutput = this->output;
            
            this->input = &input;
            this->output = &output;
            
            node->execute(*this);
            
            this->input = oldInput;
            this->output = oldOutput;
        }
        
        void executeDirectly(const std::shared_ptr<Node>& node, std::istream& input, std::ostream& output) {
            executeNode(node, input, output);
        }
        
        CommandRegistry &getCommandRegistry() {
            return registry;
        }

    private:
        CommandRegistry registry;
        std::string path;
        int lastExitStatus = 0;
        mutable std::istream* input = nullptr;
        mutable std::ostream* output = nullptr;

        void executeNode(const std::shared_ptr<cmd::Node>& node, std::istream& input, std::ostream& output) {
            if (auto cmd = std::dynamic_pointer_cast<cmd::Command>(node)) {
                executeCommand(cmd, input, output);
            } 
            else if (auto seq = std::dynamic_pointer_cast<cmd::Sequence>(node)) {
                executeSequence(seq, input, output);
            }
            else if (auto pipe = std::dynamic_pointer_cast<cmd::Pipeline>(node)) {
                executePipeline(pipe, input, output);
            }
            else if (auto redir = std::dynamic_pointer_cast<cmd::Redirection>(node)) {
                executeRedirection(redir, input, output);
            }
            else if (auto varAssign = std::dynamic_pointer_cast<cmd::VariableAssignment>(node)) {
                executeVariableAssignment(varAssign, input, output);
            }
            else if (auto logicalAnd = std::dynamic_pointer_cast<cmd::LogicalAnd>(node)) {
                executeLogicalAnd(logicalAnd, input, output);
            }
            else if (auto expr = std::dynamic_pointer_cast<cmd::Expression>(node)) {
                expr->evaluate(*this);
            }
            else {
                throw std::runtime_error("Unknown node type");
            }
        }
        
        void executeCommand(const std::shared_ptr<cmd::Command>& cmd, std::istream& input, std::ostream& output) {
            std::vector<std::string> expandedArgs;
            for (const auto& arg : cmd->args) {
                expandedArgs.push_back(expandVariables(arg));
            }
            
            lastExitStatus = registry.executeCommand(cmd->name, expandedArgs, input, output);
            
            if (lastExitStatus != 0) {
                output << cmd->name << ": command failed with exit status " << lastExitStatus << std::endl;
            }
        }
        
        void executeSequence(const std::shared_ptr<cmd::Sequence>& seq, std::istream& input, std::ostream& output) {
            for (const auto& cmd : seq->commands) {
                executeNode(cmd, input, output);
            }
        }
        
        void executePipeline(const std::shared_ptr<cmd::Pipeline>& pipe, std::istream& input, std::ostream& output) {
            std::stringstream buffer;
            for (size_t i = 0; i < pipe->commands.size() - 1; i++) {
                std::stringstream nextBuffer;
                auto cmd = pipe->commands[i];
                std::istream* currentInput = (i == 0) ? &input : &buffer;
                lastExitStatus = registry.executeCommand(cmd->name, cmd->args, *currentInput, nextBuffer);
                
                if (lastExitStatus != 0) {
                    output << cmd->name << ": command failed with exit status " << lastExitStatus << std::endl;
                }
                
                buffer = std::move(nextBuffer);
            }
            auto lastCmd = pipe->commands.back();
            lastExitStatus = registry.executeCommand(lastCmd->name, lastCmd->args, buffer, output);
            
            if (lastExitStatus != 0) {
                output << lastCmd->name << ": command failed with exit status " << lastExitStatus << std::endl;
            }
        }
        
        void executeRedirection(const std::shared_ptr<cmd::Redirection>& redir, std::istream& input, std::ostream& output) {
            if (redir->type == cmd::Redirection::INPUT) {
                std::ifstream fileInput(redir->file);
                if (!fileInput) {
                    throw std::runtime_error("Failed to open input file: " + redir->file);
                }
                executeNode(redir->command, fileInput, output);
            }
            else if (redir->type == cmd::Redirection::OUTPUT) {
                std::ofstream fileOutput(redir->file);
                if (!fileOutput) {
                    throw std::runtime_error("Failed to open output file: " + redir->file);
                }
                executeNode(redir->command, input, fileOutput);
            }
            else if (redir->type == cmd::Redirection::APPEND) {
                std::ofstream fileOutput(redir->file, std::ios::app);
                if (!fileOutput) {
                    throw std::runtime_error("Failed to open output file for append: " + redir->file);
                }
                executeNode(redir->command, input, fileOutput);
            }
        }

        void executeVariableAssignment(const std::shared_ptr<cmd::VariableAssignment>& varAssign, 
                                       std::istream& input, std::ostream& output) {
            
            if (auto strLit = std::dynamic_pointer_cast<cmd::StringLiteral>(varAssign->value)) {
                std::string value = strLit->value;
                if (value.size() >= 2 && 
                    ((value.front() == '"' && value.back() == '"') || 
                     (value.front() == '\'' && value.back() == '\''))) {
                    value = value.substr(1, value.size() - 2);
                }
                setVariable(varAssign->name, value);
            }
            else if (auto expr = std::dynamic_pointer_cast<cmd::Expression>(varAssign->value)) {
                std::string value = expr->evaluate(*this);
                setVariable(varAssign->name, value);
            } 
            else {
                throw std::runtime_error("Unsupported value type in assignment");
            }
        }

        void executeLogicalAnd(const std::shared_ptr<cmd::LogicalAnd>& logicalAnd, std::istream& input, std::ostream& output) {
            executeNode(logicalAnd->left, input, output);
            if (lastExitStatus == 0) {
                executeNode(logicalAnd->right, input, output);
            }
        }
    };


    class BinaryExpression : public Expression {
    public:
        enum OpType { ADD, SUBTRACT, MULTIPLY, DIVIDE, MODULO };
        
        BinaryExpression(std::shared_ptr<Expression> left, OpType op, std::shared_ptr<Expression> right)
            : left(std::move(left)), op(op), right(std::move(right)) {}
        
        void print(int indent = 0) const override {
            std::string spaces(indent, ' ');
            std::cout << spaces << "BinaryExpression: ";
            switch (op) {
                case ADD: std::cout << "+"; break;
                case SUBTRACT: std::cout << "-"; break;
                case MULTIPLY: std::cout << "*"; break;
                case DIVIDE: std::cout << "/"; break;
                case MODULO: std::cout << "%"; break;
            }
            std::cout << std::endl;
            left->print(indent + 2);
            right->print(indent + 2);
        }
        
        std::string evaluate(const AstExecutor& executor) const override {
            return std::to_string(evaluateNumber(executor));
        }
        
        double evaluateNumber(const AstExecutor& executor) const override {
            double leftVal = left->evaluateNumber(executor);
            double rightVal = right->evaluateNumber(executor);
            
            switch (op) {
                case ADD: return leftVal + rightVal;
                case SUBTRACT: return leftVal - rightVal;
                case MULTIPLY: return leftVal * rightVal;
                case DIVIDE: 
                    if (rightVal == 0) throw std::runtime_error("Division by zero");
                    return leftVal / rightVal;
                case MODULO:
                    if (rightVal == 0) throw std::runtime_error("Modulo by zero");
                    return std::fmod(leftVal, rightVal);
                default: throw std::runtime_error("Unknown binary operator");
            }
        }
        
        std::shared_ptr<Expression> left;
        OpType op;
        std::shared_ptr<Expression> right;
    };

    class UnaryExpression : public Expression {
    public:
        enum OpType { INCREMENT, DECREMENT, NEGATE };
        enum Position { PREFIX, POSTFIX };
        
        UnaryExpression(std::shared_ptr<Expression> operand, OpType op, Position pos = PREFIX)
            : operand(std::move(operand)), op(op), position(pos) {}
        
        void print(int indent = 0) const override {
            std::string spaces(indent, ' ');
            std::cout << spaces << "UnaryExpression: ";
            switch (op) {
                case INCREMENT: std::cout << "++"; break;
                case DECREMENT: std::cout << "--"; break;
                case NEGATE: std::cout << "-"; break;
            }
            std::cout << (position == PREFIX ? " (prefix)" : " (postfix)") << std::endl;
            operand->print(indent + 2);
        }
        
        std::string evaluate(const AstExecutor& executor) const override {
            return std::to_string(evaluateNumber(executor));
        }
        
        double evaluateNumber(const AstExecutor& executor) const override {
            if (op == INCREMENT || op == DECREMENT) {
                auto varRef = std::dynamic_pointer_cast<VariableReference>(operand);
                if (!varRef) {
                    throw std::runtime_error("Increment/decrement can only be applied to variables");
                }
                
                auto valueOpt = executor.getVariable(varRef->name);
                double currentValue = 0.0;
                if (valueOpt) {
                    try {
                        currentValue = std::stod(valueOpt.value());
                    } catch (const std::exception&) {
                        
                    }
                }
                
                double originalValue = currentValue;
                if (op == INCREMENT) {
                    currentValue += 1.0;
                } else { 
                    currentValue -= 1.0;
                }
                const_cast<AstExecutor&>(executor).setVariable(varRef->name, std::to_string(currentValue));
                return (position == PREFIX) ? currentValue : originalValue;
            } 
            else if (op == NEGATE) {
                return -operand->evaluateNumber(executor);
            }
            
            throw std::runtime_error("Unknown unary operator");
        }
        
        std::shared_ptr<Expression> operand;
        OpType op;
        Position position;
    };

    class CommandSubstitution : public Expression {
    public:
        CommandSubstitution(std::shared_ptr<Node> command) : command(command) {}
        
        std::string evaluate(const AstExecutor& executor) const override {
            std::stringstream input;  
            std::stringstream output;
            
            if (auto cmd = std::dynamic_pointer_cast<cmd::Command>(command)) {
                std::vector<std::string> expandedArgs;
                for (const auto& arg : cmd->args) {
                    expandedArgs.push_back(executor.expandVariables(arg));
                }
                
                const_cast<AstExecutor&>(executor).getCommandRegistry().executeCommand(
                    cmd->name, expandedArgs, input, output);
            }
            else if (auto seq = std::dynamic_pointer_cast<cmd::Sequence>(command)) {
                for (const auto& node : seq->commands) {
                    const_cast<AstExecutor&>(executor).executeDirectly(node, input, output);
                }
            }
            else {
                const_cast<AstExecutor&>(executor).executeDirectly(command, input, output);
            }
            
            std::string result = output.str();
            while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
                result.pop_back();
            }
            
            return result;
        }
        
        double evaluateNumber(const AstExecutor& executor) const override {
            std::string result = evaluate(executor);
            try {
                return std::stod(result);
            } catch (...) {
                return 0.0;
            }
        }
        
   
        void print(int indent = 0) const override {
            std::string spaces(indent, ' ');
            std::cout << spaces << "CommandSubstitution:" << std::endl;
            command->print(indent + 2);
        }
        std::shared_ptr<Node> command;
    };
}

#endif
