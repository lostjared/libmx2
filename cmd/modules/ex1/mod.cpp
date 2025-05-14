#include"ast.hpp"
extern "C" {
    int HelloWorldCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream& output) {
        output << "Hello, World!" << std::endl;
        return 0;
    }
}