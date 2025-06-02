#include"ast.hpp"
#include"parser.hpp"
#include"scanner.hpp"
#include"types.hpp"
#include"string_buffer.hpp"
#include"html.hpp"
#include"command.hpp"
#include<iostream>
#include<string>
#include<fstream>
#include<optional>
#include<vector>
#include<memory>
#include<functional>
#include<unordered_map>
#include<sstream>
#include <iomanip>
#include<readline/readline.h>
#include<readline/history.h>
#include"version_info.hpp"
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
#include <signal.h>
#include <unistd.h>
#endif
#ifdef _WIN32
#include<windows.h>
#endif
#include<cstdlib>
#include"cmd_argz.hpp"

struct Args {
    bool debug_output = false;
    bool debug_syntax_highlight = false;
    bool command_proc = false;
    bool stdin_input = false;
    bool debug_tokens = false;
    std::string command_text;
    std::string filename;
    std::vector<std::string> arguments;
};

Args proc_custom_args(int &argc, char **argv) {
	Args args;
	Argz<std::string> parser(argc, argv);
    parser.addOptionSingle('h', "Display help message")
        .addOptionDouble('H', "help", "Display help message")
        .addOptionSingle('v', "Display version")
        .addOptionDouble('V', "version", "Display version")     
        .addOptionSingle('i', "stdin input")
        .addOptionDouble('I', "stdin", "stdin input")
        .addOptionSingle('d', "debug output")
        .addOptionDouble('D', "debug", "debug output")  
        .addOptionSingle('s', "debug syntax highlight")
        .addOptionDouble('S', "syntax", "debug syntax highlight")
        .addOptionSingle('t', "debug tokens")
        .addOptionDouble('T', "tokens", "debug tokens")
        .addOptionSingleValue('c', "command")
        .addOptionDoubleValue('C', "command", "command")
        ;
    Argument<std::string> arg;
    int value = 0;
    try {
        while((value = parser.proc(arg)) != -1) {
            switch(value) {
                case 'h':
                case 'H':
                case 'v':
                case 'V':
                    std::cout << "MXCMD " << version_string << "\n(C) 1999-2025 LostSideDead Software\n\n";
                    parser.help(std::cout);
                    exit(EXIT_SUCCESS);
                    break;
                case 'i':
                case 'I':
                    args.stdin_input = true;;
                    break;
                case 'd':
                case 'D':
                    args.debug_output = true;
                    break;
                case 's':
                case 'S':
                    args.debug_syntax_highlight = true;
                    break;
                case 'c':
                case 'C':
                    args.command_proc = true;
                    args.command_text = arg.arg_value;
                    break;
                case 't':
                case 'T':
                    args.debug_tokens = true;
                    break;
                case '-':
                default:
                    args.arguments.push_back(arg.arg_value);
                    break;

                }
        }
    } catch (const ArgException<std::string>& e) {
        std::cerr << "mxcmd: Argument Exception" << e.text() << std::endl;
		return args;
    }
    cmd::argv.clear();
    for(auto &i : args.arguments) {
        cmd::argv.push_back(i);
    }
    return args;
}

#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
void sigint_handler(int sig) {
    if (program_running) {
        program_running = 0;
    } else {
          std::cout << "\nMXCMD: Interrupt signal received. Exiting...\n";
          exit(130);
    }
}
#endif

void dumpTokens(scan::Scanner &scan, std::ostream& out = std::cout) {
    out << "Idx | Type           | Value\n";
    out << "----+----------------+--------------------------\n";
    for (size_t i = 0; i < scan.size(); ++i) {
        std::ostringstream data;
        types::print_type_TokenType(data,scan[i].getTokenType());
        out << std::setw(3) << i << " | "
            << std::setw(14) << data.str() << " | "
            << scan[i].getTokenValue() << "\n";
    }
}

