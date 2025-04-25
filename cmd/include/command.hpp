#ifndef _CMD_X_H_
#define _CMD_X_H_
#include<iostream>
#include<vector>
#include<string>
#include<sstream>
#include<unordered_map>

namespace state {

    class StateException {
        public:
            StateException(const std::string& message) : message(message) {}
            const char* what() const noexcept { return message.c_str(); }
        private:
            std::string message;
    };

    class GameState {
    public:
        GameState() = default;
        ~GameState() = default;
        void setVariable(const std::string& name, const std::string& value) {
            variables[name] = value;
        }
        std::string getVariable(const std::string& name) const {
            auto it = variables.find(name);
            if (it != variables.end()) {
                return it->second;
            }
            throw StateException("Variable not found: " + name);
        }
    protected:
       std::unordered_map<std::string, std::string> variables;
    };

    GameState *getGameState();
}




namespace cmd {
    int exitCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output);
    int echoCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output);
    int catCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output);
    int grepCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output);
    int printCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output);
    int cdCommand(const std::vector<std::string> &args, std::istream& input, std::ostream& output);
    int listCommand(const std::vector<std::string> &args, std::istream& input, std::ostream& output);
    int sortCommand(const std::vector<std::string> &args, std::istream& input, std::ostream& output);
    int findCommand(const std::vector<std::string> &args, std::istream& input, std::ostream& output);
    int pwdCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output);
    int mkdirCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output);
    int rmCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output);
    int cpCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output);
    int mvCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output);
    int touchCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output);
    int headCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output);
    int tailCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output);
    int wcCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output);
    int chmodCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output);
    int sedCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output);
    int debugSet(const std::vector<std::string>& args, std::istream& input, std::ostream& output);
    int debugGet(const std::vector<std::string>& args, std::istream& input, std::ostream& output);
}
#endif