#include"command.hpp"
#include<fstream>
#include<filesystem>
#include<regex>
#include<sstream>
#include<iomanip>
namespace cmd {
    int exitCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output) {
        std::exit(0); 
        return 0;     
    }
    
    int echoCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output) {
        for (size_t i = 0; i < args.size(); i++) {
            output << args[i];
            if (i < args.size() - 1) {
                output << " ";
            }
        }
        output << std::endl;
        return 0; 
    }

    int catCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output) {
        if (args.empty()) {
            std::string line;
            while (std::getline(input, line)) {
                output << line << std::endl;
            }
            return 0; 
        } else {
            bool success = true;
            for (const auto& filename : args) {
                std::ifstream file(filename);
                if (!file) {
                    output << "cat: " << filename << ": No such file" << std::endl;
                    success = false;
                    continue;
                }
                std::string line;
                while (std::getline(file, line)) {
                    output << line << std::endl;
                }
            }
            return success ? 0 : 1; 
        }
    }

    int grepCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output) {
        if (args.empty()) {
            output << "grep: missing pattern" << std::endl;
            return 1;
        }
        
        bool useRegex = false;
        int patternIndex = 0;
        bool success = true;
        
        if (!args.empty() && (args[0] == "-r" || args[0] == "-e")) {
            useRegex = true;
            patternIndex = 1;
            
            if (args.size() <= patternIndex) {
                output << "grep: missing pattern after " << args[0] << std::endl;
                return 1;
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
                            output << "grep: " << fileName << ": No such file" << std::endl;
                            success = false;
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
                output << "grep: invalid regex pattern: " << pattern << " - " << e.what() << std::endl;
                return 1;
            }
        } else {
            std::string line;
            if (!fileNames.empty()) {
                for (const auto& fileName : fileNames) {
                    std::ifstream file(fileName);
                    if (!file) {
                        output << "grep: " << fileName << ": No such file" << std::endl;
                        success = false;
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
        
        return success ? 0 : 1;
    }

    int printCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output) {
        if (args.empty()) {
            output << "print: missing variable name" << std::endl;
            return 1;
        }

        bool output_integer = false;
        bool success = true;
        
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
                        output << "print: can't convert '" << arg << "' to integer" << std::endl;
                        success = false;
                    }
                } else {
                    output << arg << std::endl;
                }
            }
        }
        
        return success ? 0 : 1;
    }

    int cdCommand(const std::vector<std::string> &args, std::istream& input, std::ostream& output) {
        if (args.size() != 1) {
            output << "cd: expected exactly one argument" << std::endl;
            return 1;
        }
        
        const std::string& path = args[0];
        try {
            std::filesystem::current_path(path);
            return 0;
        } catch (const std::filesystem::filesystem_error& e) {
            output << "cd: " << path << ": " << e.what() << std::endl;
            return 1;
        }
    }

    int listCommand(const std::vector<std::string> &args, std::istream& input, std::ostream& output) {
        std::string path = ".";
        if (!args.empty()) {
            path = args[0];
        }
        
        try {
            for (const auto& entry : std::filesystem::directory_iterator(path)) {
                output << entry.path().filename().string() << std::endl;
            }
            return 0;
        } catch (const std::filesystem::filesystem_error& e) {
            output << "ls: " << e.what() << std::endl;
            return 1;
        }
    }

    int sortCommand(const std::vector<std::string> &args, std::istream& input, std::ostream& output) {
        std::vector<std::string> lines;
        std::string line;
        
        if (args.empty()) {
            while (std::getline(input, line)) {
                lines.push_back(line);
            }
        } else {
            for (const auto& filename : args) {
                std::ifstream file(filename);
                if (!file) {
                    output << "sort: " << filename << ": No such file" << std::endl;
                    return 1;
                }
                
                while (std::getline(file, line)) {
                    lines.push_back(line);
                }
            }
        }
        
        std::sort(lines.begin(), lines.end());
        
        for (const auto& sortedLine : lines) {
            output << sortedLine << std::endl;
        }
        
        return 0;
    }

    int findCommand(const std::vector<std::string> &args, std::istream& input, std::ostream& output) {
        if (args.size() < 2) {
            output << "find: expected at least two arguments" << std::endl;
            return 1;
        }
        
        const std::string& path = args[0];
        const std::string& pattern = args[1];
        
        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
                if (entry.path().filename().string().find(pattern) != std::string::npos) {
                    output << entry.path() << std::endl;
                }
            }
            return 0;
        } catch (const std::filesystem::filesystem_error& e) {
            output << "find: " << e.what() << std::endl;
            return 1;
        }
    }

    int pwdCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output) {
        try {
            output << std::filesystem::current_path() << std::endl;
            return 0;
        } catch (const std::filesystem::filesystem_error& e) {
            output << "pwd: " << e.what() << std::endl;
            return 1;
        }
    }

    int mkdirCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output) {
        if (args.empty()) {
            output << "mkdir: missing operand" << std::endl;
            return 1;
        }
        
        bool parents = false;
        std::vector<std::string> dirs;
        bool success = true;
        
        for (const auto& arg : args) {
            if (arg == "-p") {
                parents = true;
            } else {
                dirs.push_back(arg);
            }
        }
        
        for (const auto& dir : dirs) {
            try {
                if (parents) {
                    std::filesystem::create_directories(dir);
                } else {
                    std::filesystem::create_directory(dir);
                }
            } catch (const std::filesystem::filesystem_error& e) {
                output << "mkdir: cannot create directory '" << dir << "': " << e.what() << std::endl;
                success = false;
            }
        }
        
        return success ? 0 : 1;
    }

    int cpCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output) {
        if (args.size() < 2) {
            output << "cp: missing destination file operand after '" << (args.empty() ? "cp" : args[0]) << "'" << std::endl;
            return 1;
        }
        
        bool recursive = false;
        std::vector<std::string> files;
        
        for (const auto& arg : args) {
            if (arg == "-r" || arg == "-R") {
                recursive = true;
            } else {
                files.push_back(arg);
            }
        }
        
        if (files.size() < 2) {
            output << "cp: missing destination file operand" << std::endl;
            return 1;
        }
        
        std::string dest = files.back();
        files.pop_back();
        
        bool success = true;
        for (const auto& src : files) {
            try {
                if (recursive && std::filesystem::is_directory(src)) {
                    std::filesystem::copy(src, dest, std::filesystem::copy_options::recursive);
                } else {
                    std::filesystem::copy(src, dest);
                }
            } catch (const std::filesystem::filesystem_error& e) {
                output << "cp: cannot copy '" << src << "' to '" << dest << "': " << e.what() << std::endl;
                success = false;
            }
        }
        
        return success ? 0 : 1;
    }

    int mvCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output) {
        if (args.size() < 2) {
            output << "mv: missing destination file operand after '" << (args.empty() ? "mv" : args[0]) << "'" << std::endl;
            return 1;
        }
        
        std::string dest = args.back();
        bool success = true;
        
        for (size_t i = 0; i < args.size() - 1; ++i) {
            try {
                std::filesystem::rename(args[i], dest);
            } catch (const std::filesystem::filesystem_error& e) {
                output << "mv: cannot move '" << args[i] << "' to '" << dest << "': " << e.what() << std::endl;
                success = false;
            }
        }
        
        return success ? 0 : 1;
    }

    int touchCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output) {
        if (args.empty()) {
            output << "touch: missing file operand" << std::endl;
            return 1;
        }
        
        bool success = true;
        
        for (const auto& file : args) {
            try {
                if (!std::filesystem::exists(file)) {
                    std::ofstream(file).close();
                } else {
                    auto now = std::filesystem::file_time_type::clock::now();
                    std::filesystem::last_write_time(file, now);
                }
            } catch (const std::filesystem::filesystem_error& e) {
                output << "touch: cannot touch '" << file << "': " << e.what() << std::endl;
                success = false;
            }
        }
        
        return success ? 0 : 1;
    }

    int headCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output) {
        int numLines = 10;
        std::vector<std::string> files;
        
        for (size_t i = 0; i < args.size(); ++i) {
            if (args[i] == "-n" && i + 1 < args.size()) {
                try {
                    numLines = std::stoi(args[++i]);
                } catch (const std::exception& e) {
                    output << "head: invalid number of lines: '" << args[i] << "'" << std::endl;
                    return 1;
                }
            } else {
                files.push_back(args[i]);
            }
        }
        
        bool success = true;
        
        if (files.empty()) {
            std::string line;
            int lineCount = 0;
            while (std::getline(input, line) && lineCount < numLines) {
                output << line << std::endl;
                lineCount++;
            }
        } else {
            for (const auto& file : files) {
                if (files.size() > 1) {
                    output << "==> " << file << " <==" << std::endl;
                }
                
                std::ifstream in(file);
                if (!in) {
                    output << "head: cannot open '" << file << "' for reading" << std::endl;
                    success = false;
                    continue;
                }
                
                std::string line;
                int lineCount = 0;
                while (std::getline(in, line) && lineCount < numLines) {
                    output << line << std::endl;
                    lineCount++;
                }
            }
        }
        
        return success ? 0 : 1;
    }

    int tailCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output) {
        int numLines = 10;  
        std::vector<std::string> files;
        for (size_t i = 0; i < args.size(); ++i) {
            if (args[i] == "-n" && i + 1 < args.size()) {
                try {
                    numLines = std::stoi(args[++i]);
                } catch (const std::exception& e) {
                    output << "tail: invalid number of lines: '" << args[i] << "'" << std::endl;
                    return 1;
                }
            } else {
                files.push_back(args[i]);
            }
        }
        
        bool success = true;
        
        if (files.empty()) {
            std::vector<std::string> lines;
            std::string line;
            while (std::getline(input, line)) {
                lines.push_back(line);
            }
            
            size_t start = lines.size() > numLines ? lines.size() - numLines : 0;
            for (size_t i = start; i < lines.size(); ++i) {
                output << lines[i] << std::endl;
            }
        } else {
            for (const auto& file : files) {
                if (files.size() > 1) {
                    output << "==> " << file << " <==" << std::endl;
                }
                
                std::ifstream in(file);
                if (!in) {
                    output << "tail: cannot open '" << file << "' for reading" << std::endl;
                    success = false;
                    continue;
                }
                
                std::vector<std::string> lines;
                std::string line;
                while (std::getline(in, line)) {
                    lines.push_back(line);
                }
                
                size_t start = lines.size() > numLines ? lines.size() - numLines : 0;
                for (size_t i = start; i < lines.size(); ++i) {
                    output << lines[i] << std::endl;
                }
            }
        }
        
        return success ? 0 : 1;
    }

    int wcCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output) {
        bool countLines = false;
        bool countWords = false;
        bool countChars = false;
        std::vector<std::string> files;
    
        for (const auto& arg : args) {
            if (arg == "-l") {
                countLines = true;
            } else if (arg == "-w") {
                countWords = true;
            } else if (arg == "-c") {
                countChars = true;
            } else {
                files.push_back(arg);
            }
        }
        
        if (!countLines && !countWords && !countChars) {
            countLines = countWords = countChars = true;
        }
        
        int totalLines = 0;
        int totalWords = 0;
        int totalChars = 0;
        
        auto processStream = [&](std::istream& in, const std::string& name) {
            int lineCount = 0;
            int wordCount = 0;
            int charCount = 0;
            
            std::string line;
            while (std::getline(in, line)) {
                lineCount++;
                charCount += line.length() + 1;  
                
                std::istringstream iss(line);
                std::string word;
                while (iss >> word) {
                    wordCount++;
                }
            }
            
            if (countLines) output << std::setw(8) << lineCount << " ";
            if (countWords) output << std::setw(8) << wordCount << " ";
            if (countChars) output << std::setw(8) << charCount << " ";
            output << name << std::endl;
            
            totalLines += lineCount;
            totalWords += wordCount;
            totalChars += charCount;
        };
        
        if (files.empty()) {
            processStream(input, "");
        } else {
            for (const auto& file : files) {
                std::ifstream in(file);
                if (!in) {
                    output << "wc: " << file << ": No such file or directory" << std::endl;
                    continue;
                }
                processStream(in, file);
            }
            
            if (files.size() > 1) {
                if (countLines) output << std::setw(8) << totalLines << " ";
                if (countWords) output << std::setw(8) << totalWords << " ";
                if (countChars) output << std::setw(8) << totalChars << " ";
                output << "total" << std::endl;
            }
        }

        return 0;
    }

    int chmodCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output) {
        if (args.size() < 2) {
            output << "chmod: missing operand" << std::endl;
            return 1;
        }
        const std::string& mode = args[0];
        std::filesystem::perms perms;
        try {
            int octalMode = std::stoi(mode, nullptr, 8);
            perms = static_cast<std::filesystem::perms>(octalMode);
        } catch (const std::exception& e) {
            output << "chmod: invalid mode: '" << mode << "'" << std::endl;
            return 1;
        }
        
        for (size_t i = 1; i < args.size(); ++i) {
            try {
                std::filesystem::permissions(args[i], perms, std::filesystem::perm_options::replace);
            } catch (const std::filesystem::filesystem_error& e) {
                output << "chmod: cannot access '" << args[i] << "': " << e.what() << std::endl;
            }
        }
        return 0;
    }

}