void execute_command(const std::string &text) {
    fflush(stdout);
    try {
        cmd::AstExecutor &executor = cmd::AstExecutor::getExecutor();
        scan::TString string_buffer(text);
        scan::Scanner scanner(string_buffer);
        cmd::Parser parser(scanner);
        auto ast = parser.parse();
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
        program_running = 1;
#endif
        executor.execute(std::cin, std::cout, ast);
        fflush(stdout);
        exit(0);
    } catch (const scan::ScanExcept &e) {
        std::cerr << "Scan error: " << e.why() << std::endl;
        exit(EXIT_FAILURE);
    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    } catch(const cmd::AstFailure &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    } catch (const state::StateException &e) {
        std::cerr << "State error: " << e.what() << std::endl;;
        exit(EXIT_FAILURE);
    } catch(const cmd::Exit_Exception  &e) {
        std::cout << "\nExit: " << e.getCode() << std::endl;
        exit(e.getCode());
    }
    catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv) {
    
    cmd::AstExecutor::getExecutor().setUpdateCallback(
        [](const std::string &chunk) {
            std::cout << chunk;
        }
    );
    
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);
#endif
    Args args = proc_custom_args(argc, argv);
    if(args.command_proc) {
        if(args.command_text.empty()) {
            std::cerr << "mx: No command provided.\n";
            exit(EXIT_FAILURE);
        }
        execute_command(args.command_text);
        exit(EXIT_SUCCESS);
    }
    std::cout << "MXCMD " << version_string << "\n(C) 1999-2025 LostSideDead Software\n\n";
    cmd::app_name = argv[0];

#ifdef _WIN32
    cmd::AstExecutor::getExecutor().getRegistry().registerTypedCommand("exec", 
        [](const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output) {
           std::ostringstream all_args;
           for(auto &arg : args) {
                try {
                    all_args << getVar(arg) << " ";
                } catch(const std::runtime_error &) {
                    all_args << arg.value << " ";
                }
            }
            std::string command_str = all_args.str();
            SECURITY_ATTRIBUTES sa;
            sa.nLength = sizeof(SECURITY_ATTRIBUTES);
            sa.bInheritHandle = TRUE;
            sa.lpSecurityDescriptor = NULL;
            HANDLE hStdOutRead, hStdOutWrite;
            if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0)) {
                output << "exec: failed to create output pipe" << std::endl;
                return 1;
            }
            SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0); 
            HANDLE hStdInRead, hStdInWrite;
            if (!CreatePipe(&hStdInRead, &hStdInWrite, &sa, 0)) {
                CloseHandle(hStdOutRead);
                CloseHandle(hStdOutWrite);
                output << "exec: failed to create input pipe" << std::endl;
                return 1;
            }
            SetHandleInformation(hStdInWrite, HANDLE_FLAG_INHERIT, 0); 
            STARTUPINFO si;
            ZeroMemory(&si, sizeof(STARTUPINFO));
            si.cb = sizeof(STARTUPINFO);
            si.hStdInput = hStdInRead;
            si.hStdOutput = hStdOutWrite;
            si.hStdError = hStdOutWrite;
            si.dwFlags |= STARTF_USESTDHANDLES;
            PROCESS_INFORMATION pi;
            ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
            std::string cmdLine = cmd::cmd_type + " " + command_str;
            if (!CreateProcess(NULL, const_cast<LPSTR>(cmdLine.c_str()), NULL, NULL, TRUE, 
                            CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
                CloseHandle(hStdOutRead);
                CloseHandle(hStdOutWrite);
                CloseHandle(hStdInRead);
                CloseHandle(hStdInWrite);
                output << "exec: failed to create process" << std::endl;
                return 1;
            }
            CloseHandle(hStdOutWrite);
            CloseHandle(hStdInRead);
            CloseHandle(hStdInWrite);
            DWORD bytesRead;
            char buffer[4096];
            BOOL success;
            while (true) {
                success = ReadFile(hStdOutRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL);
                if (!success || bytesRead == 0) break;
                buffer[bytesRead] = '\0';
                output << buffer;
                output.flush();
            }
            WaitForSingleObject(pi.hProcess, INFINITE);
            DWORD exitCode = 0;
            GetExitCodeProcess(pi.hProcess, &exitCode);
            CloseHandle(hStdOutRead);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return static_cast<int>(exitCode);
        });
