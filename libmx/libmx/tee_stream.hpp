/**
 * @file tee_stream.hpp
 * @brief Tee stream utilities that duplicate output to two sinks simultaneously.
 *
 * Provides TeeBuf and TeeStream, which mirror every byte written to both
 * underlying stream buffers.  The global system_out and system_err streams
 * are pre-wired to stdout/stderr and a log file after redirect() is called.
 */
#ifndef MX_TSTREAM_H
#define MX_TSTREAM_H

#include <iostream>
#include <fstream>
#include <streambuf>
#include <stdexcept>
#include <memory>

namespace mx {

/**
 * @class TeeBuf
 * @brief A std::streambuf that writes every byte to two underlying buffers.
 *
 * Attach two std::streambuf pointers at construction.  Any character written
 * through this buffer is forwarded to both sinks; overflow() and sync() are
 * both propagated.
 */
    class TeeBuf : public std::streambuf {
    public:
        /**
         * @brief Construct a TeeBuf that mirrors output to buf1 and buf2.
         * @param buf1 First destination stream buffer (must outlive this object).
         * @param buf2 Second destination stream buffer (must outlive this object).
         */
        TeeBuf(std::streambuf* buf1, std::streambuf* buf2) : buf1(buf1), buf2(buf2) {}

    protected:
        /**
         * @brief Write a single character to both buffers.
         * @param c Character to write (may be EOF).
         * @return The character written on success, or traits_type::eof() on failure.
         */
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

        /**
         * @brief Flush both underlying buffers.
         * @return 0 if both flushes succeed, -1 otherwise.
         */
        virtual int sync() override {
            if (buf1 == nullptr || buf2 == nullptr) {
                return -1;
            }
            int res1 = buf1->pubsync();
            int res2 = buf2->pubsync();
            return (res1 == 0 && res2 == 0) ? 0 : -1;
        }

    private:
        std::streambuf* buf1 = nullptr; ///< First sink.
        std::streambuf* buf2 = nullptr; ///< Second sink.
    };

/**
 * @class TeeStream
 * @brief An std::ostream that writes to two output streams simultaneously.
 *
 * Wraps TeeBuf so all normal stream operators (<<, write, flush) are
 * mirrored to both stream1 and stream2.
 */
    class TeeStream : public std::ostream {
    public:
        /**
         * @brief Construct a TeeStream that writes to both stream1 and stream2.
         * @param stream1 First output stream.
         * @param stream2 Second output stream.
         */
        TeeStream(std::ostream& stream1, std::ostream& stream2)
            : std::ostream(&tbuf), tbuf(stream1.rdbuf(), stream2.rdbuf()) {}

    private:
        TeeBuf tbuf; ///< Internal tee buffer.
    };

    extern TeeStream system_out; ///< Global stdout + log tee stream.
    extern TeeStream system_err; ///< Global stderr + log tee stream.

    /**
     * @brief Redirect std::cout and std::cerr through the tee streams.
     *
     * After calling this function, all writes to std::cout and std::cerr
     * are forwarded to both the terminal and the configured log file.
     */
    void redirect();
}

#endif
