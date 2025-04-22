#ifndef _CMD_X_H_
#define _CMD_X_H_

#include<iostream>
#include<vector>
#include<string>

namespace cmd {
    void exitCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output);
    void echoCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output);
    void catCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output);
    void grepCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output);
    void printCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output);
    void cdCommand(const std::vector<std::string> &args, std::istream& input, std::ostream& output);
    void listCommand(const std::vector<std::string> &args, std::istream& input, std::ostream& output);
    void sortCommand(const std::vector<std::string> &args, std::istream& input, std::ostream& output);
    void findCommand(const std::vector<std::string> &args, std::istream& input, std::ostream& output);

    void pwdCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output);
    void mkdirCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output);
    void rmCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output);
    void cpCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output);
    void mvCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output);
    void touchCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output);
    void headCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output);
    void tailCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output);
    void wcCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output);
    void chmodCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output);
}
#endif