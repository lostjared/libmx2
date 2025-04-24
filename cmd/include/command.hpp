#ifndef _CMD_X_H_
#define _CMD_X_H_

#include<iostream>
#include<vector>
#include<string>
#include<sstream>

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
}
#endif