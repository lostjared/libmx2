#include"command.hpp"
#include<fstream>
#include<filesystem>
#include<regex>
#include<sstream>
#include<iomanip>
#include<cstdlib>
#include"game_state.hpp"
#include"parser.hpp"
#include"html.hpp"
#include"command_reg.hpp"

#if !defined(__EMSCRIPTEN__) &&  !defined(_WIN32)
#include<unistd.h>
#include<sys/wait.h>
#endif
#if !defined(__EMSCRIPTEN__) && defined(_WIN32)
#include <windows.h>
#endif
namespace state {
    GameState *getGameState() {
        static GameState gameState;
        return &gameState;
    }
}

namespace cmd {

    std::vector<std::string> argv;

    int exitCommand(const std::vector<std::string>& args, std::istream& input, std::ostream& output) {
        std::exit(0);      
        return 0;     
    }
    
    int echoCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream& output) {
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

    int printfCommand(const std::vector<cmd::Argument>& args, std::istream& input, std::ostream& output) {
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
                    output << "test: integer expression expected" << std::endl;
                    return 2;
                }
            } else if (op == "--ne") {
                try {
                    double left = std::stod(leftValue);
                    double right = std::stod(rightValue);
                    return left != right ? 0 : 1; 
                } catch (const std::exception&) {
                    output << "test: integer expression expected" << std::endl;
                    return 2;
                }
            } else if (op == "--gt") {
                try {
                    double left = std::stod(leftValue);
                    double right = std::stod(rightValue);
                    return left > right ? 0 : 1; 
                } catch (const std::exception&) {
                    output << "test: integer expression expected" << std::endl;
                    return 2;
                }
            } else if (op == "--ge") {
                try {
                    double left = std::stod(leftValue);
                    double right = std::stod(rightValue);
                    return left >= right ? 0 : 1;
                } catch (const std::exception&) {
                    output << "test: integer expression expected" << std::endl;
                    return 2;
                }
            } else if (op == "--lt") {
                try {
                    double left = std::stod(leftValue);
                    double right = std::stod(rightValue);
                    return left < right ? 0 : 1; 
                } catch (const std::exception&) {
                    output << "test: integer expression expected" << std::endl;
                    return 2;
                }
            } else if (op == "--le") {
                try {
                    double left = std::stod(leftValue);
                    double right = std::stod(rightValue);
                    return left <= right ? 0 : 1;
                } catch (const std::exception&) {
                    output << "test: integer expression expected" << std::endl;
                    return 2;
                }
            } else {
                std::cout << "test: unknown operator: " << op << std::endl;
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
            return 1;
        }

        std::string filename = getVar(args[0]);
        if (filename.size() >= 2 && 
            ((filename.front() == '"' && filename.back() == '"') || 
             (filename.front() == '\'' && filename.back() == '\''))) {
            filename = filename.substr(1, filename.size() - 2);
        }

        std::ifstream file(filename);
        if (!file.is_open()) {
            output << "cmd: cannot open file '" << filename << "': No such file or directory" << std::endl;
            return 1;
        }


        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string script = buffer.str();

        try {
            scan::TString string_buffer(script);
            scan::Scanner scanner(string_buffer);
            cmd::Parser parser(scanner);
            auto ast = parser.parse();
            cmd::AstExecutor scriptExecutor;

#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
            program_running = 1;
#endif
            scriptExecutor.execute(input, output, ast); 


            return 0;
        } catch (const scan::ScanExcept &e) {
            output << "cmd: Scan error in " << filename << ": " << e.why() << std::endl;
            return 1;
        } catch (const std::exception &e) {
            output << "cmd: Error in " << filename << ": " << e.what() << std::endl;
            return 1;
        } 
        catch(const AstFailure &e) {
            throw AstFailure("Exception: Script: " + filename + " execution failed.\n");
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
#if !defined(__EMSCRIPTEN__)  && !defined(_WIN32)
        int in_pipe[2];
        int out_pipe[2];
        
        if (pipe(in_pipe) < 0 || pipe(out_pipe) < 0) {
            output << "exec: failed to create pipes" << std::endl;
            return 1;
        }
        
        pid_t pid = fork();
        if (pid < 0) {
            output << "exec: failed to fork process" << std::endl;
            return 1;
        }
        
        if (pid == 0) {
            close(in_pipe[1]);  
            dup2(in_pipe[0], STDIN_FILENO);
            close(in_pipe[0]);
            
            
            close(out_pipe[0]);  
            dup2(out_pipe[1], STDOUT_FILENO);
            dup2(out_pipe[1], STDERR_FILENO); 
            close(out_pipe[1]);
            
            execl("/bin/sh", "sh", "-c", command_str.c_str(), NULL);
            exit(127);  
        } else {
            
            close(in_pipe[0]);  
            close(out_pipe[1]); 
            
            close(in_pipe[1]);
            
            char buffer[4096];
            ssize_t bytes_read;
            while ((bytes_read = read(out_pipe[0], buffer, sizeof(buffer) - 1)) > 0) {
                buffer[bytes_read] = '\0';
                output << buffer;
                output.flush();
            }
            close(out_pipe[0]);
            
            int status;
            waitpid(pid, &status, 0);
            
            return WEXITSTATUS(status);
        }
#elif !defined(__EMSCRIPTEN__)  && defined(_WIN32)
        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = NULL;

        HANDLE hStdOutRead, hStdOutWrite;
        if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0)) {
            output << "exec: failed to create output pipe" << std::endl;
            return 1;
        }
        SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0); // Don't inherit read handle

        HANDLE hStdInRead, hStdInWrite;
        if (!CreatePipe(&hStdInRead, &hStdInWrite, &sa, 0)) {
            CloseHandle(hStdOutRead);
            CloseHandle(hStdOutWrite);
            output << "exec: failed to create input pipe" << std::endl;
            return 1;
        }
        SetHandleInformation(hStdInWrite, HANDLE_FLAG_INHERIT, 0); // Don't inherit write handle

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
#endif
        return 0;
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
        } else if(args[0].type == ArgType::ARG_STRING_LITERAL) {
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
            output << "extern: failed to load library " << libPath << "\n";
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
            program_running = 0;
#else
            exit(0);
#endif
            return 1;
        }
        if(!lib->hasSymbol(funcName)) {
            output << "extern: function " << funcName << " not found in library " << libPath << "\n";
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
            program_running = 0;
#else
            exit(0);
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
            output << "extern: failed to get function pointer for " << funcName << "\n";
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
            program_running = 0;
#else
            exit(0);
#endif
            return 1;
        }
    }
}
