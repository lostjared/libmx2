#ifndef __AST_HPP_X_
#define __AST_HPP_X_

// #define DEBUG_MODE_ON

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
#include<set>
#include<cmath>
#include"game_state.hpp"

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
    class IfStatement;
    class WhileStatement;
    class ForStatement;

    class Node {
    public:
        virtual ~Node() = default;
        virtual void print(std::ostream& out, int indent = 0) const = 0;
        virtual void execute(const AstExecutor& executor) const {}

    protected:
        
        std::string getIndent(int indent) const {
            return std::string(indent, ' ');
        }

        
        std::string escapeHtml(const std::string& text) const {
            std::string result = text;
            size_t pos = 0;
            while ((pos = result.find("<", pos)) != std::string::npos) {
                result.replace(pos, 1, "&lt;");
                pos += 4;
            }
            pos = 0;
            while ((pos = result.find(">", pos)) != std::string::npos) {
                result.replace(pos, 1, "&gt;");
                pos += 4;
            }
            pos = 0;
            while ((pos = result.find("&", pos)) != std::string::npos && 
                   result.substr(pos, 4) != "&lt;" && result.substr(pos, 4) != "&gt;") {
                result.replace(pos, 1, "&amp;");
                pos += 5;
            }
            pos = 0;
            while ((pos = result.find("\"", pos)) != std::string::npos) {
                result.replace(pos, 1, "&quot;");
                pos += 6;
            }
            return result;
        }
    };
    
    class Expression : public Node {
    public:
        virtual ~Expression() = default;
        virtual std::string evaluate(const AstExecutor& executor) const = 0;
        virtual double evaluateNumber(const AstExecutor& executor) const = 0;
    };
    
    enum ArgType { ARG_LITERAL, ARG_VARIABLE };
    
    struct Argument {
        std::string value;
        ArgType type;
    };

    class Command : public Node {
    public:
        Command(std::string name, std::vector<Argument> args) 
            : name(std::move(name)), args(std::move(args)) {}

        void print(std::ostream& out, int indent = 0) const override {
            std::string spaces = getIndent(indent);
            out << spaces << "<div class='node command'>\n";
            out << spaces << "  <h3>Command: " << escapeHtml(name) << "</h3>\n";
            out << spaces << "  <table>\n";
            out << spaces << "    <tr><th colspan='2'>Arguments</th></tr>\n";
            for (const auto& arg : args) {
                out << spaces << "    <tr><td class='" 
                    << (arg.type == ARG_LITERAL ? "literal" : "variable") << "'>"
                    << escapeHtml(arg.value) << "</td>";
                out << "<td>(" << (arg.type == ARG_LITERAL ? "literal" : "variable") << ")</td></tr>\n";
            }
            out << spaces << "  </table>\n";
            out << spaces << "</div>\n";
        }
        
        std::string name;
        std::vector<Argument> args;
    };

    class LogicalAnd : public Node {
    public:
        LogicalAnd(std::shared_ptr<Node> left, std::shared_ptr<Node> right)
            : left(std::move(left)), right(std::move(right)) {}
        
        void print(std::ostream& out, int indent = 0) const override {
            std::string spaces = getIndent(indent);
            out << spaces << "<div class='node logical-and'>\n";
            out << spaces << "  <h3>LogicalAnd</h3>\n";
            out << spaces << "  <table>\n";
            out << spaces << "    <tr><th colspan='2'>Left Operand</th></tr>\n";
            out << spaces << "    <tr><td colspan='2'>\n";
            left->print(out, indent + 6);
            out << spaces << "    </td></tr>\n";
            out << spaces << "    <tr><td colspan='2' class='symbol'>&amp;&amp;</td></tr>\n";
            out << spaces << "    <tr><th colspan='2'>Right Operand</th></tr>\n";
            out << spaces << "    <tr><td colspan='2'>\n";
            right->print(out, indent + 6);
            out << spaces << "    </td></tr>\n";
            out << spaces << "  </table>\n";
            out << spaces << "</div>\n";
        }
        
        std::shared_ptr<Node> left;
        std::shared_ptr<Node> right;
    };

    class IfStatement : public Node {
    public:
        struct Branch {
            std::shared_ptr<Node> condition;  
            std::shared_ptr<Node> action;     
        };
        
        IfStatement(std::vector<Branch> branches, std::shared_ptr<Node> elseAction = nullptr) 
            : branches(std::move(branches)), elseAction(std::move(elseAction)) {}
        
        void print(std::ostream& out, int indent = 0) const override {
            std::string spaces = getIndent(indent);
            out << spaces << "<div class='node if-statement'>\n";
            out << spaces << "  <h3>IfStatement</h3>\n";
            out << spaces << "  <table>\n";
            
            for (size_t i = 0; i < branches.size(); ++i) {
                out << spaces << "    <tr><th colspan='2'>" << (i == 0 ? "if" : "elif") << "</th></tr>\n";
                out << spaces << "    <tr><td>condition:</td><td>\n";
                branches[i].condition->print(out, indent + 8);
                out << spaces << "    </td></tr>\n";
                out << spaces << "    <tr><td>then:</td><td>\n";
                branches[i].action->print(out, indent + 8);
                out << spaces << "    </td></tr>\n";
            }
            
            if (elseAction) {
                out << spaces << "    <tr><th colspan='2'>else</th></tr>\n";
                out << spaces << "    <tr><td colspan='2'>\n";
                elseAction->print(out, indent + 8);
                out << spaces << "    </td></tr>\n";
            }
            
            out << spaces << "    <tr><td colspan='2' class='symbol'>fi</td></tr>\n";
            out << spaces << "  </table>\n";
            out << spaces << "</div>\n";
        }
        
        std::vector<Branch> branches;  
        std::shared_ptr<Node> elseAction;  
    };

    class WhileStatement : public Node {
    public:
        WhileStatement(std::shared_ptr<Node> condition, std::shared_ptr<Node> body)
            : condition(std::move(condition)), body(std::move(body)) {}
        
        void print(std::ostream& out, int indent = 0) const override {
            std::string spaces = getIndent(indent);
            out << spaces << "<div class='node while-statement'>\n";
            out << spaces << "  <h3>WhileStatement</h3>\n";
            out << spaces << "  <table>\n";
            out << spaces << "    <tr><th>Condition</th></tr>\n";
            out << spaces << "    <tr><td>\n";
            condition->print(out, indent + 6);
            out << spaces << "    </td></tr>\n";
            out << spaces << "    <tr><th>Body</th></tr>\n";
            out << spaces << "    <tr><td>\n";
            body->print(out, indent + 6);
            out << spaces << "    </td></tr>\n";
            out << spaces << "    <tr><td class='symbol'>done</td></tr>\n";
            out << spaces << "  </table>\n";
            out << spaces << "</div>\n";
        }
        
        std::shared_ptr<Node> condition;
        std::shared_ptr<Node> body;
    };
    
    class ForStatement : public Node {
    public:
        ForStatement(std::string variable, std::vector<Argument> values, std::shared_ptr<Node> body)
            : variable(std::move(variable)), values(std::move(values)), body(std::move(body)) {}
        
        void print(std::ostream& out, int indent = 0) const override {
            std::string spaces = getIndent(indent);
            out << spaces << "<div class='node for-statement'>\n";
            out << spaces << "  <h3>ForStatement</h3>\n";
            out << spaces << "  <table>\n";
            out << spaces << "    <tr><th>Variable</th><td class='variable'>" << escapeHtml(variable) << "</td></tr>\n";
            out << spaces << "    <tr><th>Values</th><td>\n";
            out << spaces << "      <table>\n";
            for (const auto& val : values) {
                out << spaces << "        <tr><td class='" 
                    << (val.type == ARG_LITERAL ? "literal" : "variable") << "'>" 
                    << escapeHtml(val.value) << "</td><td>" 
                    << (val.type == ARG_LITERAL ? "literal" : "variable") << "</td></tr>\n";
            }
            out << spaces << "      </table>\n";
            out << spaces << "    </td></tr>\n";
            out << spaces << "    <tr><th>Body</th><td>\n";
            body->print(out, indent + 6);
            out << spaces << "    </td></tr>\n";
            out << spaces << "    <tr><td colspan='2' class='symbol'>done</td></tr>\n";
            out << spaces << "  </table>\n";
            out << spaces << "</div>\n";
        }
        
        std::string variable;
        std::vector<Argument> values;
        std::shared_ptr<Node> body;
    };

    class Pipeline : public Node {
    public:
        Pipeline(std::vector<std::shared_ptr<Command>> commands) 
            : commands(std::move(commands)) {}

        void print(std::ostream& out, int indent = 0) const override {
            std::string spaces = getIndent(indent);
            out << spaces << "<div class='node pipeline'>\n";
            out << spaces << "  <h3>Pipeline</h3>\n";
            out << spaces << "  <table>\n";
            out << spaces << "    <tr><th>Commands</th></tr>\n";
            for (size_t i = 0; i < commands.size(); i++) {
                out << spaces << "    <tr><td>\n";
                commands[i]->print(out, indent + 6);
                if (i < commands.size() - 1) {
                    out << spaces << "      <div class='symbol'>|</div>\n";
                }
                out << spaces << "    </td></tr>\n";
            }
            out << spaces << "  </table>\n";
            out << spaces << "</div>\n";
        }

        std::vector<std::shared_ptr<Command>> commands;
    };


    class Redirection : public Node {
    public:
        enum Type { INPUT, OUTPUT, APPEND };

        Redirection(std::shared_ptr<Node> command, Type type, std::string file) 
            : command(std::move(command)), type(type), file(std::move(file)) {}

        void print(std::ostream& out, int indent = 0) const override {
                std::string spaces = getIndent(indent);
                out << spaces << "<div class='node redirection'>\n";
                out << spaces << "  <h3>Redirection</h3>\n";
                out << spaces << "  <table>\n";
                out << spaces << "    <tr><th>Type</th><td class='symbol'>";
                
                if (type == INPUT) {
                    out << "&lt; (Input)";
                } else if (type == OUTPUT) {
                    out << "&gt; (Output)";
                } else {
                    out << "&gt;&gt; (Append)";
                }
                
                out << "</td></tr>\n";
                out << spaces << "    <tr><th>File</th><td class='filename'>" << escapeHtml(file) << "</td></tr>\n";
                out << spaces << "    <tr><th>Command</th><td>\n";
                command->print(out, indent + 6);
                out << spaces << "    </td></tr>\n";
                out << spaces << "  </table>\n";
                out << spaces << "</div>\n";
            }
        std::shared_ptr<Node> command;
        Type type;
        std::string file;
    };


    class Sequence : public Node {
    public:
        Sequence(std::vector<std::shared_ptr<Node>> commands) 
            : commands(std::move(commands)) {}
    
        void print(std::ostream& out, int indent = 0) const override {
            std::string spaces = getIndent(indent);
            out << spaces << "<div class='node sequence'>\n";
            out << spaces << "  <h3>Sequence</h3>\n";
            out << spaces << "  <table>\n";
            out << spaces << "    <tr><th>Commands</th></tr>\n";
            for (size_t i = 0; i < commands.size(); i++) {
                out << spaces << "    <tr><td>\n";
                commands[i]->print(out, indent + 6);
                out << spaces << "    </td></tr>\n";
            }
            out << spaces << "  </table>\n";
            out << spaces << "</div>\n";
        }
        std::vector<std::shared_ptr<Node>> commands;
    };

    using CommandFunction = std::function<int(const std::vector<std::string>&, std::istream&, std::ostream&)>;
    using TypedCommandFunction = std::function<int(const std::vector<Argument>&, std::istream&, std::ostream&)>;

    class CommandRegistry {
    public:
    
        void registerCommand(const std::string& name, CommandFunction func) {
            commands[name] = func;
        }

        void registerTypedCommand(const std::string& name, TypedCommandFunction func) {
            typedCommands[name] = func;
        }
        
        int executeCommand(const std::string& name, const std::vector<Argument>& args, 
                          std::istream& input, std::ostream& output) {
            
            auto it = typedCommands.find(name);
            if (it != typedCommands.end()) {
                return it->second(args, input, output);
            }
            
            
            auto it2 = commands.find(name);
            if (it2 != commands.end()) {
                std::vector<std::string> argValues;
                for (const auto& arg : args) {
                    argValues.push_back(arg.value);
                }
                return it2->second(argValues, input, output);
            }
            output << "Command not found: " << name << std::endl;
            return 1; 
        }
        
    private:
        std::unordered_map<std::string, CommandFunction> commands;
        std::unordered_map<std::string, TypedCommandFunction> typedCommands;
    };

    class StringLiteral : public Expression {
    public:
        StringLiteral(std::string value) : value(std::move(value)) {}
        
        void print(std::ostream& out, int indent = 0) const override {
            std::string spaces = getIndent(indent);
            out << spaces << "<div class='node string-literal'>\n";
            out << spaces << "  <h3>StringLiteral</h3>\n";
            out << spaces << "  <table>\n";
            out << spaces << "    <tr><th>Value</th></tr>\n";
            out << spaces << "    <tr><td class='literal'>" << escapeHtml(value) << "</td></tr>\n";
            out << spaces << "  </table>\n";
            out << spaces << "</div>\n";
        }
        
        std::string evaluate(const AstExecutor& executor) const override {
            return value;
        }
        
        double evaluateNumber(const AstExecutor& executor) const override {
            try {
                return std::stod(value);
            } catch (const std::exception&) {
                return 0.0;  
            }
        }
        
        std::string value;
    };
    
    class VariableAssignment : public Node {
    public:
        VariableAssignment(std::string name, std::shared_ptr<Node> value)
            : name(std::move(name)), value(std::move(value)) {}
        
        void print(std::ostream& out, int indent = 0) const override {
            std::string spaces = getIndent(indent);
            out << spaces << "<div class='node variable-assignment'>\n";
            out << spaces << "  <h3>VariableAssignment</h3>\n";
            out << spaces << "  <table>\n";
            out << spaces << "    <tr><th>Name</th><th>Value</th></tr>\n";
            out << spaces << "    <tr><td class='variable'>" << escapeHtml(name) << "</td><td>\n";
            value->print(out, indent + 6);
            out << spaces << "    </td></tr>\n";
            out << spaces << "  </table>\n";
            out << spaces << "</div>\n";
        }
        
        std::string name;
        std::shared_ptr<Node> value;
    };
    class NumberLiteral : public Expression {
    public:
        NumberLiteral(double value) : value(value) {}
        
        void print(std::ostream& out, int indent = 0) const override {
            std::string spaces = getIndent(indent);
            out << spaces << "<div class='node number-literal'>\n";
            out << spaces << "  <h3>NumberLiteral</h3>\n";
            out << spaces << "  <table>\n";
            out << spaces << "    <tr><th>Value</th></tr>\n";
            out << spaces << "    <tr><td class='literal'>" << value << "</td></tr>\n";
            out << spaces << "  </table>\n";
            out << spaces << "</div>\n";
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
        
        void print(std::ostream& out, int indent = 0) const override {
            std::string spaces = getIndent(indent);
            out << spaces << "<div class='node variable-reference'>\n";
            out << spaces << "  <h3>VariableReference</h3>\n";
            out << spaces << "  <table>\n";
            out << spaces << "    <tr><th>Name</th></tr>\n";
            out << spaces << "    <tr><td class='variable'>" << escapeHtml(name) << "</td></tr>\n";
            out << spaces << "  </table>\n";
            out << spaces << "</div>\n";
        }
        
        std::string evaluate(const AstExecutor& executor) const override;
        double evaluateNumber(const AstExecutor& executor) const override;
        
        std::string name;
    };

    class AstExecutor {
    public:
        AstExecutor();

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
                #ifdef DEBUG_MODE_ON
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
                    }  else {
                        return input;
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
            else if (auto ifStmt = std::dynamic_pointer_cast<cmd::IfStatement>(node)) {
                executeIfStatement(ifStmt, input, output);
            }
            else if (auto whileStmt = std::dynamic_pointer_cast<cmd::WhileStatement>(node)) {
                executeWhileStatement(whileStmt, input, output);
            }
            else if (auto forStmt = std::dynamic_pointer_cast<cmd::ForStatement>(node)) {
                executeForStatement(forStmt, input, output);
            }
            else {
                throw std::runtime_error("Unknown node type");
            }
        }
        
        void executeCommand(const std::shared_ptr<cmd::Command>& cmd, std::istream& input, std::ostream& output) {
            lastExitStatus = registry.executeCommand(cmd->name, cmd->args, input, output);
            if (lastExitStatus != 0 && cmd->name != "test") {
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

        void executeVariableAssignment(const std::shared_ptr<cmd::VariableAssignment>& varAssign, std::istream& input, std::ostream& output) {   
            try {
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
                lastExitStatus = 0;
            }
            catch (const std::exception& e) {
                output << "Assignment error: " << e.what() << std::endl;
                lastExitStatus = 1; 
            }
        }

        void executeLogicalAnd(const std::shared_ptr<cmd::LogicalAnd>& logicalAnd, std::istream& input, std::ostream& output) {
            executeNode(logicalAnd->left, input, output);
            if (lastExitStatus == 0) {
                executeNode(logicalAnd->right, input, output);
            }
        }

        void executeIfStatement(const std::shared_ptr<cmd::IfStatement>& ifStmt, 
                                std::istream& input, std::ostream& output) {
            for (const auto& branch : ifStmt->branches) {
                executeNode(branch.condition, input, output);
                if (lastExitStatus == 0) {
                    executeNode(branch.action, input, output);
                    return;
                }
            }
            if (ifStmt->elseAction) {
                executeNode(ifStmt->elseAction, input, output);
            }
        }

        void executeWhileStatement(const std::shared_ptr<cmd::WhileStatement>& whileStmt,
                                   std::istream& input, std::ostream& output) {
            while (true) {
                executeNode(whileStmt->condition, input, output);
                if (lastExitStatus != 0) {
                    break;
                }
                executeNode(whileStmt->body, input, output);
            }
        }

        void executeForStatement(const std::shared_ptr<cmd::ForStatement>& forStmt,
                                 std::istream& input, std::ostream& output) {
            
            std::optional<std::string> originalValue = getVariable(forStmt->variable);
            bool hadOriginalValue = originalValue.has_value();
            for (const auto& value : forStmt->values) {
                if (value.type == ARG_VARIABLE) {
                    std::optional<std::string> varValue = getVariable(value.value);
                    
                    if (varValue.has_value()) {
                        std::string valueStr = varValue.value();
                        
                    
                        if (valueStr.find('\n') != std::string::npos) {
                    
                            std::istringstream lineStream(valueStr);
                            std::string line;
                            while (std::getline(lineStream, line)) {
                                if (!line.empty()) {
                                    setVariable(forStmt->variable, line);
                                    executeNode(forStmt->body, input, output);
                                }
                            }
                        } else if (valueStr.find_first_of(" \t") != std::string::npos) {
                            std::istringstream wordStream(valueStr);
                            std::string word;
                            while (wordStream >> word) {
                                setVariable(forStmt->variable, word);
                                executeNode(forStmt->body, input, output);
                            }
                        } else {
                            setVariable(forStmt->variable, valueStr);
                            executeNode(forStmt->body, input, output);
                        }
                    }
                } else {
                    std::string literalValue = value.value;
                    if (literalValue.size() >= 2 && 
                        ((literalValue.front() == '"' && literalValue.back() == '"') ||
                         (literalValue.front() == '\'' && literalValue.back() == '\''))) {
                        
                        literalValue = literalValue.substr(1, literalValue.size() - 2);
                        if (literalValue.find('\n') != std::string::npos) {
                            std::istringstream lineStream(literalValue);
                            std::string line;
                            while (std::getline(lineStream, line)) {
                                if (!line.empty()) {
                                    setVariable(forStmt->variable, line);
                                    executeNode(forStmt->body, input, output);
                                }
                            }
                            continue;
                        }
                    }
            
                    setVariable(forStmt->variable, literalValue);
                    executeNode(forStmt->body, input, output);
                }
            }
            
            
            if (hadOriginalValue) {
                setVariable(forStmt->variable, originalValue.value());
            } else {
                try {
                    state::GameState* gameState = state::getGameState();
                    gameState->setVariable(forStmt->variable, "");
                } catch (...) {
                    setVariable(forStmt->variable, "");
                }
            }
        }
    };


    class BinaryExpression : public Expression {
    public:
        enum OpType { ADD, SUBTRACT, MULTIPLY, DIVIDE, MODULO };
        
        BinaryExpression(std::shared_ptr<Expression> left, OpType op, std::shared_ptr<Expression> right)
            : left(std::move(left)), op(op), right(std::move(right)) {}
        
        void print(std::ostream& out, int indent = 0) const override {
            std::string spaces = getIndent(indent);
            out << spaces << "<div class='node binary-expression'>\n";
            out << spaces << "  <h3>BinaryExpression</h3>\n";
            out << spaces << "  <table>\n";
            out << spaces << "    <tr><th>Operator</th><td class='operator'>";
            switch (op) {
                case ADD: out << "+"; break;
                case SUBTRACT: out << "-"; break;
                case MULTIPLY: out << "*"; break;
                case DIVIDE: out << "/"; break;
                case MODULO: out << "%"; break;
            }
            out << "</td></tr>\n";
            out << spaces << "    <tr><th>Left Operand</th><td>\n";
            left->print(out, indent + 6);
            out << spaces << "    </td></tr>\n";
            out << spaces << "    <tr><th>Right Operand</th><td>\n";
            right->print(out, indent + 6);
            out << spaces << "    </td></tr>\n";
            out << spaces << "  </table>\n";
            out << spaces << "</div>\n";
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
        
        void print(std::ostream& out, int indent = 0) const override {
            std::string spaces = getIndent(indent);
            out << spaces << "<div class='node unary-expression'>\n";
            out << spaces << "  <h3>UnaryExpression</h3>\n";
            out << spaces << "  <table>\n";
            out << spaces << "    <tr><th>Operator</th><td class='operator'>";
            switch (op) {
                case INCREMENT: out << "++"; break;
                case DECREMENT: out << "--"; break;
                case NEGATE: out << "-"; break;
            }
            out << "</td></tr>\n";
            out << spaces << "    <tr><th>Position</th><td class='position'>" 
                << (position == PREFIX ? "prefix" : "postfix") << "</td></tr>\n";
            out << spaces << "    <tr><th>Operand</th><td>\n";
            operand->print(out, indent + 6);
            out << spaces << "    </td></tr>\n";
            out << spaces << "  </table>\n";
            out << spaces << "</div>\n";
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
        
        void print(std::ostream& out, int indent = 0) const override {
            std::string spaces = getIndent(indent);
            out << spaces << "<div class='node command-substitution'>\n";
            out << spaces << "  <h3>CommandSubstitution</h3>\n";
            out << spaces << "  <table>\n";
            out << spaces << "    <tr><th>Command</th></tr>\n";
            out << spaces << "    <tr><td>\n";
            command->print(out, indent + 6);
            out << spaces << "    </td></tr>\n";
            out << spaces << "  </table>\n";
            out << spaces << "</div>\n";
        }
        
        std::string evaluate(const AstExecutor& executor) const override {
            std::stringstream input;  
            std::stringstream output;
            
            if (auto cmd = std::dynamic_pointer_cast<cmd::Command>(command)) {
                std::vector<Argument> expandedArgs;
                for (const auto& arg : cmd->args) {
                    Argument expandedArg;
                    expandedArg.value = executor.expandVariables(arg.value);
                    expandedArg.type = arg.type;
                    expandedArgs.push_back(expandedArg);
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
        
        std::shared_ptr<Node> command;
    };

   
}

#endif
