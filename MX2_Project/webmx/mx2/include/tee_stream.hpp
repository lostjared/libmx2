#ifndef MX_TSTREAM_H
#define MX_TSTREAM_H

#include <iostream>
#include <fstream>
#include <streambuf>
#include <stdexcept>
#include <memory>

namespace mx {

    class TeeBuf : public std::streambuf {
    public:
        TeeBuf(std::streambuf* buf1, std::streambuf* buf2) : buf1(buf1), buf2(buf2) {}

    protected:
        virtual int overflow(int c) override {
            if (buf1 == nullptr || buf2 == nullptr) {
                return traits_type::eof();
            }

            if (traits_type::eq_int_type(c, traits_type::eof())) {
                if (this->sync() == -1) {
                    return traits_type::eof();
                }
                return traits_type::not_eof(c);
            }

            const auto ch = traits_type::to_char_type(c);
            if (traits_type::eq_int_type(buf1->sputc(ch), traits_type::eof()) ||
                traits_type::eq_int_type(buf2->sputc(ch), traits_type::eof())) {
                return traits_type::eof();
            }

            return c;
        }

        virtual int sync() override {
            if (buf1 == nullptr || buf2 == nullptr) {
                return -1;
            }
            int res1 = buf1->pubsync();
            int res2 = buf2->pubsync();
            return (res1 == 0 && res2 == 0) ? 0 : -1;
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
    
    extern TeeStream system_out;
    extern TeeStream system_err;
    void redirect();
} 

#endif 
