#ifndef __TSTREAM_H__
#define __TSTREAM_H__

#include<iostream>
#include<fstream>
#include<streambuf>
#include<chrono>
#include<iomanip>
#include<sstream>

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
        std::streambuf* buf1 = 0;
        std::streambuf* buf2 = 0;
    };

    class TeeStream : public std::ostream {
    public:
        TeeStream(std::ostream& stream1, std::ostream& stream2)
            : std::ostream(&tbuf), tbuf(stream1.rdbuf(), stream2.rdbuf()) {}
    private:
        TeeBuf tbuf;
    };

    inline std::ofstream file("system.log.txt", std::ios::app);
    inline std::ofstream file_err("error.log.txt", std::ios::app);
    inline TeeStream system_out(mx::system_out, file);
    inline TeeStream system_err(mx::system_err, file_err);
}

#endif