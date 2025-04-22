#include"ast.hpp"
#include"scanner.hpp"
#include"types.hpp"
#include"string_buffer.hpp"
#include<iostream>
#include<string>
#include<fstream>
#include<optional>
#include<vector>
#include<memory>
#include<functional>
#include<unordered_map>

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

    class AstExecutor {
    public:
        AstExecutor(const CommandRegistry& registry) : registry(registry) {}
        
        void execute(const std::shared_ptr<cmd::Node>& node) {
            std::istream& defaultInput = std::cin;
            std::ostream& defaultOutput = std::cout;   
            executeNode(node, defaultInput, defaultOutput);
        }
        
    private:
        const CommandRegistry& registry;
        
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
            else {
                throw std::runtime_error("Unknown node type");
            }
        }
        
        void executeCommand(const std::shared_ptr<cmd::Command>& cmd, std::istream& input, std::ostream& output) {
            registry.executeCommand(cmd->name, cmd->args, input, output);
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
    };

    void exitCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output) {
        std::exit(0);
    }
    
    void echoCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output) {
        for (size_t i = 0; i < args.size(); i++) {
            output << args[i];
            if (i < args.size() - 1) {
                output << " ";
            }
        }
        output << std::endl;
    }

    
    void catCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output) {
        if (args.empty()) {
    
            std::string line;
            while (std::getline(input, line)) {
                output << line << std::endl;
            }
        } else {
    
            for (const auto& filename : args) {
                std::ifstream file(filename);
                if (!file) {
                    std::cerr << "cat: " << filename << ": No such file" << std::endl;
                    continue;
                }
                std::string line;
                while (std::getline(file, line)) {
                    output << line << std::endl;
                }
            }
        }
    }

    
    void grepCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output) {
        if (args.empty()) {
            std::cerr << "grep: missing pattern" << std::endl;
            return;
        }
        
        const std::string& pattern = args[0];
        std::string line;
        
        while (std::getline(input, line)) {
            if (line.find(pattern) != std::string::npos) {
                output << line << std::endl;
            }
        }
    }

}

int main(int argc, char **argv) {
    bool active = true;
    try {
        cmd::CommandRegistry registry;
        registry.registerCommand("echo", cmd::echoCommand);
        registry.registerCommand("cat", cmd::catCommand);
        registry.registerCommand("grep", cmd::grepCommand);
        registry.registerCommand("exit", cmd::exitCommand);
        cmd::AstExecutor executor(registry);

        while(active) {
            std::string command_data;
            std::cout << "=)> ";
            std::getline(std::cin, command_data);
            scan::TString string_buffer(command_data);
            scan::Scanner scanner(string_buffer);
            cmd::Parser parser(scanner);
            auto ast = parser.parse();
            executor.execute(ast);
        }
        
    } catch (const scan::ScanExcept &e) {
        std::cerr << "Scan error: " << e.why() << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
    }   
    return 0;
}