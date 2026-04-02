#include "argz.hpp"
#include <zlib.h>
#include <memory>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>

namespace cmp {

    /**
     * @brief Compresses raw binary data using zlib.
     * * @param data Pointer to the input data.
     * @param sourceLen Length of the input data.
     * @param destLen Output parameter that will store the compressed size.
     * @return std::unique_ptr<char[]> Pointer to the compressed data buffer.
     */
    std::unique_ptr<char[]> compressData(const char* data, uLong sourceLen, uLong &destLen) {
        destLen = compressBound(sourceLen);
        std::unique_ptr<char[]> compressedData(new char[destLen]);

        int ret = ::compress(reinterpret_cast<Bytef*>(compressedData.get()), &destLen,
                             reinterpret_cast<const Bytef*>(data), sourceLen);
        if (ret != Z_OK) {
            destLen = 0;
            throw std::runtime_error("Compression failed with error code: " + std::to_string(ret));
        }
        
        return compressedData;
    }

    /**
     * @brief Decompresses zlib-compressed data.
     * * @param data Pointer to the compressed data.
     * @param size_ Size of the compressed data.
     * @return std::vector<char> The decompressed raw binary data.
     */
    std::vector<char> decompressData(const void *data, uLong size_) {
        z_stream strm = {};
        strm.avail_in = size_;
        strm.next_in = reinterpret_cast<Bytef*>(const_cast<void*>(data));

        if (inflateInit(&strm) != Z_OK) {
            throw std::runtime_error("inflateInit failed");
        }

        std::vector<char> outData;
        const size_t chunkSize = 16384;
        char outBuffer[chunkSize];
        int ret;

        do {
            strm.avail_out = chunkSize;
            strm.next_out = reinterpret_cast<Bytef*>(outBuffer);
            ret = inflate(&strm, Z_NO_FLUSH);
            
            if (ret != Z_OK && ret != Z_STREAM_END) {
                inflateEnd(&strm);
                throw std::runtime_error("inflate failed with error code: " + std::to_string(ret));
            }
            
            size_t have = chunkSize - strm.avail_out;
            outData.insert(outData.end(), outBuffer, outBuffer + have);
        } while (ret != Z_STREAM_END);

        inflateEnd(&strm);
        return outData;
    }

    /**
     * @brief Reads an entire file into a character vector.
     * * @param filename Path to the file.
     * @return std::vector<char> Buffer containing the file's contents.
     */
    std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + filename);
        }
        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        return buffer;
    }
}

int main(int argc, char **argv) {
    Arguments args; 
    Argz<std::string> parser(argc, argv);
    parser.addOptionSingleValue('c', "Model to compress")
        .addOptionSingleValue('o', "Output")
        .addOptionSingleValue('z', "Model to decompress")
        .addOptionSingle('h', "Help")
        .addOptionSingle('v', "Version");

    Argument<std::string> arg;
    std::string compress_file;
    std::string decomp_file;
    std::string output;
    int value = 0;

    try {
        while((value = parser.proc(arg)) != -1) {
            switch(value) {
                case 'h':
                case 'v':
                    parser.help(std::cout);
                    exit(EXIT_SUCCESS);
                    break;
                case 'c':
                    compress_file = arg.arg_value;
                    break;
                case 'z':
                    decomp_file = arg.arg_value;
                    break;
                case 'o':
                    output = arg.arg_value;
                    if(!compress_file.empty() && output.rfind(".mxmod.z") == std::string::npos) {
                        std::cerr << "Error model file must end in .mxmod.z to be recognized\n";
                        exit(EXIT_FAILURE);
                    }
                    break;
            }
        }
    } catch (const ArgException<std::string>& e) {
        std::cerr << "mx: Argument Exception" << e.text() << std::endl;
        std::cerr.flush();
        exit(EXIT_FAILURE);
    }

    try {
        if(compress_file.empty() && decomp_file.empty()) {
            std::cerr << "mx: Error no file to compress or decompress\nmx: Use -h for help\n";
            exit(EXIT_FAILURE);
        }
        if(!compress_file.empty() && !decomp_file.empty()) {
            std::cerr << "mx: Error cannot compress and decompress at the same time\n";
            exit(EXIT_FAILURE);
        }
        if(output.empty()) {
            std::cerr << "mx: Error no output file specified\n";
            exit(EXIT_FAILURE);
        }

        if(!compress_file.empty()) {
            std::vector<char> buffer_value = cmp::readFile(compress_file);
            uLong destLen = 0;
            
            auto compressed_data = cmp::compressData(buffer_value.data(), buffer_value.size(), destLen);
            
            std::fstream ofile(output, std::ios::out | std::ios::binary);
            if(!ofile.is_open()) {
                throw std::runtime_error("Error could not open file: " + output + " for writing");
            }
            ofile.write(compressed_data.get(), destLen);
        }

        if(!decomp_file.empty()) {
            std::vector<char> buffer_value = cmp::readFile(decomp_file);
            
            std::vector<char> decomp_data = cmp::decompressData(buffer_value.data(), buffer_value.size());
            
            std::fstream ofile(output, std::ios::out | std::ios::binary);
            if(!ofile.is_open()) {
                throw std::runtime_error("Error could not open file: " + output + " for writing");
            }
            ofile.write(decomp_data.data(), decomp_data.size());
        }
    } catch(const std::exception &e) {
        std::cerr << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}