#endif
    if(argc == 1) {
        bool active = true;
        bool debug_cmd = false;
        try {
            cmd::AstExecutor &executor = cmd::AstExecutor::getExecutor();
            using_history();
            read_history(".cmd_history");
            std::ostringstream input_stream;
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
            signal(SIGINT, sigint_handler);
#endif
            while(active) {
                try {
                    std::string prompt = executor.getPath() + " $> ";
                    char* line = readline(prompt.c_str());
                    if (line == nullptr) {
                        std::cout << std::endl;  
                        write_history(".cmd_history");
                        break;
                    }

                    if (line[0] != '\0') {

                        if(line[0] == '{') {
                            std::string iline;
                            input_stream.str("");
                            while(true) {
                                char *in_line = readline("... ");
                                iline = in_line;
                                if(in_line != nullptr) {
                                    free(in_line);
                                    auto pos = iline.find("}");
                                    if(pos != std::string::npos) {
                                        std::string left;
                                        left = iline.substr(0, pos);
                                        input_stream << left << "\n";
                                        break;
                                    } else {
                                        add_history(iline.c_str());
                                        input_stream << iline << "\n";
                                    }
                                }
                            }
                            scan::TString string_buffer(input_stream.str());
                            scan::Scanner scanner(string_buffer);
                            cmd::Parser parser(scanner);
                            auto ast = parser.parse();
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
                            program_running = 1;
#endif
                            executor.execute(std::cin, std::cout, ast);
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
                            program_running = 0;
#endif
                            if(debug_cmd) {
                                ast->print(std::cout);
                            }
                            continue;
                        }
                        std::string command_data;
                        add_history(line);
                        command_data = line;
                        free(line);

                        if(command_data == "wsl_on") {

#ifdef _WIN32
                            if(!std::filesystem::exists("C:\\Windows\\System32\\wsl.exe")) {
                                std::cerr << "WSL executable not found. Please ensure WSL is installed." << std::endl;
                                continue;
                            } else {
                                std::cout << "WSL mode enabled. Use 'wsl_off' to disable." << std::endl;
                                cmd::cmd_type = "wsl.exe";
                                continue;
                            }
#else
                            std::cerr << "WSL mode is not available on this platform." << std::endl;
                            continue;
#endif

                        } else if(command_data == "wsl_off") {
#ifdef _WIN32
                            cmd::cmd_type = "cmd.exe /c ";
                            std::cout << "WSL mode disabled. Using default command shell." << std::endl;
                            continue;
#else
                            std::cerr << "WSL mode is not available on this platform." << std::endl;    
                            continue;
#endif

                        } else if(command_data == "clear" || command_data == "cls") {
                            std::cout << "\033[2J\033[1;1H"; 
                            continue;
                        } else if(command_data == "exit" || command_data == "quit" || command_data == "q") {
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
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
                        program_running = 1;
#endif
                        executor.execute(std::cin, std::cout, ast);

#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
                        program_running = 1;
#endif
                        if(debug_cmd) {
                            ast->print(std::cout);
                        }
                    }
                } catch (const scan::ScanExcept &e) {
                    std::cerr << "Scan error: " << e.why() << std::endl;
                } catch (const std::exception &e) {
                    std::cerr << "Exception: " << e.what() << std::endl;
                } catch(const cmd::AstFailure &e) {
                    std::cerr << "Exception: " << e.what() << std::endl;
                } catch (const state::StateException &e) {
                    std::cerr << "State error: " << e.what() << std::endl;;
                } catch(const cmd::Exit_Exception  &e) {
                    std::cout << "\nExit: " << e.getCode() << std::endl;
                }
                 catch (...) {
                    std::cerr << "Unknown error occurred." << std::endl;
                }
            }
            write_history(".cmd_history");
        } catch (const scan::ScanExcept &e) {
            std::cerr << "Scan error: " << e.why() << std::endl;
        } catch (const std::exception &e) {
            std::cerr << "Exception: " << e.what() << std::endl;
        } catch (const cmd::AstFailure &e) {
            std::cerr << "Exception: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "Unknown error occurred." << std::endl;
        }

    } else if(argc >= 2  && args.stdin_input == false) {

        cmd::AstExecutor &executor = cmd::AstExecutor::getExecutor();
        bool debug_cmd = false;
        bool debug_html = false;
        bool debug_tokens = false;
            std::ostringstream stream;
            std::fstream file;
            if(args.debug_output) {
                debug_cmd = true;
            }
            if(args.debug_syntax_highlight) {
                debug_html = true;
            }
            if(args.debug_tokens) {
                debug_tokens = true;
            }
            if(args.arguments.size() == 0) {
                std::cerr << "mx: No script file name provided.\n";
                return EXIT_FAILURE;
            }
            std::string app_name = args.arguments[0];
            file.open(app_name, std::ios::in);
            if(!file.is_open()) {
                std::cerr << "Error loading file: " << app_name << "\n";;
                return EXIT_FAILURE;
            }
            stream << file.rdbuf();
            std::string fileContent = stream.str();
            
            if(!fileContent.empty()) {
                fileContent.erase(
                    std::remove(fileContent.begin(), fileContent.end(), '\r'),
                        fileContent.end()
                );
            }

            if (fileContent.size() >= 2 && fileContent[0] == '#' && fileContent[1] == '!') {
                size_t newlinePos = fileContent.find('\n');
                if (newlinePos != std::string::npos) {
                    fileContent = fileContent.substr(newlinePos + 1);
                }
            }

            std::atomic<bool> exec_interrupt = false;
            executor.setInterrupt(&exec_interrupt);
            try {
                scan::TString string_buffer(fileContent); 
                scan::Scanner scanner(string_buffer);
                cmd::Parser parser(scanner);
                auto ast = parser.parse();

                if(debug_tokens) {
                    std::ofstream out_file("debug.tokens.txt");
                    std::cout << "Debug Tokens Information written to: debug.tokens.txt\n\n";
                    dumpTokens(scanner, out_file);
                }

    #if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
                program_running = 1;
    #endif
                executor.setPath(std::filesystem::path(app_name).parent_path().string());
                executor.execute(std::cin, std::cout, ast);

                std::cout.flush();
                std::cout << std::flush;
                if(debug_cmd)  {
                    std::ofstream html_file("debug.html");
                    html::gen_html(html_file, ast);
                    std::cout << "Debug Information written to: debug.html\n\n";
                }
                if(debug_html) {
                    std::ofstream html_file("debug.syntax.html");
                    html::gen_html_color(html_file, ast);
                    std::cout << "Debug Syntax Information written to: debug.syntax.html\n\n";
                }
                
            } catch(const scan::ScanExcept &e) {
                std::cerr << "Scan Error: " << e.why() << std::endl;
                return EXIT_FAILURE;
            } catch(const std::exception &e) {
                std::cerr << "Exception: " << e.what() << std::endl;
                return EXIT_FAILURE;
            } catch(const cmd::AstFailure &e) {
                std::cerr << "Failure: " << e.what() << std::endl;
                return EXIT_FAILURE;   
            } catch(const state::StateException &e) {
                std::cerr << "State Exception: " << e.what() << std::endl;
            } catch(const cmd::Exit_Exception &e) {
                std::cout << "\nExit: " << e.getCode() << std::endl;
                return e.getCode();
            }
            catch(...) {
                std::cerr << "Fatal Error has occoured.\n";
                throw;
                return EXIT_FAILURE;
            }
        } else if(argc == 2 && args.stdin_input) {
            try {
            cmd::AstExecutor &executor = cmd::AstExecutor::getExecutor();
            std::ostringstream stream;
            stream << std::cin.rdbuf();
            scan::TString string_buffer(stream.str());
            scan::Scanner scanner(string_buffer);
            cmd::Parser parser(scanner);
            auto ast = parser.parse();
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
            program_running = 1;
#endif
            executor.execute(std::cin, std::cout, ast);
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
            program_running = 0;
#endif    
        } catch(const scan::ScanExcept &e) {
            std::cerr << "Scan Error: " << e.why() << std::endl;
            return EXIT_FAILURE;
        } catch(const std::exception &e) {
            std::cerr << "Exception: " << e.what() << std::endl;
            return EXIT_FAILURE;
        } catch(cmd::AstFailure &e) {
            std::cerr << "Failure: " << e.what() << std::endl;
            return EXIT_FAILURE;
        } catch(state::StateException &e) {
            std::cerr << "State Exception: " << e.what() << std::endl;
            return EXIT_FAILURE;
        } catch(cmd::Exit_Exception &e) {
            std::cout << "\nExit: " << e.getCode() << std::endl;
            return e.getCode();
        }
        catch(...) {
            std::cerr << "Unknown Error has Occoured..\n";
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}
