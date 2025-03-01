#include"argz.hpp"
#include<zlib.h>
#include<memory>
#include<fstream>
#include<iostream>
#include<string>
#include<vector>

namespace cmp {
    std::unique_ptr<char[]> compressString(const std::string &text, uLong &len) {
        uLong sourceLen = text.size();
        uLong destLen = compressBound(sourceLen);
        std::unique_ptr<char[]> compressedData(new char[destLen]);
    
        int ret = ::compress(reinterpret_cast<Bytef*>(compressedData.get()), &destLen,
                             reinterpret_cast<const Bytef*>(text.data()), sourceLen);
        if (ret != Z_OK) {
            len = 0;
            throw std::runtime_error("Compression failed with error code: " + std::to_string(ret));
        }
        len = destLen;
        return compressedData;
    }

    std::string decompressString(void *data, uLong size_) {
        z_stream strm;
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        strm.opaque = Z_NULL;
        strm.avail_in = size_;
        strm.next_in = reinterpret_cast<Bytef*>(data);
        int ret = inflateInit(&strm);
        if (ret != Z_OK) {
            throw std::runtime_error("inflateInit failed");
        }
    
        std::string outStr;
        const size_t chunkSize = 16384;
        char outBuffer[chunkSize];
        do {
            strm.avail_out = chunkSize;
            strm.next_out = reinterpret_cast<Bytef*>(outBuffer);
            ret = inflate(&strm, Z_NO_FLUSH);
            if (ret != Z_OK && ret != Z_STREAM_END) {
                inflateEnd(&strm);
                throw std::runtime_error("inflate failed with error code: " + std::to_string(ret));
            }
            size_t have = chunkSize - strm.avail_out;
            outStr.append(outBuffer, have);
        } while (ret != Z_STREAM_END);
    
        inflateEnd(&strm);
        return outStr;
    }
    
    std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + filename);
        }
        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();
        return buffer;
    }
}

int main(int argc, char **argv) {
    Arguments args;
	Argz<std::string> parser(argc, argv);
    parser.addOptionSingleValue('c', "Model to compress")
        .addOptionSingleValue('o', "Output")
        .addOptionSingleValue('z', "Model to decompress");
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
                        std::cout << "Error model file must end in .mxmod.z to be recongized\n";
                        exit(EXIT_FAILURE);
                    }
                    break;
            }
        }
    } catch (const ArgException<std::string>& e) {
        std::cerr << "mx: Argument Exception" << e.text() << std::endl;
		std::cerr.flush();
        exit(0);
    }
    try {
        if(compress_file.empty() && decomp_file.empty()) {
            std::cerr << "mx: Error no file to compress or decompress\n";
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
        if(!compress_file.empty() && !output.empty()) {
            std::vector<char> buffer_value = cmp::readFile(compress_file);
            uLong size_val = buffer_value.size();   
            auto compressed_data = cmp::compressString(std::string(buffer_value.begin(), buffer_value.end()), size_val);
            std::fstream ofile;
            ofile.open(output, std::ios::out | std::ios::binary);
            if(!ofile.is_open()) {
                throw std::runtime_error("Error could not open file: " + output + " for writing");
            }
            ofile.write(reinterpret_cast<char *>(compressed_data.get()), size_val);
            ofile.close();
        }

        if(!decomp_file.empty()  && !output.empty()) {
            std::vector<char> buffer_value = cmp::readFile(decomp_file);
            std::string decomp_data = cmp::decompressString(buffer_value.data(), buffer_value.size());
            std::fstream ofile;
            ofile.open(output, std::ios::out | std::ios::binary);
            if(!ofile.is_open()) {
                throw std::runtime_error("Error could not open file: " + output + " for writing");
            }
            ofile.write(decomp_data.c_str(), decomp_data.size());
            ofile.close();
        }
    } catch(const std::exception &e) {
        std::cerr << e.what() << "\n";
    }
    return 0;
}