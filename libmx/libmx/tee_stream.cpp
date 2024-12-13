#include"tee_stream.hpp"

namespace mx {
    static std::ofstream log_file("system.log.txt", std::ios::out);
    static std::ofstream error_file("error.log.txt", std::ios::out);
    TeeStream system_out(std::cout, log_file);
    TeeStream system_err(std::cerr, error_file);
    
     void redirect() {
        if (!log_file.is_open()) {
            throw std::runtime_error("Failed to open system.log.txt");
        }
        if (!error_file.is_open()) {
            throw std::runtime_error("Failed to open error.log.txt");
        }

        std::cout.rdbuf(system_out.rdbuf());
        std::cerr.rdbuf(system_err.rdbuf());
    }
}