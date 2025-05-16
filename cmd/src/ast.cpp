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
            registry.registerTypedCommand("stringf", cmd::printfCommand);
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
            registry.registerTypedCommand("argv", cmd::argvCommand);
            registry.registerTypedCommand("extern", cmd::externCommand);
            registry.registerTypedCommand("list_new", cmd::newListCommand);
            registry.registerTypedCommand("list_add", cmd::newListAddCommand);
            registry.registerTypedCommand("list_remove", cmd::newListRemoveCommand); 
            registry.registerTypedCommand("list_get", cmd::newListGetCommand);
            registry.registerTypedCommand("list_clear", cmd::newListClearCommand);
            registry.registerTypedCommand("list_add", cmd::newListAddCommand);
            registry.registerTypedCommand("list_remove", cmd::newListRemoveCommand);
            registry.registerTypedCommand("list_set", cmd::newListSetCommand);
            registry.registerTypedCommand("list_erase", cmd::newListClearCommand);
            registry.registerTypedCommand("list_clear", cmd::newListClearAllCommand);
            registry.registerTypedCommand("list_exists", newListExistsCommand);
            registry.registerTypedCommand("list_init", cmd::newListInitCommand);
            registry.registerTypedCommand("rand", cmd::newRandCommand);
            registry.registerTypedCommand("list_len", cmd::newListLenCommand);
            registry.registerTypedCommand("expr", cmd::exprCommand);
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

     void AstExecutor::executeNode(const std::shared_ptr<cmd::Node>& node, std::istream& input, std::ostream& output) {
        if (returnSignal) {
            return;
        }

        if (auto returnStmt = std::dynamic_pointer_cast<cmd::Return>(node)) {
            executeReturn(returnStmt, input, output);
        } else if (auto cmdDef = std::dynamic_pointer_cast<cmd::CommandDefinition>(node)) {
            executeCommandDefinition(cmdDef, input, output);
        } else if (auto cmd = std::dynamic_pointer_cast<cmd::Command>(node)) {
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
        else if (auto logicalNot = std::dynamic_pointer_cast<LogicalNot>(node)) {
            executeLogicalNot(logicalNot, input, output);
        }
        else if (auto logicalOr = std::dynamic_pointer_cast<cmd::LogicalOr>(node)) {
            executeLogicalOr(logicalOr, input, output);
        }
        else if (auto expr = std::dynamic_pointer_cast<cmd::Expression>(node)) {
            if (auto unaryExpr = std::dynamic_pointer_cast<cmd::UnaryExpression>(expr)) {
                unaryExpr->evaluateNumber(*this); 
            } else {
                expr->evaluate(*this);  
            }
        }
        else if (auto cmdSubst = std::dynamic_pointer_cast<cmd::CommandSubstitution>(node)) {
            output << cmdSubst->evaluate(*this);
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
        else if (auto breakStmt = std::dynamic_pointer_cast<cmd::Break>(node)) {
            throw BreakException();
        }
        else if (auto continueStmt = std::dynamic_pointer_cast<cmd::Continue>(node)) {
            throw ContinueException();
        }
        else {
            throw std::runtime_error("Unknown node type");
        }
    }
        

    CommandRegistry AstExecutor::registry;

     std::string parseEscapeSequences(const std::string& input) {
        std::string result;
        for (size_t i = 0; i < input.length(); ++i) {
            if (input[i] == '\\' && i + 1 < input.length()) {
                switch (input[i + 1]) {
                    case 'n': result += '\n'; break;
                    case 't': result += '\t'; break;
                    case 'r': result += '\r'; break;
                    case 'b': result += '\b'; break;
                    case 'f': result += '\f'; break;
                    case 'v': result += '\v'; break;
                    case 'a': result += '\a'; break;
                    case '\\': result += '\\'; break;
                    case '\'': result += '\''; break;
                    case '"': result += '"'; break;
                    case '0': result += '\0'; break;
                    default: result += input[i + 1];
                }
                ++i;
            } else {
                result += input[i];
            }
        }
        return result;
    }

    std::string getVar(const Argument &arg) {
        std::string a = arg.value;
        if(arg.type == ArgType::ARG_VARIABLE) {
            try {
                state::GameState  *s = state::getGameState();
                a = s->getVariable(arg.value);
                if(!a.empty())
                    a = parseEscapeSequences(a);
                
            } catch(const state::StateException &) {
                throw std::runtime_error("Variable name: " + arg.value + " not found");
            }
        } else if (arg.type == ARG_COMMAND_SUBST && arg.cmdNode) {
            AstExecutor executor;
            std::stringstream input, output;
            executor.executeDirectly(arg.cmdNode, input, output);
            std::string result = output.str();
            while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
                result.pop_back();
            }
            return result;
        }
        
        return a;
    }

}