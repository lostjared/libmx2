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
#include <readline/readline.h>
#include <readline/history.h>

int main(int argc, char **argv) {
    bool active = true;
    bool debug_cmd = false;
    try {
        cmd::AstExecutor executor{};
        using_history();
        read_history(".cmd_history");
        while(active) {
            try {
                std::string prompt = executor.getPath() + "> ";
                char* line = readline(prompt.c_str());
                if (line == nullptr) {
                    std::cout << std::endl;  
                    break;
                }
                if (line[0] != '\0') {

                    add_history(line);
                    std::string command_data(line);
                    free(line);
                    if(command_data == "clear" || command_data == "cls") {
                        std::cout << "\033[2J\033[1;1H"; // ANSI escape code to clear the screen
                        continue;
                    } else if(command_data == "exit" || command_data == "quit") {
                        active = false;
                        continue;
                    } else if(command_data == "@debug_on") {
                        debug_cmd = true;
                        std::cout << "Debugging commands on." << std::endl;
                        continue;
                    } else if(command_data == "@debug_off") {
                        debug_cmd = false;
                        std::cout << "Debugging commands off." << std::endl;
                        continue;
                    }
                    scan::TString string_buffer(command_data);
                    scan::Scanner scanner(string_buffer);
                    cmd::Parser parser(scanner);
                    auto ast = parser.parse();
                    executor.execute(std::cin, std::cout, ast);
                    if(debug_cmd) {
                        ast->print();
                    }
                }
                
            } catch (const scan::ScanExcept &e) {
                std::cerr << "Scan error: " << e.why() << std::endl;
            } catch (const std::exception &e) {
                std::cerr << "Error: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "Unknown error occurred." << std::endl;
            }
        }
        write_history(".cmd_history");
    } catch (const scan::ScanExcept &e) {
        std::cerr << "Scan error: " << e.why() << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
    }   
    return 0;
}