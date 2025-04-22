#include"command.hpp"
#include<fstream>
#include <filesystem>

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
        
        const std::string& pattern = args[0];
        std::string line;
        
        while (std::getline(input, line)) {
            if (line.find(pattern) != std::string::npos) {
                output << line << std::endl;
            }
        }
    }

    void printCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output) {
        if (args.empty()) {
            std::cerr << "print: missing variable name" << std::endl;
            return;
        }
        
        for (const auto& arg : args) {
            output << arg << std::endl;
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