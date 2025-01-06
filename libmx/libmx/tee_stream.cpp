#include"tee_stream.hpp"

namespace mx {
    static std::ofstream log_file("system.log.txt", std::ios::out);
    static std::ofstream error_file("error.log.txt", std::ios::out);
    TeeStream system_out(std::cout, log_file);
    TeeStream system_err(std::cerr, error_file);
    
     void redirect() {
        if (!log_file.is_open()) {
            log_file.open("/tmp/system.log.txt", std::ios::out);
            if(!log_file.is_open()) {
                std::cerr << "Error creating log file.\n";
            }
        }
        if (!error_file.is_open()) {
            error_file.open("/tmp/error.log.txt");
            if(!error_file.is_open())  {
                std::cerr << "Error creating error log.\n";
            }
        }

        std::cout.rdbuf(system_out.rdbuf());
        std::cerr.rdbuf(system_err.rdbuf());
    }
}