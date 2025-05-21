#include"command_reg.hpp"
#include"ast.hpp"
#include<vector>

namespace cmd {

    void CommandRegistry::registerCommand(const std::string& name,
                                        CommandFunction func)    {
        commands[name] = func;
    }

    void CommandRegistry::registerTypedCommand(const std::string& name,
                                            TypedCommandFunction func) {
        typedCommands[name] = func;
    }

    void CommandRegistry::registerUserDefinedCommand(
        const std::string& name,
        const UserDefinedCommandInfo& info) {
        userDefinedCommands[name] = info;
    }

    bool CommandRegistry::isUserDefinedCommand(const std::string& name) const {
        return userDefinedCommands.find(name) != userDefinedCommands.end();
    }

    int CommandRegistry::executeCommand(const std::string& name,
                                        const std::vector<Argument>& args,
                                        std::istream& input,
                                        std::ostream& output) {

        auto it_typed = typedCommands.find(name);
        if (it_typed != typedCommands.end()) {
            return it_typed->second(args, input, output);
        }

        auto it_str = commands.find(name);
        if (it_str != commands.end()) {
            std::vector<std::string> argValues;
            argValues.reserve(args.size());
            for (auto const & arg : args) {
                argValues.push_back(arg.value);
            }
            return it_str->second(argValues, input, output);
        }

        auto it_ud = userDefinedCommands.find(name);
        if (it_ud != userDefinedCommands.end()) {
            return executeUserDefinedCommand(name, it_ud->second,
                                            args, input, output);
        }

        auto it_ext = externCommands.find(name);
        if (it_ext != externCommands.end()) {
            return executeExternCommand(name, args, input, output);
        }
        
        throw std::runtime_error("Command not found: " + name);
        return 1;
    }

    void CommandRegistry::registerExternCommand(const std::string& name, const ExternCommandInfo& info) {
        auto it = commands.find(name);
        if (it != commands.end()) {
            throw std::runtime_error("Command " + name + " already registered");
        }
        externCommands[name] = info;
    }
        
    int CommandRegistry::executeExternCommand(const std::string& name, const std::vector<Argument>& args, 
                                std::istream& input, std::ostream& output) {
        auto it = externCommands.find(name);
        if (it != externCommands.end()) {
            const auto& info = it->second;
            if (info.func) {
                return info.func(args, input, output);
            } 
            else {
                output << "Function: " << info.functionName << " not found in library: " << info.libraryPath << "\n";
                return 1;
            }
        }
            return 0;
    }
    
    int CommandRegistry::executeUserDefinedCommand(
        const std::string& name,
        const UserDefinedCommandInfo& info,
        const std::vector<Argument>& args,
        std::istream& input,
        std::ostream& output) {
        
        state::GameState* gameState = state::getGameState();
        std::unordered_map<std::string, std::optional<std::string>> origValues;

        for (size_t i = 0; i < std::min(args.size(), info.parameters.size()); ++i) {
            const auto& paramName = info.parameters[i];
        
            try {
                origValues[paramName] = gameState->getVariable(paramName);
            }
            catch (const state::StateException&) {
                origValues[paramName] = std::nullopt;
            }

            try {
                if (args[i].type == ARG_VARIABLE) {
                    std::string val;
                    try {
                        val = gameState->getVariable(args[i].value);
                    }
                    catch (...) {
                        val.clear();
                    }
                    gameState->setVariable(paramName, val);
                }
                else if (args[i].type == ARG_COMMAND_SUBST && args[i].cmdNode) {
                    AstExecutor executor;
                    std::stringstream cmdInput, cmdOutput;
                    executor.executeDirectly(args[i].cmdNode, cmdInput, cmdOutput);
                    std::string result = cmdOutput.str();
                    while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
                        result.pop_back();
                    }
                    gameState->setVariable(paramName, result);
                }
                else {
                    gameState->setVariable(paramName, args[i].value);
                }
            }
            catch (const std::exception& e) {
                for (auto const & kv : origValues) {
                    if (kv.second.has_value())
                        gameState->setVariable(kv.first, *kv.second);
                    else
                        gameState->clearVariable(kv.first);
                }
                output << name << ": error setting parameter '"
                    << paramName << "': " << e.what() << "\n";
                return 1;
            }
        }

        int result = 0;
        try {
            AstExecutor &executor = AstExecutor::getExecutor();
            for (auto const & kv : userDefinedCommands) {
                executor.getRegistry()
                        .registerUserDefinedCommand(kv.first, kv.second);
            }
            executor.execute(input, output, info.body);
            result = executor.getLastExitStatus();
        }
        catch (const std::exception& e) {
            output << name << ": execution failed: " << e.what() << "\n";
            result = 1;
        }
        for (auto const & kv : origValues) {
            if (kv.second.has_value())
                gameState->setVariable(kv.first, *kv.second);
            else
                gameState->clearVariable(kv.first);
        }
        return result;
    }

    std::shared_ptr<Library> &CommandRegistry::getLibrary(const std::string& name){
        auto it = libraries.find(name);
        if (it != libraries.end()) {
            return it->second;
        }
        throw std::runtime_error("Library not found: " + name);
    }
    std::shared_ptr<Library> &CommandRegistry::setLibrary(const std::string& name) {
        auto it = libraries.find(name);
        if (it != libraries.end()) {
            return it->second;
        }
        libraries[name] = std::make_shared<cmd::Library>(name);
        return libraries[name];
    }

    void CommandRegistry::printInfo(std::ostream &out) {
        auto listSorted = [](std::ostream  &o, auto &l) -> void {
            std::vector<std::string> lst;
            for(auto &f: l) {
                lst.push_back(f.first);
            }
            std::sort(lst.begin(), lst.end());
            for(auto &i : lst) {
                o << "\t" << i << "\n";
            }
        };
        out << "Commands {\n";
        listSorted(out, this->commands);
        out << "}\n";
        out << "Typed Commands {\n";
        listSorted(out, this->typedCommands);
        out << "}\n";
        out << "User Defined Commands {\n";
        listSorted(out, this->userDefinedCommands);
        out << "}\n";
        out << "Commands from Libraries {\n";
        listSorted(out, this->externCommands);
        out << "}\n";
    }

    bool CommandRegistry::empty() const {
        return commands.empty() && typedCommands.empty() && userDefinedCommands.empty();
    }

}