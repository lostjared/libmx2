#ifndef __AST_HPP_X_
#define __AST_HPP_X_

//#define DEBUG_MODE 

#include"scanner.hpp"
#include<memory>
#include<functional>
#include<optional>
#include<string>
#include<vector>
#include<unordered_map>
#include<fstream>
#include<iostream>
#include<filesystem>
#include"command.hpp"

namespace cmd {

    using CommandFunction = std::function<void(const std::vector<std::string>& args, std::istream& input, std::ostream& output)>;

    class Node {
        public:
            virtual ~Node() = default;
            virtual void print(int indent = 0) const = 0;
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

    class CommandRegistry {
    public:
        void registerCommand(const std::string& name, CommandFunction func) {
            commands[name] = std::move(func);
        }
        
        bool hasCommand(const std::string& name) const {
            return commands.find(name) != commands.end();
        }
        
        void executeCommand(const std::string& name, const std::vector<std::string>& args, std::istream& input, std::ostream& output) const {
            if (!hasCommand(name)) {
                throw std::runtime_error("Command not found: " + name);
            }
            commands.at(name)(args, input, output);
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

    class NumberLiteral : public Node {
    public:
        NumberLiteral(double value) : value(value) {}
        double value;
        
        void print(int indent = 0) const override {
            std::string spaces(indent, ' ');
            std::cout << spaces << "NumberLiteral: " << value << std::endl;
        }
    };

    class VariableReference : public Node {
    public:
        VariableReference(const std::string& name) : name(name) {}
        std::string name;
        
        void print(int indent = 0) const override {
            std::string spaces(indent, ' ');
            std::cout << spaces << "VariableReference: " << name << std::endl;
        }
    };

    class VariableAssignment : public Node {
    public:
        VariableAssignment(const std::string& name, std::shared_ptr<Node> value) 
            : name(name), value(value) {}
        std::string name;
        std::shared_ptr<Node> value;
        
        void print(int indent = 0) const override {
            std::string spaces(indent, ' ');
            std::cout << spaces << "VariableAssignment: " << name << " =" << std::endl;
            value->print(indent + 2);
        }
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
        
        void execute(const std::shared_ptr<cmd::Node>& node) {
            std::istream& defaultInput = std::cin;
            std::ostream& defaultOutput = std::cout;
            
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
                std::cerr << "Error: " << e.what() << std::endl;
            }
        }

        void setVariable(const std::string& name, const std::string& value) {
            variables[name] = value;
        }
        
        std::optional<std::string> getVariable(const std::string& name) const {
            auto it = variables.find(name);
            if (it != variables.end()) {
                return it->second;
            }
            return std::nullopt;
        }

        std::string expandVariables(const std::string& input) const {
            std::string result = input;
            size_t pos = 0;
            
            while ((pos = result.find("${", pos)) != std::string::npos) {
                size_t end = result.find("}", pos);
                if (end == std::string::npos) break;
                
                std::string varName = result.substr(pos + 2, end - pos - 2);
                auto value = getVariable(varName);
                
                if (value) {
                    result.replace(pos, end - pos + 1, *value);
                } else {
                    result.replace(pos, end - pos + 1, "");
                }
            }
            
            return result;
        }
        
    private:
        CommandRegistry registry;
        std::unordered_map<std::string, std::string> variables; 
        std::string path;
        
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
            else {
                throw std::runtime_error("Unknown node type");
            }
        }
        
        void executeCommand(const std::shared_ptr<cmd::Command>& cmd, std::istream& input, std::ostream& output) {
            std::vector<std::string> expandedArgs;
            for (const auto& arg : cmd->args) {
                expandedArgs.push_back(expandVariables(arg));
            }
            
            registry.executeCommand(cmd->name, expandedArgs, input, output);
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
                registry.executeCommand(cmd->name, cmd->args, *currentInput, nextBuffer);
                buffer = std::move(nextBuffer);
            }
            auto lastCmd = pipe->commands.back();
            registry.executeCommand(lastCmd->name, lastCmd->args, buffer, output);
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
            std::string value;
            
            if (auto strLiteral = std::dynamic_pointer_cast<cmd::StringLiteral>(varAssign->value)) {
                value = strLiteral->value;
            } else if (auto numLiteral = std::dynamic_pointer_cast<cmd::NumberLiteral>(varAssign->value)) {
                value = std::to_string(numLiteral->value);
            } else if (auto varRef = std::dynamic_pointer_cast<cmd::VariableReference>(varAssign->value)) {
                auto existingValue = getVariable(varRef->name);
                value = existingValue.value_or("");
            }
            
            setVariable(varAssign->name, value);
        }
    };

   
}

#endif