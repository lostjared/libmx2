#include"linenoise.h"
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

    char *line = nullptr;
    bool active = true;

    try {
        cmd::AstExecutor executor{};
        linenoiseHistoryLoad("cmd-history.txt");
        while(active) {
            try {
                std::string prompt = executor.getPath() + "> ";
                line = linenoise(prompt.c_str());
                std::string command_data(line);
                free(line);
                linenoiseHistoryAdd(command_data.c_str());
                linenoiseHistorySave("cmd-history.txt");
                scan::TString string_buffer(command_data);
                scan::Scanner scanner(string_buffer);
                cmd::Parser parser(scanner);
                auto ast = parser.parse();
                executor.execute(ast);
            } catch (const scan::ScanExcept &e) {
                std::cerr << "Scan error: " << e.why() << std::endl;
            } catch (const std::exception &e) {
                std::cerr << "Error: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "Unknown error occurred." << std::endl;
            }
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