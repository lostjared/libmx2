#include"ast.hpp"
extern "C" {
    int HeyCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream& output) {
        int count = 0;
        for(auto &arg : args) {
            try {
                output << count++ << " " << getVar(arg) << "\n";
            } catch(const std::runtime_error &) {
                output << arg.value << " ";
            }
        }
        return 0;
    }
}