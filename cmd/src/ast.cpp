#include"ast.hpp"
#include"command.hpp"

namespace cmd {

    AstExecutor::AstExecutor() {
        if(registry.empty()) {
            registry.registerTypedCommand("echo", cmd::echoCommand);
            registry.registerTypedCommand("test",cmd::testCommand);
            registry.registerTypedCommand("cmd", cmd::cmdCommand);
            registry.registerTypedCommand("cat", cmd::catCommand);
            registry.registerTypedCommand("grep", cmd::grepCommand);
            registry.registerCommand("exit", cmd::exitCommand);
            registry.registerCommand("quit", cmd::exitCommand);
            registry.registerCommand("print", cmd::printCommand);
            registry.registerTypedCommand("cd", cmd::cdCommand);
            registry.registerTypedCommand("ls", cmd::listCommand);
            registry.registerTypedCommand("dir", cmd::listCommand);
            registry.registerTypedCommand("find", cmd::findCommand);
            registry.registerTypedCommand("sort", cmd::sortCommand);
            registry.registerCommand("pwd", cmd::pwdCommand);
            registry.registerTypedCommand("mkdir", cmd::mkdirCommand);
            registry.registerTypedCommand("cp", cmd::cpCommand);
            registry.registerTypedCommand("mv", cmd::mvCommand);
            registry.registerTypedCommand("touch", cmd::touchCommand);
            registry.registerCommand("head", cmd::headCommand);
            registry.registerCommand("tail", cmd::tailCommand);
            registry.registerTypedCommand("wc", cmd::wcCommand);
            registry.registerTypedCommand("sed", cmd::sedCommand);
            registry.registerTypedCommand("printf", cmd::printfCommand);
            registry.registerTypedCommand("debug_set", cmd::debugSet);
            registry.registerTypedCommand("debug_get", cmd::debugGet);
            registry.registerCommand("debug_list", cmd::debugList);
            registry.registerTypedCommand("debug_clear", cmd::debugClear);
            registry.registerCommand("debug_clear_all", cmd::debugClearAll);
            registry.registerCommand("debug_search", cmd::debugSearch);
            registry.registerCommand("debug_dump", cmd::dumpVariables);
            registry.registerTypedCommand("visual", cmd::visualCommand);
            registry.registerTypedCommand("at", cmd::atCommand);
            registry.registerTypedCommand("len", cmd::lenCommand);
            registry.registerTypedCommand("index", cmd::indexCommand);
            registry.registerTypedCommand("strlen", cmd::strlenCommand);
            registry.registerTypedCommand("strfind", cmd::strfindCommand);
            registry.registerTypedCommand("strfindr", cmd::strfindrCommand);
            registry.registerTypedCommand("strtok", cmd::strtokCommand);
            registry.registerTypedCommand("exec", cmd::execCommand);
            registry.registerTypedCommand("cmdlist", cmd::commandListCommand);
            registry.registerTypedCommand("debug_cmd", cmd::commandListCommand);
            registry.registerTypedCommand("set", [this](const std::vector<Argument>& args, std::istream& input, std::ostream& output) {
                if (args.size() >= 1) {
                    if (args[0].value == "-e") {
                        setTerm(true);
                        output << "Early termination enabled" << std::endl;
                        return 0;
                    } else if (args[0].value == "+e") {
                        setTerm(false);
                        output << "Early termination disabled" << std::endl;
                        return 0;
                    }
                }
                output << "Usage: set [-e|+e] (enable or disable early termination)" << std::endl;
                return 1;
            });
        }
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

    double UnaryExpression::evaluateNumber(const AstExecutor& executor) const {
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

    CommandRegistry AstExecutor::registry;

}