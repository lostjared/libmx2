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
        output << "Command not found: " << name << "\n";
        return 1;
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
            AstExecutor executor;
            for (auto const & kv : userDefinedCommands) {
                executor.getCommandRegistry()
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
    }

    bool CommandRegistry::empty() const {
        return commands.empty() && typedCommands.empty() && userDefinedCommands.empty();
    }

}