#include"command.hpp"
#include<fstream>
#include <filesystem>
#include <regex>

namespace cmd {
    void exitCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output) {
        std::exit(0);
    }
    
    void echoCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output) {
        for (size_t i = 0; i < args.size(); i++) {
            output << args[i];
            if (i < args.size() - 1) {
                output << " ";
            }
        }
        output << std::endl;
    }

    
    void catCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output) {
        if (args.empty()) {
    
            std::string line;
            while (std::getline(input, line)) {
                output << line << std::endl;
            }
        } else {
    
            for (const auto& filename : args) {
                std::ifstream file(filename);
                if (!file) {
                    std::cerr << "cat: " << filename << ": No such file" << std::endl;
                    continue;
                }
                std::string line;
                while (std::getline(file, line)) {
                    output << line << std::endl;
                }
            }
        }
    }

    
    void grepCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output) {
        if (args.empty()) {
            std::cerr << "grep: missing pattern" << std::endl;
            return;
        }
        
        bool useRegex = false;
        int patternIndex = 0;
        
        if (!args.empty() && (args[0] == "-r" || args[0] == "-e")) {
            useRegex = true;
            patternIndex = 1;
            
            if (args.size() <= patternIndex) {
                std::cerr << "grep: missing pattern after " << args[0] << std::endl;
                return;
            }
        }
        
        const std::string& pattern = args[patternIndex];
        std::vector<std::string> fileNames;
        
        for (size_t i = patternIndex + 1; i < args.size(); i++) {
            fileNames.push_back(args[i]);
        }
        
        if (useRegex) {
            try {
                std::regex re(pattern, std::regex::extended);
                std::string line;
                
                if (!fileNames.empty()) {
                    for (const auto& fileName : fileNames) {
                        std::ifstream file(fileName);
                        if (!file) {
                            std::cerr << "grep: " << fileName << ": No such file" << std::endl;
                            continue;
                        }
                        
                        while (std::getline(file, line)) {
                            if (std::regex_search(line, re)) {
                                output << line << std::endl;
                            }
                        }
                    }
                } else {
                    while (std::getline(input, line)) {
                        if (std::regex_search(line, re)) {
                            output << line << std::endl;
                        }
                    }
                }
            } catch (const std::regex_error& e) {
                std::cerr << "grep: invalid regex pattern: " << pattern << " - " << e.what() << std::endl;
            }
        } else {
            std::string line;
            if (!fileNames.empty()) {
                for (const auto& fileName : fileNames) {
                    std::ifstream file(fileName);
                    if (!file) {
                        std::cerr << "grep: " << fileName << ": No such file" << std::endl;
                        continue;
                    }
                    
                    while (std::getline(file, line)) {
                        if (line.find(pattern) != std::string::npos) {
                            output << line << std::endl;
                        }
                    }
                }
            } else {
                while (std::getline(input, line)) {
                    if (line.find(pattern) != std::string::npos) {
                        output << line << std::endl;
                    }
                }
            }
        }
    }

    void printCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output) {
        if (args.empty()) {
            std::cerr << "print: missing variable name" << std::endl;
            return;
        }

        bool output_integer = false;
        
        for (const auto& arg : args) {
            if (arg == "-i") {
                output_integer = true;
                continue;
            } else if(arg == "-f") {
                output_integer = false;
                continue;
            } 
            else {
                if(output_integer == true) {
                    try {
                        output << std::stoi(arg) << std::endl;
                        output_integer = false;
                    } catch (const std::exception& e) {
                        std::cerr << "print: can't convert '" << arg << "' to integer" << std::endl;
                    }
                } else {
                    output << arg << std::endl;
                }
            }
        }
    }

    void cdCommand(const std::vector<std::string> &args, std::istream& input, std::ostream& output) {
        if (args.size() != 1) {
            output << "cd: expected exactly one argument" << std::endl;
            return;
        }
        
        const std::string& path = args[0];
        try {
            std::filesystem::current_path(path);
        } catch (const std::filesystem::filesystem_error& e) {
            output << "cd: " << path << ": " << e.what() << std::endl;
        }
    }

    void listCommand(const std::vector<std::string> &args, std::istream& input, std::ostream& output) {
        std::string path = ".";
        if (!args.empty()) {
            path = args[0];
        }
        
        try {
            for (const auto& entry : std::filesystem::directory_iterator(path)) {
                output << entry.path().filename().string() << std::endl;
            }
        } catch (const std::filesystem::filesystem_error& e) {
            output << "ls: " << e.what() << std::endl;
        }
    }
}