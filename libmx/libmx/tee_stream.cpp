#include"tee_stream.hpp"
#include<filesystem>

namespace mx {
    static std::ofstream log_file("system.log.txt", std::ios::out);
    static std::ofstream error_file("error.log.txt", std::ios::out);
    TeeStream system_out(std::cout, log_file);
    TeeStream system_err(std::cerr, error_file);
    
     void redirect() {
        std::filesystem::path tempDir = std::filesystem::temp_directory_path();
        if (!log_file.is_open()) {
            std::filesystem::path logFilePath = tempDir / "system.log.txt";
            log_file.open(logFilePath, std::ios::out);
            if(!log_file.is_open()) {
                std::cerr << "Error";
            }
            system_out << "Redirected to: " << logFilePath << "\n";         
        }
        if (!error_file.is_open()) {
            std::filesystem::path logFilePath = tempDir / "error.log.txt";
            error_file.open(logFilePath, std::ios::out);
            if(!log_file.is_open()) {
                std::cerr << "Error";
            }
            system_out << "Redirected to: " << logFilePath << "\n";
        }

        std::cout.rdbuf(system_out.rdbuf());
        std::cerr.rdbuf(system_err.rdbuf());
    }
}