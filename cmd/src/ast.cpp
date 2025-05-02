#include"ast.hpp"
#include"command.hpp"

namespace cmd {

    AstExecutor::AstExecutor() {
        registry.registerTypedCommand("echo", cmd::echoCommand);
        registry.registerTypedCommand("test",cmd::testCommand);
        registry.registerTypedCommand("cmd", cmd::cmdCommand);
        registry.registerTypedCommand("cat", cmd::catCommand);
        registry.registerTypedCommand("grep", cmd::grepCommand);
        registry.registerCommand("exit", cmd::exitCommand);
        registry.registerCommand("print", cmd::printCommand);
        registry.registerTypedCommand("cd", cmd::cdCommand);
        registry.registerTypedCommand("ls", cmd::listCommand);
        registry.registerTypedCommand("dir", cmd::listCommand);
        registry.registerTypedCommand("find", cmd::findCommand);
        registry.registerTypedCommand("sort", cmd::sortCommand);
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
        registry.registerTypedCommand("printf", cmd::printfCommand);
        registry.registerCommand("debug_set", cmd::debugSet);
        registry.registerCommand("debug_get", cmd::debugGet);
        registry.registerCommand("debug_list", cmd::debugList);
        registry.registerCommand("debug_clear", cmd::debugClear);
        registry.registerCommand("debug_search", cmd::debugSearch);
        registry.registerCommand("debug_dump", cmd::dumpVariables);
    }

    std::string VariableReference::evaluate(const AstExecutor& executor) const {
        auto value = executor.getVariable(name);
        return value.value_or("");
    }

    double VariableReference::evaluateNumber(const AstExecutor& executor) const {
        auto value = executor.getVariable(name);
        if (!value) return 0.0;
        
        try {
            return std::stod(value.value());
        } catch (const std::exception&) {
            return 0.0;
        }
    }
}