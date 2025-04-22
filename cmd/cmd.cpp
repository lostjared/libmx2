#include"ast.hpp"
#include"parser.hpp"
#include"scanner.hpp"
#include"types.hpp"
#include"string_buffer.hpp"
#include<iostream>
#include<string>
#include<fstream>
#include<optional>
#include<vector>
#include<memory>
#include<functional>
#include<unordered_map>

int main(int argc, char **argv) {
    bool active = true;
    try {
        cmd::CommandRegistry registry;
        registry.registerCommand("echo", cmd::echoCommand);
        registry.registerCommand("cat", cmd::catCommand);
        registry.registerCommand("grep", cmd::grepCommand);
        registry.registerCommand("exit", cmd::exitCommand);
        cmd::AstExecutor executor(registry);

        while(active) {
            std::string command_data;
            std::cout << "=)> ";
            std::getline(std::cin, command_data);
            scan::TString string_buffer(command_data);
            scan::Scanner scanner(string_buffer);
            cmd::Parser parser(scanner);
            auto ast = parser.parse();
            executor.execute(ast);
        }
        
    } catch (const scan::ScanExcept &e) {
        std::cerr << "Scan error: " << e.why() << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
    }   
    return 0;
}