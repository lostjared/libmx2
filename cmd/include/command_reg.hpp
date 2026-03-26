
#ifndef _CMD_REG_H_
#define _CMD_REG_H_

#include "library.hpp"
#include "plugin_api.h"
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
namespace cmd {

    class Node;
    struct Argument;

    using CommandFunction = std::function<int(const std::vector<std::string> &, std::istream &, std::ostream &)>;
    using TypedCommandFunction = std::function<int(const std::vector<Argument> &, std::istream &, std::ostream &)>;
    using TypedCommandFunctionPtr = std::function<int(const std::vector<Argument> &, std::istream &, std::ostream &)>;

    struct ExternCommandInfo {
        std::string name;
        std::string libraryPath;
        std::string functionName;
        std::shared_ptr<Library> library;
        plugin_func_t func; // C-compatible function pointer (no C++ types cross the DLL boundary)
    };
    class CommandRegistry {
      public:
        struct UserDefinedCommandInfo {
            std::vector<std::string> parameters;
            std::shared_ptr<Node> body;
        };

        void registerCommand(const std::string &name, CommandFunction func);
        void registerTypedCommand(const std::string &name, TypedCommandFunction func);
        void registerUserDefinedCommand(const std::string &name, const UserDefinedCommandInfo &info);
        int executeCommand(const std::string &name, const std::vector<Argument> &args,
                           std::istream &input, std::ostream &output);
        void registerExternCommand(const std::string &name, const ExternCommandInfo &info);
        int executeExternCommand(const std::string &name, const std::vector<Argument> &args,
                                 std::istream &input, std::ostream &output);
        bool unregisterExternCommand(const std::string &name);
        std::size_t clearExternCommands();
        std::size_t pruneLibraries();

        void printInfo(std::ostream &out);
        bool empty() const;
        bool isUserDefinedCommand(const std::string &name) const;

        std::shared_ptr<Library> &getLibrary(const std::string &name);
        std::shared_ptr<Library> &setLibrary(const std::string &name);

      private:
        int executeUserDefinedCommand(const std::string &name, const UserDefinedCommandInfo &info,
                                      const std::vector<Argument> &args,
                                      std::istream &input, std::ostream &output);

        std::unordered_map<std::string, CommandFunction> commands;
        std::unordered_map<std::string, TypedCommandFunction> typedCommands;
        std::unordered_map<std::string, UserDefinedCommandInfo> userDefinedCommands;
        std::unordered_map<std::string, ExternCommandInfo> externCommands;
        std::unordered_map<std::string, std::shared_ptr<cmd::Library>> libraries;
    };
} // namespace cmd

#endif
