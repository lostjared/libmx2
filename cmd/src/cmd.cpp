#include"ast.hpp"
#include"parser.hpp"
#include"scanner.hpp"
#include"types.hpp"
#include"string_buffer.hpp"
#include"html.hpp"
#include<iostream>
#include<string>
#include<fstream>
#include<optional>
#include<vector>
#include<memory>
#include<functional>
#include<unordered_map>
#include<sstream>
#include<readline/readline.h>
#include<readline/history.h>



int main(int argc, char **argv) {

    if(argc == 1) {
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
                            std::cout << "\033[2J\033[1;1H"; 
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
                            ast->print(std::cout);
                        }
                    }
                    
                } catch (const scan::ScanExcept &e) {
                    std::cerr << "Scan error: " << e.why() << std::endl;
                } catch (const std::exception &e) {
                    std::cerr << "Error: " << e.what() << std::endl;
                }  catch (const state::StateException &e) {
                    std::cerr << "State error: " << e.what() << std::endl;;
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
    } else {

        cmd::AstExecutor executor{};
        bool debug_cmd = false;
        
        if(argc == 2 || argc == 3) {
            std::ostringstream stream;
            std::fstream file;
            if(argc == 3) {
                if(std::string(argv[2]) == "-d") {
                    debug_cmd = true;
                }
            }
            file.open(argv[1], std::ios::in);
            if(!file.is_open()) {
                std::cerr << "Error loading file: " << argv[1] << "\n";;
                return EXIT_FAILURE;
            }
            stream << file.rdbuf();
            try {
                scan::TString string_buffer(stream.str());
                scan::Scanner scanner(string_buffer);
                cmd::Parser parser(scanner);
                auto ast = parser.parse();
                executor.execute(std::cin, std::cout, ast);
                if(debug_cmd) {
                    std::cout << "Debug Information written to: debug.html\n\n";
                    std::ofstream html_file("debug.html");
                    if(html_file.is_open()) {
                        html::gen_html(html_file, ast);
                        html_file.close();
                    }
                }
            } catch(const scan::ScanExcept &e) {
                std::cerr << "Scan Error: " << e.why() << std::endl;
                return EXIT_FAILURE;
            } catch(const std::exception &e) {
                std::cerr << "Exception: " << e.what() << std::endl;
                return EXIT_FAILURE;
            } catch(...) {
                std::cerr << "Unknown Error has Occoured..\n";
                return EXIT_FAILURE;
            }
        }

    }   
    return 0;
}