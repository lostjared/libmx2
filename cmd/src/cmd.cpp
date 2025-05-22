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
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);
#endif
    if(argc == 3 && std::string(argv[1]) == "-c") {
        execute_command(argv[2]);
        exit(0);
    }
    std::cout << "MXCMD " << version_string << "\n(C) 1999-2025 LostSideDead Software\n\n";
    cmd::app_name = argv[0];
    if(argc > 2) {
        for(int i = 2; i < argc; ++i) {
            cmd::argv.push_back(argv[i]);
        }
    }
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
            std::string cmdLine = "cmd.exe /c " + command_str;
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

    } else if(argc >= 2 && (std::string(argv[1]) != "--stdin" && std::string(argv[1]) != "-i")) {
        cmd::AstExecutor &executor = cmd::AstExecutor::getExecutor();
        bool debug_cmd = false;
        bool debug_html = false;
        bool debug_tokens = false;
            std::ostringstream stream;
            std::fstream file;
            if(argc == 3) {
                if(std::string(argv[2]) == "-d") {
                    debug_cmd = true;
                }
                else if(std::string(argv[2]) == "-f") {
                    debug_html = true;
                }
                else if(std::string(argv[2]) == "-t") {
                    debug_tokens = true;
                }
            }
            cmd::app_name = argv[1];
            file.open(argv[1], std::ios::in);
            if(!file.is_open()) {
                std::cerr << "Error loading file: " << argv[1] << "\n";;
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
                    dumpTokens(scanner, std::cout);
                    return 0;
                }

    #if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
                program_running = 1;
    #endif
                executor.setPath(std::filesystem::path(argv[1]).parent_path().string());
                executor.setUpdateCallback([&exec_interrupt](const std::string &text) {
                    if(exec_interrupt.load()) {
                        throw cmd::Exit_Exception(100);
                    }
                });
                executor.execute(std::cin, std::cout, ast);

                std::cout.flush();
                std::cout << std::flush;
                if(debug_cmd || debug_html) {
                    std::cout << "Debug Information written to: debug.html\n\n";
                    std::ofstream html_file("debug.html");
                    if(html_file.is_open()) {
                        if(debug_cmd) 
                            html::gen_html(html_file, ast);
                        else if(debug_html)
                            html::gen_html_color(html_file, ast);
                        html_file.close();
                    }
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
        } else if(argc == 2 && (std::string(argv[1]) == "--stdin" || std::string(argv[1]) == "-i")) {
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