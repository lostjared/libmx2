#include"ast.hpp"

namespace cmd {
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