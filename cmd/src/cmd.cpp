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
#include<sstream>
#include<readline/readline.h>
#include<readline/history.h>

void generateHtmlAst(std::ostream& out, const std::shared_ptr<cmd::Node>& node) {
    out << "<!DOCTYPE html>\n";
    out << "<html>\n";
    out << "<head>\n";
    out << "  <meta charset='utf-8'>\n";
    out << "  <title>Shell Script AST Visualization</title>\n";
    out << "  <style>\n";
    out << "    body { font-family: 'Courier New', monospace; margin: 20px; background-color: #0a0a0a; color: #e0e0e0; }\n";
    out << "    h1, h2, h3, h4, h5 { color: #ff3333; margin: 5px 0; text-shadow: 1px 1px 2px rgba(0,0,0,0.8); }\n";
    out << "    .node { margin: 10px 0; padding: 10px; border-radius: 5px; background-color: #1a1a1a; border: 1px solid #333; }\n";
    out << "    table { border-collapse: collapse; width: auto; margin: 10px 0; border: 0px solid #444; background-color: #151515; }\n";
    out << "    th { background-color: #2a0000; color: #ff6666; text-align: left; padding: 8px; }\n";
    out << "    td { padding: 8px; border: 1px solid #333; }\n";
    out << "    .command { background-color: #1a0000; }\n";
    out << "    .pipeline { background-color: #001a1a; }\n";
    out << "    .redirection { background-color: #1a1a00; }\n";
    out << "    .sequence { background-color: #001a1a; }\n";
    out << "    .logical-and { background-color: #1a001a; }\n";
    out << "    .if-statement { background-color: #0a1a0a; }\n";
    out << "    .while-statement { background-color: #1a0a0a; }\n";
    out << "    .for-statement { background-color: #0a0a1a; }\n";
    out << "    .variable-assignment { background-color: #1a1a0a; }\n";
    out << "    .string-literal { background-color: #1a0a00; }\n";
    out << "    .number-literal { background-color: #001a0a; }\n";
    out << "    .variable-reference { background-color: #0a001a; }\n";
    out << "    .binary-expression { background-color: #1a000a; }\n";
    out << "    .unary-expression { background-color: #00001a; }\n";
    out << "    .command-substitution { background-color: #0a1a00; }\n";
    out << "    .filename { color: #ffaa33; font-style: italic; }\n";
    out << "    .operator { color: #ff9999; font-weight: bold; }\n";
    out << "    .literal { color: #66ff66; }\n";
    out << "    .variable { color: #66ffff; }\n";
    out << "    .symbol { color: #ff6666; font-weight: bold; text-align: center; }\n";
    out << "  </style>\n";
    out << "</head>\n";
    out << "<body>\n";
    out << "  <h1>Shell Script AST Visualization</h1>\n";   
    node->print(out, 0);
    out << "</body>\n";
    out << "</html>\n";
}

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
                        generateHtmlAst(html_file, ast);
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