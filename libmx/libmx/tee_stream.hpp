#ifndef MX_TSTREAM_H
#define MX_TSTREAM_H

#include <iostream>
#include <fstream>
#include <streambuf>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace mx {

    class TeeBuf : public std::streambuf {
    public:
        TeeBuf(std::streambuf* buf1, std::streambuf* buf2) : buf1(buf1), buf2(buf2) {}
    protected:
        virtual int overflow(int c) override {
            if (c == EOF) {
                return !EOF;
            } else {
                if (buf1->sputc(c) == EOF || buf2->sputc(c) == EOF) {
                    return EOF;
                }
                return c;
            }
        }
        virtual int sync() override {
            if (buf1->pubsync() == 0 && buf2->pubsync() == 0) {
                return 0;
            }
            return -1;
        }
    private:
        std::streambuf* buf1 = nullptr;
        std::streambuf* buf2 = nullptr;
    };

    class TeeStream : public std::ostream {
    public:
        TeeStream(std::ostream& stream1, std::ostream& stream2)
            : std::ostream(&tbuf), tbuf(stream1.rdbuf(), stream2.rdbuf()) {}
    private:
        TeeBuf tbuf;
    };

    inline std::ofstream log_file("system.log.txt", std::ios::out);
    inline std::ofstream error_file("error.log.txt", std::ios::out);
    inline TeeStream system_out(std::cout, log_file);
    inline TeeStream system_err(std::cerr, error_file);

    inline void redirect() {
        static TeeStream out_stream(std::cout, log_file);
        static TeeStream err_stream(std::cerr, error_file);
        std::cout.rdbuf(out_stream.rdbuf());
        std::cerr.rdbuf(err_stream.rdbuf());
    }
} 

#endif 
