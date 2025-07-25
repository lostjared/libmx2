#include"command.hpp"
#include<fstream>
#include<filesystem>
#include<regex>
#include<sstream>
#include<iomanip>
#include<cstdlib>
#include<random>
#include"game_state.hpp"
#include"parser.hpp"
#include"html.hpp"
#include"command_reg.hpp"

#if !defined(__EMSCRIPTEN__) &&  !defined(_WIN32)
#include<unistd.h>
#include<sys/wait.h>
#include<signal.h>
#include<fcntl.h>
#include<sys/select.h>
#include<pty.h>
#include<sys/stat.h>
#endif
#if !defined(__EMSCRIPTEN__) && defined(_WIN32)
#include <windows.h>
#endif

#include<thread>
#include<mutex>
#include<atomic>

namespace state {
    GameState *getGameState() {
        static GameState gameState;
        return &gameState;
    }
}

namespace cmd {

    std::vector<std::string> argv;
    std::string app_name;    
    std::string cmd_type = "cmd.exe /c ";

    int exitCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output) {
        if(args.empty()) {
              throw Exit_Exception(0);
              return 0;
        } else if(args.size() == 1) {
            int exitCode = std::stoi(args[0]);
            throw Exit_Exception(exitCode);
        }
        throw Exit_Exception(0);
        return 0;     
    }
    
    int echoCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream& stream) {
        std::ostringstream output;;
        for (size_t i = 0; i < args.size(); i++) {
            try {
                output << getVar(args[i]);
            } catch (const std::runtime_error &e) {
                output << args[i].value;
            }
            
            if (i < args.size() - 1) {
                output << " ";
            }
        }
        output << std::endl;
        
        if(&stream != &std::cout) {
            stream << output.str();
        } else  {
            printf("%s", output.str().c_str());
            fflush(stdout);
        }
        return 0;
    }

    int catCommand(const std::vector<Argument>& args, std::istream& input, std::ostream& output) {
        if (args.empty()) {
            std::string line;
            while (std::getline(input, line)) {
                output << line << std::endl;
            }
            return 0; 
        } else {
            bool success = true;
            for (const auto& filename : args) {
                std::string filen_ = getVar(filename);
                std::ifstream file(filen_);
                if (!file) {
                    output << "cat: " << filen_ << ": No such file" << std::endl;
                    success = false;
                    continue;
                }
                std::string line;
                while (std::getline(file, line)) {
                    output << line << std::endl;
                }
            }
    
            fflush(stdout);
            output.flush();
            return success ? 0 : 1; 
        }
    }

    int grepCommand(const std::vector<Argument>& args, std::istream& input, std::ostream& output) {
        if (args.empty()) {
            output << "grep: missing pattern" << std::endl;
            return 1;
        }
        
        bool useRegex = false;
        int patternIndex = 0;
        bool success = true;
        
        if (!args.empty() && (args[0].value ==  "--r" || args[0].value == "--e")) {
            useRegex = true;
            patternIndex = 1;
            
            if (static_cast<int>(args.size()) <= patternIndex) {
                output << "grep: missing pattern after " << args[0].value << std::endl;
                return 1;
            }
        }
        
        std::string patternStr = getVar(args[patternIndex]);
        std::vector<std::string> fileNames;
        for (size_t i = patternIndex + 1; i < args.size(); i++) {
            std::string file_n = getVar(args[i]);
            fileNames.push_back(file_n);
        }
        
        if (useRegex) {
            try {
                std::regex re(patternStr, std::regex::extended);
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
                output << "grep: invalid regex pattern: " << patternStr << " - " << e.what() << std::endl;
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
                        if (line.find(patternStr) != std::string::npos) {
                            output << line << std::endl;
                        }
                    }
                }
            } else {
                while (std::getline(input, line)) {
                    if (line.find(patternStr) != std::string::npos) {
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
            if (arg == "--i") {
                output_integer = true;
                continue;
            } else if(arg == "--f") {
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

    int cdCommand(const std::vector<Argument> &args, std::istream& input, std::ostream& output) {
        if (args.size() != 1) {
            output << "cd: expected exactly one argument" << std::endl;
            return 1;
        }
        std::string path = getVar(args[0]);
        try {
            std::filesystem::current_path(path);
            return 0;
        } catch (const std::filesystem::filesystem_error& e) {
            output << "cd: " << path << ": " << e.what() << std::endl;
            return 1;
        }
    }

    int listCommand(const std::vector<Argument> &args, std::istream& input, std::ostream& output) {
        std::string path = args.empty() ? "." : getVar(args[0]);
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

    int sortCommand(const std::vector<Argument> &args, std::istream& input, std::ostream& output) {
        std::vector<std::string> lines;
        std::string line;
        
        if (args.empty()) {
            while (std::getline(input, line)) {
                lines.push_back(line);
            }
        } else {
            for (const auto& filename : args) {
                std::string file_n = getVar(filename);
                std::ifstream file(file_n);
                if (!file) {
                    output << "sort: " << file_n << ": No such file" << std::endl;
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

    int findCommand(const std::vector<Argument> &args, std::istream& input, std::ostream& output) {
        if (args.size() < 2) {
            output << "find: expected at least two arguments" << std::endl;
            return 1;
        }
        std::string path = getVar(args[0]);
        std::string pattern = getVar(args[1]);
        
        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
                if (entry.path().filename().string().find(pattern) != std::string::npos) {
                    output << entry.path().string() << std::endl;
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
            output << std::filesystem::current_path().string() << std::endl;
            return 0;
        } catch (const std::filesystem::filesystem_error& e) {
            output << "pwd: " << e.what() << std::endl;
            return 1;
        }
    }

    int mkdirCommand(const std::vector<Argument>& args, std::istream& input, std::ostream& output) {
        if (args.empty()) {
            output << "mkdir: missing operand" << std::endl;
            return 1;
        }
        bool parents = false;
        std::vector<std::string> dirs;
        bool success = true;
        for (const auto& arg : args) {
            std::string a = getVar(arg);
            if (a == "--p") {
                parents = true;
            } else {
                dirs.push_back(a);
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

    int cpCommand(const std::vector<Argument>& args, std::istream& input, std::ostream& output) {
        if (args.size() < 2) {
            output << "cp: missing destination file operand after '" << (args.empty() ? "cp" : args[0].value) << "'" << std::endl;
            return 1;
        }
        
        bool recursive = false;
        std::vector<std::string> files;
        
        for (const auto& arg : args) {
            std::string a = getVar(arg);
            if (a == "--r" || a == "--R") {
                recursive = true;
            } else {
                files.push_back(a);
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

    int mvCommand(const std::vector<Argument>& args, std::istream& input, std::ostream& output) {
        if (args.size() < 2) {
            output << "mv: missing destination file operand after '" << (args.empty() ? "mv" : args[0].value) << "'" << std::endl;
            return 1;
        }
        
        std::string dest = getVar(args.back());
        
        bool success = true;

        for (size_t i = 0; i < args.size() - 1; ++i) {
            try {  
                std::string a = getVar(args[i]);
                std::filesystem::rename(a, dest);
            } catch (const std::filesystem::filesystem_error& e) {
                output << "mv: cannot move '" << args[i].value << "' to '" << dest << "': " << e.what() << std::endl;
                success = false;
            } 
        }
        
        return success ? 0 : 1;
    }

    int touchCommand(const std::vector<Argument>& args, std::istream& input, std::ostream& output) {
        if (args.empty()) {
            output << "touch: missing file operand" << std::endl;
            return 1;
        }
        bool success = true;
        for (const auto& file : args) {
            try {
                std::string f = getVar(file);
                if (!std::filesystem::exists(f)) {
                    std::ofstream(f).close();
                } else {
                    auto now = std::filesystem::file_time_type::clock::now();
                    std::filesystem::last_write_time(f, now);
                }
            } catch (const std::filesystem::filesystem_error& e) {
                output << "touch: cannot touch '" << file.value << "': " << e.what() << std::endl;
                success = false;
            } catch(const state::StateException &e) { }
        }

        return success ? 0 : 1;
    }

    int headCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output) {
        int numLines = 10;
        std::vector<std::string> files;
        
        for (size_t i = 0; i < args.size(); ++i) {
            if (args[i] == "--n" && i + 1 < args.size()) {
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
            if (args[i] == "--n" && i + 1 < args.size()) {
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
            
            size_t start = lines.size() > static_cast<size_t>(numLines) ? lines.size() - static_cast<size_t>(numLines) : 0;
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
                
                size_t start = lines.size() > static_cast<size_t>(numLines) ? lines.size() -static_cast<size_t>(numLines) : 0;
                for (size_t i = start; i < lines.size(); ++i) {
                    output << lines[i] << std::endl;
                }
            }
        }
        
        return success ? 0 : 1;
    }

    int wcCommand(const std::vector<Argument>& args, std::istream& input, std::ostream& output) {
        bool countLines = false;
        bool countWords = false;
        bool countChars = false;
        std::vector<std::string> files;
    
        for (const auto& arg : args) {
            std::string a = getVar(arg);
            if (a == "--l") {
                countLines = true;
            } else if (a == "--w") {
                countWords = true;
            } else if (a == "--c") {
                countChars = true;
            } else {
                files.push_back(a);
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

    int sedCommand(const std::vector<Argument>& args, std::istream& input, std::ostream& output) {
        if (args.empty()) {
            output << "Usage: sed [options] 's/pattern/replacement/[g]' [file]" << std::endl;
            output << "Options:" << std::endl;
            output << "  -n        Suppress automatic printing of pattern space" << std::endl;
            output << "  -i        Edit files in place" << std::endl;
            return 1;
        }

        bool suppressOutput = false;
        bool editInPlace = false;
        std::string expression;
        std::string filename;
        
        for (const auto& arg : args) {
            std::string a = getVar(arg);
            if (a.size()>2 && a[0] == '-' && a[1] == '-') {
                if (a.find('n') != std::string::npos) {
                    suppressOutput = true;
                }
                if (a.find('i') != std::string::npos) {
                    editInPlace = true;
                }
            } else if (a[0] == 's' && a.find('/') == 1) {
                expression = a;
            } else {
                filename = a;
            }
        }

        if (expression.empty() || expression[0] != 's') {
            output << "Error: Expected substitution expression (s/pattern/replacement/[g])" << std::endl;
            return 1;
        }

        size_t firstSlash = expression.find('/');
        if (firstSlash == std::string::npos) {
            output << "Error: Invalid substitution format" << std::endl;
            return 1;
        }
        
        size_t secondSlash = expression.find('/', firstSlash + 1);
        if (secondSlash == std::string::npos) {
            output << "Error: Invalid substitution format" << std::endl;
            return 1;
        }
        
        size_t thirdSlash = expression.find('/', secondSlash + 1);
        if (thirdSlash == std::string::npos) {
            output << "Error: Invalid substitution format" << std::endl;
            return 1;
        }
        
        std::string pattern = expression.substr(firstSlash + 1, secondSlash - firstSlash - 1);
        std::string replacement = expression.substr(secondSlash + 1, thirdSlash - secondSlash - 1);
        bool globalReplace = thirdSlash < expression.length() - 1 && expression[thirdSlash + 1] == 'g';
        
        try {
            std::regex regexPattern(pattern);
            std::ifstream inFile;
            std::stringstream outputBuffer;
            std::istream* inputStream = &input;
            
            if (!filename.empty()) {
                inFile.open(filename);
                if (!inFile.is_open()) {
                    output << "Error: Cannot open file '" << filename << "'" << std::endl;
                    return 1;
                }
                inputStream = &inFile;
            }
            
            std::string line;
            while (std::getline(*inputStream, line)) {
                std::string result;
                if (globalReplace) {
                    result = std::regex_replace(line, regexPattern, replacement);
                } else {
                    result = std::regex_replace(line, regexPattern, replacement, 
                                               std::regex_constants::format_first_only);
                }
                
                if (!suppressOutput) {
                    outputBuffer << result << std::endl;
                }
            }
            
            if (editInPlace && !filename.empty()) {
                std::ofstream outFile(filename);
                if (!outFile.is_open()) {
                    output << "Error: Cannot write to file '" << filename << "'" << std::endl;
                    return 1;
                }
                outFile << outputBuffer.str();
                outFile.close();
            } else {
                output << outputBuffer.str();
            }
            
            if (inFile.is_open()) {
                inFile.close();
            }
            
        } catch (const std::regex_error& e) {
            output << "Error in regular expression: " << e.what() << std::endl;
            return 1;
        }
        
        return 0;
    }

    int printfCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream& stream_output) {
        std::ostringstream output;
        if (args.empty()) {
            output << "Usage: printf FORMAT [ARGUMENTS...]" << std::endl;
            output << "Print ARGUMENTS according to FORMAT" << std::endl;
            return 1;
        }
        std::vector<std::string> expandedArgs;
        for (const auto& arg : args) {
            expandedArgs.push_back(getVar(arg));
        }
        
        std::string formatStr = parseEscapeSequences(expandedArgs[0]);
        size_t argIndex = 1;
        size_t pos = 0;

        while (pos < formatStr.length()) {
            size_t percentPos = formatStr.find('%', pos);
            if (percentPos == std::string::npos) {
                output << formatStr.substr(pos);
                break;
            }

            output << formatStr.substr(pos, percentPos - pos);
            if (percentPos + 1 >= formatStr.length()) {
                output << '%'; 
                break;
            }

            char specifier = formatStr[percentPos + 1];
            
            std::string widthPrecision;
            size_t specPos = percentPos + 1;
            
            while (specPos < formatStr.length() && 
                   (std::isdigit(formatStr[specPos]) || 
                    formatStr[specPos] == '.' || 
                    formatStr[specPos] == '-' || 
                    formatStr[specPos] == '+')) {
                widthPrecision += formatStr[specPos];
                specPos++;
            }
            
            if (specPos > percentPos + 1) {
                specifier = formatStr[specPos];
            }
            
            switch (specifier) {
                case '%':
                    output << '%';
                    break;
                case 'd':
                case 'i':
                    if (argIndex < expandedArgs.size()) {
                        try {
                            int val = std::stoi(expandedArgs[argIndex++]);
                            output << val;
                        } catch (const std::exception&) {
                            output << "0";
                        }
                    }
                    break;
                case 'f':
                    if (argIndex < expandedArgs.size()) {
                        try {
                            double val = std::stod(expandedArgs[argIndex++]);
                            size_t precision = 6; 
                            size_t dotPos = widthPrecision.find('.');
                            if (dotPos != std::string::npos && dotPos + 1 < widthPrecision.length()) {
                                try {
                                    precision = std::stoul(widthPrecision.substr(dotPos + 1));
                                } catch (...) {}
                            }
                            
                            output << std::fixed << std::setprecision(precision) << val;
                        } catch (const std::exception&) {
                            output << "0.000000";
                        }
                    }
                    break;
                case 's':
                    if (argIndex < expandedArgs.size()) {
                        output << expandedArgs[argIndex++];
                    }
                    break;
                case 'x':
                    if (argIndex < expandedArgs.size()) {
                        try {
                            unsigned int val = std::stoul(expandedArgs[argIndex++]);
                            output << std::hex << val << std::dec;
                        } catch (const std::exception&) {
                            output << "0";
                        }
                    }
                    break;
                case 'X':
                    if (argIndex < expandedArgs.size()) {
                        try {
                            unsigned int val = std::stoul(expandedArgs[argIndex++]);
                            output << std::hex << std::uppercase << val << std::nouppercase << std::dec;
                        } catch (const std::exception&) {
                            output << "0";
                        }
                    }
                    break;
                case 'o':
                    if (argIndex < expandedArgs.size()) {
                        try {
                            unsigned int val = std::stoul(expandedArgs[argIndex++]);
                            output << std::oct << val << std::dec;
                        } catch (const std::exception&) {
                            output << "0";
                        }
                    }
                    break;
                case 'c':
                    if (argIndex < expandedArgs.size()) {
                        if (!expandedArgs[argIndex].empty()) {
                            output << expandedArgs[argIndex][0];
                        }
                        argIndex++;
                    }
                    break;
                default:
                    output << '%' << specifier;
            }
            
            pos = specPos + 1;
        }

        if(&stream_output != &std::cout) {
            stream_output << output.str();
            stream_output.flush();
        } else {
            printf("%s", output.str().c_str());
            fflush(stdout);
        }
        return 0;
    }

    int debugSet(const std::vector<Argument>& args, std::istream& input, std::ostream& output) {
        if (args.size() != 2) {
            output << "Usage: debug set <variable> <value>" << std::endl;
            return 1;
        } 
        
        if(args[0].type != ArgType::ARG_VARIABLE) {
            output << "Usage: debug_set <variable> <value>\n";
            return 1;
        }
        std::string variable = args[0].value;
        std::string value = args[1].value;
        try {
            state::GameState *gameState = state::getGameState();
            if(args[1].type == ArgType::ARG_VARIABLE) {
                value = gameState->getVariable(args[1].value);
            }
            gameState->setVariable(variable, value);
        } catch(const state::StateException &e) {}
        output << "Set Debug " << variable << " to " << value << std::endl;
        return 0;
    }

    int debugGet(const std::vector<Argument>& args, std::istream& input, std::ostream& output) {
        if (args.size() != 1) {
            output << "Usage: debug_get <variable>" << std::endl;
            return 1;
        }
        if(args[0].type != ArgType::ARG_VARIABLE) {
            output << "Useage debug_get <varaible>\n";
            return 1;
        }
        std::string variable = args[0].value;
        state::GameState *gameState = state::getGameState();
        try {
            std::string value = gameState->getVariable(variable);
            
        } catch (const state::StateException &e) {
            output << "Error: " << e.what() << std::endl;
            return 1;
        }       
        return 0;
    }   

    int debugList(const std::vector<std::string>& args, std::istream& input, std::ostream& output) {
        state::GameState *gameState = state::getGameState();
        gameState->listVariables(output);
        return 0;
    }

    int debugClear(const std::vector<Argument>& args, std::istream& input, std::ostream& output) {
        if(args.empty()) {
            output << "debugClear: Requires argument to clear.\n";
            return 1;
        }
        if(args[0].type != ArgType::ARG_VARIABLE) {
            output << "debugClear: Variable name required\n";
            return 1;
        }
        state::GameState *gameState = state::getGameState();
        gameState->clearVariable(args[0].value);
        output << "Cleared all Debug variables" << std::endl;
        return 0;
    }

    int debugClearAll(const std::vector<std::string>& args, std::istream& input, std::ostream& output) {
        state::GameState *gameState = state::getGameState();
        gameState->clearVariables();
        output << "Cleared all Debug variables" << std::endl;
        return 0;
    }
 
    int debugSearch(const std::vector<std::string>& args, std::istream& input, std::ostream& output) {
        int patternIndex = 0;
        std::string pattern = "";
        if (!args.empty() && (args[0] == "-r" || args[0] == "-e")) {
            patternIndex = 1;
            
            if (static_cast<int>(args.size()) <= patternIndex) {
                output << "grep: missing pattern after " << args[0] << std::endl;
                return 1;
            }
        } else if (!args.empty()) {
            pattern = args[0];
        }
        state::GameState *state = state::getGameState();
        state->searchVariables(output, pattern);
        return 0;
    }

    int dumpVariables(const std::vector<std::string>& args, std::istream& input, std::ostream& output) {
        if(!args.empty() && args.size() == 1) {
            state::GameState *state = state::getGameState();
            if(state->dumpVariables(args[0])) {
                output << "var table dumpped to: " << args[0] << "\n";
            } else {
                output << "var table dump failed\n";
                return 1;
            }
        }
        return 0;
    }

    int testCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream& output) {
        if (args.empty()) {
            return 1; 
        }

        if (args.size() == 2) {
            const std::string& op = args[0].value;
            const std::string value = getVar(args[1]);
            
            if (op == "--z") {
                return value.empty() ? 0 : 1; 
            } else if (op == "--n") {
                return !value.empty() ? 0 : 1;
            } else if (op == "--e") {
                return std::filesystem::exists(value) ? 0 : 1;
            } else if (op == "--f") {
                return (std::filesystem::exists(value) && 
                       std::filesystem::is_regular_file(value)) ? 0 : 1; 
            } else if (op == "--d") {
                       return (std::filesystem::exists(value) && 
                       std::filesystem::is_directory(value)) ? 0 : 1; 
            }
        }

        if (args.size() >= 3) {
            const std::string leftValue = getVar(args[0]);
            const std::string& op = args[1].value;
            const std::string rightValue = getVar(args[2]);
            if (op == "=") {
                return leftValue == rightValue ? 0 : 1; 
            } else if (op == "!=") {
                return leftValue != rightValue ? 0 : 1; 
            } else if (op == "--eq") {
                try {
                    double left = std::stod(leftValue);
                    double right = std::stod(rightValue);
                    return left == right ? 0 : 1; 
                } catch (const std::exception&) {
                    output << "test: integer expression expected instead i found: " << leftValue << " " << rightValue << std::endl;
                    return 2;
                }
            } else if (op == "--ne") {
                try {
                    double left = std::stod(leftValue);
                    double right = std::stod(rightValue);
                    return left != right ? 0 : 1; 
                } catch (const std::exception&) {
                    output << "test: integer expression expected instead i found: " << leftValue << " " << rightValue << std::endl;
                    return 2;
                }
            } else if (op == "--gt") {
                try {
                    double left = std::stod(leftValue);
                    double right = std::stod(rightValue);
                    return left > right ? 0 : 1; 
                } catch (const std::exception&) {
                    output << "test: integer expression expected instead i found: " << leftValue << " " << rightValue << std::endl;
                    return 2;
                }
            } else if (op == "--ge") {
                try {
                    double left = std::stod(leftValue);
                    double right = std::stod(rightValue);
                    return left >= right ? 0 : 1;
                } catch (const std::exception&) {
                    output << "test: integer expression expected instead i found: " << leftValue << " " << rightValue << std::endl;
                    return 2;
                }
            } else if (op == "--lt") {
                try {
                    double left = std::stod(leftValue);
                    double right = std::stod(rightValue);
                    return left < right ? 0 : 1; 
                } catch (const std::exception&) {
                    output << "test: integer expression expected instead i found: " << leftValue << " " << rightValue << std::endl;
                    return 2;
                }
            } else if (op == "--le") {
                try {
                    double left = std::stod(leftValue);
                    double right = std::stod(rightValue);
                    return left <= right ? 0 : 1;
                } catch (const std::exception&) {
                    output << "test: integer expression expected instead i found: " << leftValue << " " << rightValue << std::endl;
                    return 2;
                }
            } else {
                output << "test: unknown operator: " << op << std::endl;
                return 2;
            }
        }
        if (args.size() == 1) {
            return !getVar(args[0]).empty() ? 0 : 1;
        }
        return 1;
    }

    int cmdCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream& output) {
        if (args.empty()) {
            output << "cmd: missing file operand" << std::endl;
            throw AstFailure("Exception: Script: execution failed missing operand.\n");
        }

        std::string filename = getVar(args[0]);
        if (filename.size() >= 2 && 
            ((filename.front() == '"' && filename.back() == '"') || 
             (filename.front() == '\'' && filename.back() == '\''))) {
            filename = filename.substr(1, filename.size() - 2);
        }
        cmd::app_name = filename;
        std::ifstream file(filename);
        if (!file.is_open()) {
            output << "cmd: cannot open file '" << filename << "': No such file or directory" << std::endl;
            throw AstFailure("Exception: Script: " + filename + " execution failed. File not found\n");
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string script = buffer.str();
        if(!script.empty()) {
            script.erase(
                std::remove(script.begin(), script.end(), '\r'),
                    script.end()
            );
        }
        try {
            cmd::argv.clear();
            for(size_t i = 1; i < args.size(); ++i) {
                cmd::argv.push_back(getVar(args[i]));
            }
        } catch(state::StateException &e) {
            throw AstFailure("Exception: Script: State Exception execution failed.\n");
            return 1;
        }

        try {
            scan::TString string_buffer(script);
            scan::Scanner scanner(string_buffer);
            cmd::Parser parser(scanner);
            auto ast = parser.parse();
            cmd::AstExecutor &scriptExecutor = cmd::AstExecutor::getExecutor();

#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
            program_running = 1;
#endif
            scriptExecutor.execute(input, output, ast); 
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
            program_running = 0;
#endif

            return 0;
        } catch (const scan::ScanExcept &e) {
            output << "cmd: Scan error in " << filename << ": " << e.why() << std::endl;
            return 1;
        } catch (const std::exception &e) {
            output << "cmd: Error in " << filename << ": " << e.what() << std::endl;
            return 1;
        } 
        catch(const AstFailure &e) {
            throw AstFailure("Exception: Script: " + filename + " execution failed: " + std::string(e.what()) + "\n");
        } 
        catch (...) {
            output << "cmd: Unknown error occurred while executing " << filename << std::endl;
            return 1;
        }
        return 0;
    }

    int visualCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream& output) {
        if(args.empty() || args.size() != 2) {
            output << "Usage: visual script output_filel.\n";
            return 1;
        }
        std::string op1 = getVar(args[0]), op2 = getVar(args[1]);
        std::ifstream infile(op1);
        if(!infile.is_open()) {
            output << "Could not open file: " << op1 << "\n";
            return 1;
        }
        std::ostringstream stream;
        stream <<  infile.rdbuf();

        std::ofstream ofile(op2);
        if(!ofile.is_open()) {
            output << "Could not create output file:" << op2 << "\n";
            return 1;
        }

        try {
            scan::TString string_buffer(stream.str());
            scan::Scanner scanner(string_buffer);
            cmd::Parser parser(scanner);
            auto ast = parser.parse();
            html::gen_html(ofile, ast);
            return 0;
        }  
        catch (const scan::ScanExcept &e) {
            output << "cmd: Scan error in " << op1 << ": " << e.why() << std::endl;
            return 1;
        } catch (const std::exception &e) {
            output << "cmd: Error in " << op1 << ": " << e.what() << std::endl;
            return 1;
        }
        catch (...) {
            output << "cmd: Unknown error occurred while executing " << op1 << std::endl;
            return 1;
        }
        return 0;
    }

    int atCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output) {
        if(args.empty() || args.size() != 2) {
            output << "Usage: at <list> index\n";
            return 1;
        }

        std::string op1= getVar(args[0]);
        std::string op2= getVar(args[1]);
        std::istringstream stream(op1);
        std::vector<std::string> values;
        std::string line;
        while(std::getline(stream, line)) {
            values.push_back(line);
        }
        int index = std::stoi(op2);

        if(index >= 0 && index < static_cast<int>(values.size()))
            output << values.at(index);
        return 0;
    }
    int lenCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output) {
        if(args.empty() || args.size() != 1) {
            output << "Usage: len <list>\n";
            return 1;
        }
        std::string op1 = getVar(args[0]);          
        
        if (op1.find('\n') != std::string::npos) {
        
            std::istringstream stream(op1);
            std::vector<std::string> values;
            std::string line;
            while(std::getline(stream, line)) {
                if (!line.empty()) {
                    values.push_back(line);
                }
            }
            output << values.size();
        } else {
        
            std::istringstream stream(op1);
            std::vector<std::string> values;
            std::string word;
            while(stream >> word) {
                values.push_back(word);
            }
            output << values.size();
        }
        return 0;
    }
    // index <string> start len
    int indexCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output) {
        if(args.empty() || args.size() != 3) {
            output << "Usage: index <string> start leenn\n";
            return 1;
        }

        std::string operands[3];
        operands[0] = getVar(args[0]);
        operands[1] = getVar(args[1]);
        operands[2] = getVar(args[2]);

        int val[2];
        val[0] = std::stoi(operands[1]);
        val[1] = std::stoi(operands[2]);

        if(val[0] >= 0 && val[0] < static_cast<int>(operands[0].length()))  {
            std::string temp = operands[0].substr(val[0], val[1]);
            output << temp;
            return 0;
        }
        return 1;
    }

    int strlenCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output) {
        if(args.empty() || args.size() != 1) {
            output <<  "Usage: strlen <string>\n";
            return 1;
        }
        std::string v = getVar(args[0]);
        output << v.length();
        return 0;
    }

    int strfindCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output) {
         if(args.empty() && args.size() != 3) {
            output << "Usage: <start> <string> <search>\n";
            return 1;
         }

         std::string op1 = getVar(args[0]);
         std::string op2 = getVar(args[1]);
         std::string op3 = getVar(args[2]);

         int index = std::stoi(op1);
         auto pos = op2.find(op3, index);
         if(pos == std::string::npos)
            output << "-1";
        else
            output << pos;

         return 0;
    }

    int strfindrCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output) {
         if(args.empty() && args.size() != 3) {
            output << "Usage: <start> <string> <search>\n";
            return 1;
         }

         std::string op1 = getVar(args[0]);
         std::string op2 = getVar(args[1]);
         std::string op3 = getVar(args[2]);

         int index = std::stoi(op1);
         auto pos = op2.rfind(op3, index);
         if(pos == std::string::npos)
            output << "-1";
        else
            output << pos;

         return 0;
    }

    int strtokCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output) {
        if(args.empty() || args.size() != 2) {
            output << "Usage: strtok string seperator\n";
            return 1;
        }
        std::string op1 = getVar(args[0]);
        std::string sep = getVar(args[1]);
        if(sep.empty()) {
            for(char c : op1) {
                output << c << std::endl;
            }
            return 0;
        }
        size_t pos = 0;
        size_t found;
        while((found = op1.find(sep, pos)) != std::string::npos) {
            output << op1.substr(pos, found - pos) << std::endl;
            pos = found + sep.length();
        }
        if(pos < op1.length()) {
            output << op1.substr(pos) << std::endl;
        }
        return 0;
    }

    int execCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output) {
        std::ostringstream all_args;
        for(auto &arg : args) {
            try {
                all_args << getVar(arg) << " ";
            } catch(const std::runtime_error &) {
                all_args << arg.value << " ";
            }
        }
        std::string command_str = all_args.str();
#if !defined(__EMSCRIPTEN__) && !defined(_WIN32) && defined(__linux__)
        int master_fd, slave_fd;
        pid_t pid;
        
        if (openpty(&master_fd, &slave_fd, NULL, NULL, NULL) == -1) {
            output << "exec: failed to create pseudo-terminal" << std::endl;
            return 1;
        }
        
        pid = fork();
        if (pid < 0) {
            output << "exec: failed to fork process" << std::endl;
            close(master_fd);
            close(slave_fd);
            return 1;
        }
        
        if (pid == 0) {
            // Child process
            close(master_fd);
            setsid();
            ioctl(slave_fd, TIOCSCTTY, 0);
            
            dup2(slave_fd, STDIN_FILENO);
            dup2(slave_fd, STDOUT_FILENO);
            dup2(slave_fd, STDERR_FILENO);
            close(slave_fd);
            
            execl("/bin/sh", "sh", "-c", command_str.c_str(), NULL);
            exit(127);
        } else {
            // Parent process
            close(slave_fd);
            
            int flags = fcntl(master_fd, F_GETFL, 0);
            fcntl(master_fd, F_SETFL, flags | O_NONBLOCK);
            
            output << std::unitbuf;
            
            bool still_running = true;
            std::string line_buffer;
            
            while (still_running) {
                if (cmd::AstExecutor::getExecutor().checkInterrupt()) {
                    std::cout << "exec: interrupting process" << std::endl;
                    if(killpg(pid, SIGINT) < 0) {
                        output << "exec: failed to kill process" << std::endl;
                    }
                    usleep(100000); 
                    if (waitpid(pid, NULL, WNOHANG) == 0) {
                        killpg(pid, SIGKILL);
                    }
                    close(master_fd);
                    still_running = false;
                    AstExecutor::getExecutor().setInterruptValue(false);
                    break;
                }
                
                fd_set read_fds;
                FD_ZERO(&read_fds);
                FD_SET(master_fd, &read_fds);
                
                struct timeval tv;
                tv.tv_sec = 0;
                tv.tv_usec = 10000;
                
                int ret = select(master_fd + 1, &read_fds, NULL, NULL, &tv);
                
                if (ret > 0) {
                    char buffer[4096];
                    ssize_t bytes_read;
                    
                    while ((bytes_read = read(master_fd, buffer, sizeof(buffer) - 1)) > 0) {
                        buffer[bytes_read] = '\0';
                        line_buffer += buffer;
                        
                        size_t pos = 0;
                        while ((pos = line_buffer.find('\n')) != std::string::npos) {
                            std::string line = line_buffer.substr(0, pos + 1);
                            if(&output == &std::cout) {
                                output << line;
                                output.flush();
                            } else {
                                AstExecutor::getExecutor().execUpdateCallback(line);
                            }
                            line_buffer.erase(0, pos + 1);
                        } 
                        if (line_buffer.length() > 40) {
                            if(&output == &std::cout) {
                                output << line_buffer;
                                output.flush();
                            } else {
                                AstExecutor::getExecutor().execUpdateCallback(line_buffer);
                            }
                            line_buffer.clear();
                        }
                    }   
                    if (bytes_read == 0) {
                        if (!line_buffer.empty()) {
                            if(&output == &std::cout) {
                                output << line_buffer;
                                output.flush();
                            } else {
                                AstExecutor::getExecutor().execUpdateCallback(line_buffer);
                            }
                        }
                        still_running = false;
                    }
                    
                    if (bytes_read < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                        still_running = false;
                    }
                    
                } else if (ret == 0) {
                    int status;
                    pid_t result = waitpid(pid, &status, WNOHANG);
                    if (result != 0) {
                        if (!line_buffer.empty()) {
                            if(&output == &std::cout) {
                                output << line_buffer;
                                output.flush();
                            } else {
                                AstExecutor::getExecutor().execUpdateCallback(line_buffer);
                            }
                        }
                        still_running = false;
                    }
                } else {
    
                    if (errno != EINTR) {
                        break;
                    }
                }
            }
        }
        int status;
        waitpid(pid, &status, 0);
        close(master_fd);    
        output << std::nounitbuf;
        return WEXITSTATUS(status);
#elif defined(_WIN32) && !defined(__EMSCRIPTEN__)
    std::istringstream iss(command_str);
    std::string first_word;
    iss >> first_word;
    
    if (first_word.ends_with(".exe") || first_word.ends_with(".EXE")) {
        // Your existing .exe handling code stays the same...
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

        // Use the command directly without bash
        if (!CreateProcess(NULL, const_cast<LPSTR>(command_str.c_str()), 
                          NULL, NULL, TRUE, CREATE_NO_WINDOW, 
                          NULL, NULL, &si, &pi)) {
            CloseHandle(hStdOutRead);
            CloseHandle(hStdOutWrite);
            CloseHandle(hStdInRead);
            CloseHandle(hStdInWrite);
            output << "exec: failed to create process: " << command_str << std::endl;
            return 1;
        }

        CloseHandle(hStdOutWrite);
        CloseHandle(hStdInRead);
        CloseHandle(hStdInWrite);

        // Check if we're outputting to std::cout for special handling
        bool isStdOut = (&output == &std::cout);
        
        std::string line_buffer;
        bool still_running = true;
        
        while (still_running) {
            DWORD bytesAvailable = 0;
            if (!PeekNamedPipe(hStdOutRead, NULL, 0, NULL, &bytesAvailable, NULL)) {
                still_running = false;
                break;
            }
            
            if (bytesAvailable > 0) {
                char buffer[256];
                DWORD bytesRead = 0;
                
                if (ReadFile(hStdOutRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
                    buffer[bytesRead] = '\0';
                    line_buffer += buffer;
                    
                    // Process complete lines
                    size_t pos = 0;
                    while ((pos = line_buffer.find('\n')) != std::string::npos) {
                        std::string line = line_buffer.substr(0, pos + 1);
                        
                        if (isStdOut) {
                            printf("%s", line.c_str());
                            fflush(stdout);
                        } else {
                            output << line;
                            output.flush();
                        }
                        
                        line_buffer.erase(0, pos + 1);
                    }
                    
                    // Handle long lines without newlines
                    if (line_buffer.length() > 80) {
                        if (isStdOut) {
                            printf("%s", line_buffer.c_str());
                            fflush(stdout);
                        } else {
                            output << line_buffer;
                            output.flush();
                        }
                        line_buffer.clear();
                    }
                } else {
                    still_running = false;
                    break;
                }
            } else {
                DWORD currentChildExitCode;
                if (GetExitCodeProcess(pi.hProcess, &currentChildExitCode)) {
                    if (currentChildExitCode != STILL_ACTIVE) {
                        // Output any remaining buffer
                        if (!line_buffer.empty()) {
                            if (isStdOut) {
                                printf("%s", line_buffer.c_str());
                                fflush(stdout);
                            } else {
                                output << line_buffer;
                                output.flush();
                            }
                        }
                        still_running = false;
                    }
                } else {
                    still_running = false;
                    break;
                }
                
                if (still_running) {
                    Sleep(10);
                }
            }
        }
        
        DWORD final_child_exit_code = 0;
        WaitForSingleObject(pi.hProcess, INFINITE);
        GetExitCodeProcess(pi.hProcess, &final_child_exit_code);
        
        CloseHandle(hStdOutRead);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        
        return static_cast<int>(final_child_exit_code);
    } else {
        // ADD THIS ELSE BLOCK for non-.exe commands
        // Use cmd_type (which should be "cmd.exe /c " or similar) for other commands
        std::string fullCommand = cmd::cmd_type + " " + command_str;
        
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

        if (!CreateProcess(NULL, const_cast<LPSTR>(fullCommand.c_str()), 
                          NULL, NULL, TRUE, CREATE_NO_WINDOW, 
                          NULL, NULL, &si, &pi)) {
                             DWORD error = GetLastError();
            printf("exec: failed to create process: %s Error: %ld\n ",fullCommand.c_str(), error);
            fflush(stdout);
   
            CloseHandle(hStdOutRead);
            CloseHandle(hStdOutWrite);
            CloseHandle(hStdInRead);
            CloseHandle(hStdInWrite);
            output << "exec: failed to create process: " << fullCommand << std::endl;
            return 1;
        }

        CloseHandle(hStdOutWrite);
        CloseHandle(hStdInRead);
        CloseHandle(hStdInWrite);

        // Same output handling as above...
        bool isStdOut = (&output == &std::cout);
        std::string line_buffer;
        bool still_running = true;
        
        while (still_running) {
            DWORD bytesAvailable = 0;
            if (!PeekNamedPipe(hStdOutRead, NULL, 0, NULL, &bytesAvailable, NULL)) {
                still_running = false;
                break;
            }
            
            if (bytesAvailable > 0) {
                char buffer[256];
                DWORD bytesRead = 0;
                
                if (ReadFile(hStdOutRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
                    buffer[bytesRead] = '\0';
                    line_buffer += buffer;
                    
                    size_t pos = 0;
                    while ((pos = line_buffer.find('\n')) != std::string::npos) {
                        std::string line = line_buffer.substr(0, pos + 1);
                        
                        if (isStdOut) {
                            printf("%s", line.c_str());
                            fflush(stdout);
                        } else {
                            output << line;
                            output.flush();
                        }
                        
                        line_buffer.erase(0, pos + 1);
                    }
                    
                    if (line_buffer.length() > 80) {
                        if (isStdOut) {
                            printf("%s", line_buffer.c_str());
                            fflush(stdout);
                        } else {
                            output << line_buffer;
                            output.flush();
                        }
                        line_buffer.clear();
                    }
                } else {
                    still_running = false;
                    break;
                }
            } else {
                DWORD currentChildExitCode;
                if (GetExitCodeProcess(pi.hProcess, &currentChildExitCode)) {
                    if (currentChildExitCode != STILL_ACTIVE) {
                        if (!line_buffer.empty()) {
                            if (isStdOut) {
                                printf("%s", line_buffer.c_str());
                                fflush(stdout);
                            } else {
                                output << line_buffer;
                                output.flush();
                            }
                        }
                        still_running = false;
                    }
                } else {
                    still_running = false;
                    break;
                }
                
                if (still_running) {
                    Sleep(10);
                }
            }
        }
        
        DWORD final_child_exit_code = 0;
        WaitForSingleObject(pi.hProcess, INFINITE);
        GetExitCodeProcess(pi.hProcess, &final_child_exit_code);
        
        CloseHandle(hStdOutRead);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        
        return static_cast<int>(final_child_exit_code);
    }
#endif
    }

    int commandListCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output) {
        AstExecutor::printCommandInfo(output);
        return 0;
    }

    int argvCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output) {
        if (args.empty()) {
            output << "Usage: argv <index>\n";
            return 1;
        }
        if(args[0].type == ArgType::ARG_STRING_LITERAL && args[0].value == "length") {
            output << argv.size();
            return 0;
        } else if(args[0].type == ArgType::ARG_STRING_LITERAL && args[0].value == "all") {
            for (size_t i = 0; i < argv.size(); ++i) {
                output << argv.at(i) << "\n";
            }
            return 0;
        } else if(args[0].type == ArgType::ARG_STRING_LITERAL && args[0].value == "app") {
            output << app_name;
            return 0;
        } 
        else if(args[0].type == ArgType::ARG_STRING_LITERAL) {
            output << "argv: invalid argument: " << args[0].value << "\n";
            return 1;
        }
        int index = std::stoi(getVar(args[0]));
        if (index >= 0 && index < static_cast<int>(argv.size())) {
            output << argv.at(index);
        } else {
            output << "argv: index out of range\n";
            return 1;
        }
        return 0;
    }

    int externCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output) {
 
        if(args.empty() || args.size() != 3) {
            output << "Usage: extern <library> <function> <command_name>\n";
            output << "You have: ";
            for(auto &arg : args) {
                output << arg.value << " ";
            }
            output << "\n";
            return 1;
        }
        std::string libPath = getVar(args[0]);
        std::string funcName = getVar(args[1]);
        std::string cmdName = getVar(args[2]);
        
        auto &reg = AstExecutor::getRegistry();
        std::shared_ptr<Library> &lib = reg.setLibrary(libPath);
        if(!lib) {
            std::cerr << "extern: failed to load library " << libPath << "\n";
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
            program_running = 0;
#else
            //exit(0);
#endif
            return 1;
        }
        if(!lib->hasSymbol(funcName)) {
            std::cerr << "extern: function " << funcName << " not found in library " << libPath << "\n";
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
            program_running = 0;
#else
            //exit(0);
#endif
            return 1;
        }
        ExternCommandInfo info;
        info.library = lib;
        info.func = lib->getFunction<int(*)(const std::vector<cmd::Argument>&, std::istream&, std::ostream&)>(funcName);
        info.functionName = funcName;
        info.libraryPath = libPath;

        if(info.func) {
            reg.registerExternCommand(cmdName, info);            
            return 0;
        } else {
            std::cerr << "extern: failed to get function pointer for " << funcName << "\n";
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
            program_running = 0;
#else
            //exit(0);
#endif
            return 1;
        }
    }

    
    int newListCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output) {
        if (args.empty()) {
            output << "Usage: list_new <name>\n";
            return 1;
        }
        std::string name = getVar(args[0]);
        state::GameState *gameState = state::getGameState();
        if (gameState->hasList(name)) {
            output << "Error: List '" << name << "' already exists." << std::endl;
            return 1;
        }
        gameState->createList(name);
        if(args.size() > 2) {
            std::string value = getVar(args[2]);
            std::string value2 = getVar(args[1]);
            gameState->initList(name, value, std::stoul(value2));
        }
        return 0;
    }
    int newListAddCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output) {
        if (args.size() < 2) {
            output << "Usage: list_add <name> <value>\n";
            return 1;
        }
        std::string name = getVar(args[0]);
        std::string value = getVar(args[1]);
        state::GameState *gameState = state::getGameState();
        if (!gameState->hasList(name)) {
            output << "Error: List '" << name << "' already exists." << std::endl;
            return 1;
        }
        gameState->addToList(name, value);
        return 0;
    }
    int newListRemoveCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output) {
        if (args.size() < 2) {
            output << "Usage: list_remove <name> <value>\n";
            return 1;
        }
        std::string name = getVar(args[0]);
        std::string value = getVar(args[1]);
        state::GameState *gameState = state::getGameState();
        if (!gameState->hasList(name)) {
            output << "Error: List '" << name << "' already exists." << std::endl;
            return 1;
        }
        size_t val = std::stoul(value);
        gameState->removeFromList(name, val);
        return 0;
    }
    int newListGetCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output) {
        if (args.size() < 2) {
            throw cmd::AstFailure("Usage: list_get <name> <index>");
            return 1;
        }
        std::string name = getVar(args[0]);
        std::string indexStr = getVar(args[1]);
        state::GameState *gameState = state::getGameState();
        if (!gameState->hasList(name)) {
            throw cmd::AstFailure("Error: List '" + name + "' does not exist.");
            return 1;
        }
        int index = std::stoi(indexStr);
        std::string value = gameState->getFromList(name, index);
        if (value.empty()) {
            throw cmd::AstFailure( "Error: Index out of range.");
            return 1;
        }
        output << value;
        return 0;
    }

    int newListSetCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output) {
        if (args.size() < 3) {
            output << "Usage: list_set <name> <index> <value>\n";
            return 1;
        }
        std::string name = getVar(args[0]);
        std::string indexStr = getVar(args[1]);
        std::string value = getVar(args[2]);
        state::GameState *gameState = state::getGameState();
        if (!gameState->hasList(name)) {
            output << "Error: List '" << name << "' does not exist." << std::endl;
            return 1;
        }
        size_t index = std::stoul(indexStr);
        gameState->setList(name, index, value);
        return 0;
    }
    
    int newListClearCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output) {
        if (args.empty()) {
            output << "Usage: list_clear <name>\n";
            return 1;
        }
        std::string name = getVar(args[0]);
        state::GameState *gameState = state::getGameState();
        if (!gameState->hasList(name)) {
            output << "Error: List '" << name << "' does not exist." << std::endl;
            return 1;
        }
        gameState->clearList(name);
        return 0;
    }
    int newListClearAllCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output) {
        state::GameState *gameState = state::getGameState();
        gameState->clearAllLists();
        return 0;
    }
    int newListExistsCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output) {
        if (args.empty()) {
            throw cmd::AstFailure("Usage: list_exists <name>");
            return 1;
        }
        std::string name = getVar(args[0]);
        state::GameState *gameState = state::getGameState();
        if (gameState->hasList(name)) {
            output << "0";
        } else {
            output << "1";
        }
        return 0;
    }
    int newListInitCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output) {
        if(args.empty()) {
            output << "Usage: list_init <name> <size> <value>\n";
            return 1;
        }
        std::string name = getVar(args[0]);
        std::string value = getVar(args[2]);
        std::string sizeStr = getVar(args[1]);
        state::GameState *gameState = state::getGameState();
        if (!gameState->hasList(name)) {
            output << "Error: List '" << name << "' does not exist." << std::endl;
            return 1;
        }
        size_t size = std::stoul(sizeStr);
        gameState->initList(name, value, size);
        return 0;
    }

    int newListLenCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output) {
        if (args.empty()) {
            throw cmd::AstFailure("Usage: list_len <name>");
            return 1;
        }
        std::string name = getVar(args[0]);
        state::GameState *gameState = state::getGameState();
        if (!gameState->hasList(name)) {
            throw cmd::AstFailure("Error: List '" + name + "' does not exist.");
            return 1;
        }
        size_t len = gameState->getListLength(name);
        output << len;
        return 0;
    }

    int newListTokens(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output) {
        if(args.empty()) {
            throw cmd::AstFailure("Usage: list_tokens <name>");
            return 1;
        }   
        std::string name = getVar(args[0]);
        state::GameState *gameState = state::getGameState();
        if (!gameState->hasList(name)) {
            throw cmd::AstFailure("Error: List '" + name + "' does not exist.");
            return 1;
        }
        for(size_t i = 0; i < gameState->getListLength(name); ++i) {
            std::string value = gameState->getFromList(name, i);
            output << value << "\n";
        }
        return 0;
    }

    
    int newListSortCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output) {
        if (args.empty()) {
            throw cmd::AstFailure("Usage: list_sort <name>");
            return 1;
        }
        std::string name = getVar(args[0]); 
        state::GameState *gameState = state::getGameState();
        if (!gameState->hasList(name)) {
            throw cmd::AstFailure("Error: List '" + name + "' does not exist.");
            return 1;
        }
        gameState->sortList(name);  
        return 0;
    }
    int newListReverseCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output) {
        if (args.empty()) {
            throw cmd::AstFailure("Usage: list_reverse <name>");
            return 1;
        }
        std::string name = getVar(args[0]);
        state::GameState *gameState = state::getGameState();
        if (!gameState->hasList(name)) {
            throw cmd::AstFailure("Error: List '" + name + "' does not exist.");
            return 1;
        }
        gameState->reverseList(name);   
        return 0;
    }
    int newListShuffleCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream  &output) {
        if (args.empty()) {
            throw cmd::AstFailure("Usage: list_shuffle <name>");
            return 1;
        }
        std::string name = getVar(args[0]); 
        state::GameState *gameState = state::getGameState();
        if (!gameState->hasList(name)) {
            throw cmd::AstFailure("Error: List '" + name + "' does not exist.");
            return 1;
        }   
        gameState->shuffleList(name);
        return 0;
    }

    int newListCopyCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output) {
        if (args.empty()) {
            throw cmd::AstFailure("Usage: list_copy <name>");
            return 1;
        }
        if(args.size() < 2) {
            throw cmd::AstFailure("Usage: list_copy <name> <name2>");
            return 1;
        }
        std::string name1 = getVar(args[0]);
        std::string name2 = getVar(args[1]);

        state::GameState *gameState = state::getGameState();
        if (!gameState->hasList(name1)) {
            throw cmd::AstFailure("Error: List '" + name1 + "' does not exist.");
            return 1;
        }
        gameState->copyList(name1, name2);
        return 0;
    }

    int newListPopCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output) {
        if (args.empty()) {
            throw cmd::AstFailure("Usage: list_pop <name>");
            return 1;
        }
        std::string name = getVar(args[0]);
        state::GameState *gameState = state::getGameState();
        if (!gameState->hasList(name)) {
            throw cmd::AstFailure("Error: List '" + name + "' does not exist.");
            return 1;
        }
        gameState->popList(name);
        return 0;
    }

    int concatListCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output) {
        if (args.empty()) {
            throw cmd::AstFailure("Usage: list_concat <name1> <name2>");
            return 1;
        }
        if(args.size() < 2) {
            throw cmd::AstFailure("Usage: list_concat <name1> <name2> <name3>");
            return 1;
        }
        std::string name1 = getVar(args[0]);
        std::string name2 = getVar(args[1]);
        

        state::GameState *gameState = state::getGameState();
        if (!gameState->hasList(name1)) {
            throw cmd::AstFailure("Error: List '" + name1 + "' does not exist.");
            return 1;
        }
        if(!gameState->hasList(name2)) {
            throw cmd::AstFailure("Error: List '" + name2 + "' does not exist.");
            return 1;
        }
        gameState->concatList(name1, name2);
        return 0;
    }

    int newRandCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output) {
        if(args.empty()) {
            throw cmd::AstFailure("Usage: rand <min> <max>");
            return 1;
        }
        std::string minStr = getVar(args[0]);
        std::string maxStr = getVar(args[1]);
        int min = std::stoi(minStr);
        int max = std::stoi(maxStr);
        if(min > max) {
            throw cmd::AstFailure("Usage: rand <min> <max>");
            return 1;
        }
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(min, max);
        int randomNum = dis(gen);
        output << randomNum;    
        return 0;
    }

    namespace expr_parser {
        
        enum class ExprTokenType {
            END, NUMBER, PLUS, MINUS, MUL, DIV, MOD, LPAREN, RPAREN,
            PLUSEQ, MINUSEQ, XOR, OR, AND,
            LOGICAL_AND, LOGICAL_OR, NOT,
            EQ, NEQ, GT, GTE, LT, LTE  
        };

        struct ExprToken {
            ExprTokenType type;
            int value; 
        };

        class ExprLexer {
        public:
            ExprLexer(const std::string& input) : scanner(input), pos(0) {
                scanner.scan();
                next();
            }

            void next() {
                if (pos >= scanner.size()) {
                    current = {ExprTokenType::END, 0};
                    return;
                }
                auto t = scanner[pos++];
                auto val = t.getTokenValue();
                if (t.getTokenType() == types::TokenType::TT_NUM) {
                    current = {ExprTokenType::NUMBER, std::stoi(val)};
                } else if (val == "+") {
                    current = {ExprTokenType::PLUS, 0};
                } else if (val == "-") {
                    current = {ExprTokenType::MINUS, 0};
                } else if (val == "*") {
                    current = {ExprTokenType::MUL, 0};
                } else if (val == "/") {
                    current = {ExprTokenType::DIV, 0};
                } else if (val == "%") {
                    current = {ExprTokenType::MOD, 0};
                } else if (val == "(") {
                    current = {ExprTokenType::LPAREN, 0};
                } else if (val == ")") {
                    current = {ExprTokenType::RPAREN, 0};
                } else if (val == "+=") {
                    current = {ExprTokenType::PLUSEQ, 0};
                } else if (val == "-=") {
                    current = {ExprTokenType::MINUSEQ, 0};
                } else if (val == "^") {
                    current = {ExprTokenType::XOR, 0};
                } else if (val == "|") {
                    current = {ExprTokenType::OR, 0};
                } else if (val == "&") {
                    current = {ExprTokenType::AND, 0};
                } else if (val == "&&") {
                    current = {ExprTokenType::LOGICAL_AND, 0};
                } else if (val == "||") {
                    current = {ExprTokenType::LOGICAL_OR, 0};
                } else if (val == "!") {
                    current = {ExprTokenType::NOT, 0};
                } 
                else if (val == "=") {
                    current = {ExprTokenType::EQ, 0};
                } else if (val == "!=") {
                    current = {ExprTokenType::NEQ, 0};
                } else if (val == ">") {
                    current = {ExprTokenType::GT, 0};
                } else if (val == ">=") {
                    current = {ExprTokenType::GTE, 0};
                } else if (val == "<") {
                    current = {ExprTokenType::LT, 0};
                } else if (val == "<=") {
                    current = {ExprTokenType::LTE, 0};
                }
                else {
                    current = {ExprTokenType::END, 0};
                }
            }

            ExprToken peek() const { return current; }
            void consume() { next(); }

        private:
            scan::Scanner scanner;
            size_t pos;
            ExprToken current;
        };

        class ExprParser {
        public:
            ExprParser(ExprLexer& lexer) : lexer(lexer) {}
            int parse() { return parseLogical(); }

        private:
            ExprLexer& lexer;

            int parseAdd() {
                int val = parseTerm();
                while (lexer.peek().type == ExprTokenType::PLUS ||
                    lexer.peek().type == ExprTokenType::MINUS) {
                    if (lexer.peek().type == ExprTokenType::PLUS) {
                        lexer.consume();
                        val += parseTerm();
                    } else {
                        lexer.consume();
                        val -= parseTerm();
                    }
                }
                return val;
            }

            int parseBitwise() {
                int val = parseAdd();  
                while (lexer.peek().type == ExprTokenType::XOR ||
                    lexer.peek().type == ExprTokenType::OR  ||
                    lexer.peek().type == ExprTokenType::AND) {
                    if (lexer.peek().type == ExprTokenType::XOR) {
                        lexer.consume();
                        val ^= parseAdd();
                    } else if (lexer.peek().type == ExprTokenType::OR) {
                        lexer.consume();
                        val |= parseAdd();
                    } else {  
                        lexer.consume();
                        val &= parseAdd();
                    }
                }
                return val;
            }

            int parseComparison() {
                int val = parseBitwise();
                while (lexer.peek().type == ExprTokenType::EQ ||
                       lexer.peek().type == ExprTokenType::NEQ ||
                       lexer.peek().type == ExprTokenType::GT ||
                       lexer.peek().type == ExprTokenType::GTE ||
                       lexer.peek().type == ExprTokenType::LT ||
                       lexer.peek().type == ExprTokenType::LTE) {
                    if (lexer.peek().type == ExprTokenType::EQ) {
                        lexer.consume();
                        val = (val == parseBitwise());
                    } else if (lexer.peek().type == ExprTokenType::NEQ) {
                        lexer.consume();
                        val = (val != parseBitwise());
                    } else if (lexer.peek().type == ExprTokenType::GT) {
                        lexer.consume();
                        val = (val > parseBitwise());
                    } else if (lexer.peek().type == ExprTokenType::GTE) {
                        lexer.consume();
                        val = (val >= parseBitwise());
                    } else if (lexer.peek().type == ExprTokenType::LT) {
                        lexer.consume();
                        val = (val < parseBitwise());
                    } else {  
                        lexer.consume();
                        val = (val <= parseBitwise());
                    }
                }
                return val;
            }

            int parseLogical() {
                int val = parseComparison();
                while (lexer.peek().type == ExprTokenType::LOGICAL_OR ||
                       lexer.peek().type == ExprTokenType::LOGICAL_AND) {
                    if (lexer.peek().type == ExprTokenType::LOGICAL_OR) {
                        lexer.consume();
                        val = val || parseComparison();
                    } else {
                        lexer.consume();
                        val = val && parseComparison();
                    }
                }
                return val;
            }

            int parseTerm() {
                int val = parseFactor();
                while (lexer.peek().type == ExprTokenType::MUL ||
                    lexer.peek().type == ExprTokenType::DIV ||
                    lexer.peek().type == ExprTokenType::MOD) {
                    if (lexer.peek().type == ExprTokenType::MUL) {
                        lexer.consume();
                        val *= parseFactor();
                    } else if (lexer.peek().type == ExprTokenType::DIV) {
                        lexer.consume();
                        int d = parseFactor();
                        if (d == 0) throw std::runtime_error("Division by zero");
                        val /= d;
                    } else {
                        lexer.consume();
                        int d = parseFactor();
                        if (d == 0) throw std::runtime_error("Modulo by zero");
                        val %= d;
                    }
                }
                return val;
            }

            int parseFactor() {
                if (lexer.peek().type == ExprTokenType::NOT) {
                    lexer.consume();
                    return !parseFactor();
                } else if (lexer.peek().type == ExprTokenType::MINUS) {
                    lexer.consume();
                    return -parseFactor();
                } else if (lexer.peek().type == ExprTokenType::NUMBER) {
                    int v = lexer.peek().value;
                    lexer.consume();
                    return v;
                } else if (lexer.peek().type == ExprTokenType::LPAREN) {
                    lexer.consume();
                    int v = parseLogical();
                    if (lexer.peek().type != ExprTokenType::RPAREN)
                        throw std::runtime_error("Expected ')'");
                    lexer.consume();
                    return v;
                }
                throw std::runtime_error("Unexpected token");
            }
        };
    }

    int exprCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output) {
        if (args.empty()) {
            throw AstFailure("Usage: expr <expression>\n");
            return 1;
        }
        std::string expression = getVar(args[0]);
        try {
            expr_parser::ExprLexer lexer(expression);
            expr_parser::ExprParser parser(lexer);
            int result = parser.parse();
            output << result;
            return 0;
        } catch (const std::exception& e) {
            throw AstFailure("expr: error: " + std::string(e.what()));
            return 1;
        }
    }

    int getLineCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output) {
        if (args.empty()) {
            output << "Usage: getline var\n";
            return 1;
        }
        std::string line;
        std::getline(input, line);
        if(line.empty()) {
            return 0;
        }

        state::GameState *gameState = state::getGameState();
        std::string var = args[0].value;
        gameState->setVariable(var, line);
        return 0;
    }

    int regexMatchCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output) {
        if (args.empty() || args.size() != 2) {
            output << "Usage: regex_match <pattern> <string>\n";
            return 1;
        }
        std::string pattern = getVar(args[0]);
        std::string str = getVar(args[1]);
        std::regex re(pattern);
        if (std::regex_match(str, re)) {
            output << "1";
        } else {
            output << "0";
        }
        return 0;
    }
    int regexReplaceCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output) {
        if (args.empty() || args.size() != 3) {
            output << "Usage: regex_replace <pattern> <replacement> <string>\n";
            return 1;
        }
        std::string pattern = getVar(args[0]);
        std::string replacement = getVar(args[1]);
        std::string str = getVar(args[2]);
        std::regex re(pattern);
        std::string result = std::regex_replace(str, re, replacement);
        output << result;
        return 0;
    }
    int regexSearchCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output) {
        if (args.empty() || args.size() != 2) {
            output << "Usage: regex_search <pattern> <string>\n";
            return 1;
        }
        std::string pattern = getVar(args[0]);
        std::string str = getVar(args[1]);
        std::regex re(pattern);
        if (std::regex_search(str, re)) {
            output << "1";
        } else {
            output << "0";
        }
        return 0;
    }
    int regexSplitCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream &output) {
        if (args.empty() || args.size() != 2) {
            output << "Usage: regex_split <pattern> <string>\n";
            return 1;
        }
        std::string pattern = getVar(args[0]);
        std::string str = getVar(args[1]);
        std::regex re(pattern);
        std::sregex_token_iterator it(str.begin(), str.end(), re, -1);
        std::sregex_token_iterator end;
        for (; it != end; ++it) {
            output << *it << "\n";
        }
        return 0;
    }

    int tokenizeCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream& output) {
        if(args.empty()) {
            output << "Usage: tokenize <filename>\n";
            return 1; 
        }
        if(args.size() > 0) {
            std::string filename = getVar(args[0]);
            std::ostringstream stream;
            std::ifstream file(filename);
            if(!file.is_open()) {
                output << "tokenize: Failure to load file: " <<  filename << "\n";
                return 1;
            }
            stream << file.rdbuf();
            std::string fileContent = stream.str();
            if(!fileContent.empty()) {
                fileContent.erase(
                    std::remove(fileContent.begin(), fileContent.end(), '\r'),
                        fileContent.end()
                );
            }
            try {
                scan::Scanner scanner(fileContent);
                scanner.scan();
                output << "Idx | Type           | Value\n";
                output  << "----+----------------+--------------------------\n";
                for (size_t i = 0; i < scanner.size(); ++i) {
                    std::ostringstream data;
                    types::print_type_TokenType(data,scanner[i].getTokenType());
                    output << std::setw(3) << i << " | "
                        << std::setw(14) << data.str() << " | "
                        << scanner[i].getTokenValue() << "\n";
                }        
            } catch(scan::ScanExcept &e) {
                output << e.why() << "\n";
                return 1;
            }
            return 0;
        }
        return 1;
    }
}
