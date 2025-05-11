
#ifndef _CMD_REG_H_
#define _CMD_REG_H_

#include<string>
#include<functional>
#include<unordered_map>
#include<optional>
#include<vector>    
#include<memory>

namespace cmd {

    
    class Node;
    class Argument;

    using CommandFunction = std::function<int(const std::vector<std::string>&, std::istream&, std::ostream&)>;
    using TypedCommandFunction = std::function<int(const std::vector<Argument>&, std::istream&, std::ostream&)>;

    class CommandRegistry {
    public:
        struct UserDefinedCommandInfo {
            std::vector<std::string> parameters;
            std::shared_ptr<Node> body;
        };

        void registerCommand(const std::string& name, CommandFunction func);
        void registerTypedCommand(const std::string& name, TypedCommandFunction func);
        void registerUserDefinedCommand(const std::string& name, const UserDefinedCommandInfo& info);
        int executeCommand(const std::string& name, const std::vector<Argument>& args, 
                          std::istream& input, std::ostream& output);

        void printInfo(std::ostream &out);

    private:
        int executeUserDefinedCommand(const std::string& name, const UserDefinedCommandInfo& info,
                                    const std::vector<Argument>& args, 
                                    std::istream& input, std::ostream& output);

        std::unordered_map<std::string, CommandFunction> commands;
        std::unordered_map<std::string, TypedCommandFunction> typedCommands;
        std::unordered_map<std::string, UserDefinedCommandInfo> userDefinedCommands;
    };
}

#endif