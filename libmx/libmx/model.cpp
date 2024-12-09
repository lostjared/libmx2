#include"model.hpp"
#include<sstream>
#include<fstream>

namespace mx {
    Model::Model(const std::string &filename) {
        if(!openModel(filename)) {
            std::ostringstream stream;
            stream << "Error could not open mode: " << filename << "\n";
            throw mx::Exception(stream.str());
        }
    }

    bool Model::openModel(const std::string &filename) {
        std::fstream file(filename, std::ios::in);
        if (!file.is_open()) {
            return false;
        }

        int type = -1;
        size_t count = 0;

        vertIndex = 0;
        texIndex = 0;
        normIndex = 0;

        while (!file.eof()) {
            std::string line;
            std::getline(file, line);
            if (file) {
                std::istringstream stream(line);
                if (line.rfind("vert", 0) == 0) {
                    type = 0;
                    stream.ignore(5); 
                    stream >> count;
                    vert.resize(count * 3); 
                    continue;
                } else if (line.rfind("tex", 0) == 0) {
                    type = 1;
                    stream.ignore(4); 
                    stream >> count;
                    tex.resize(count * 2); 
                    continue;
                } else if (line.rfind("norm", 0) == 0) {
                    type = 2;
                    stream.ignore(5); 
                    stream >> count;
                    norm.resize(count * 3);
                    continue;
                } else {
                    procLine(type, line);
                }
            }
        }

        file.close();
        return true;
    }


    void Model::procLine(int type, const std::string &line) {
        
        std::istringstream stream(line);
        switch (type) {
            case 0: { 
                stream >> vert[vertIndex] >> vert[vertIndex + 1] >> vert[vertIndex + 2];
                vertIndex += 3;
            } break;
            case 1: { 
                stream >> tex[texIndex] >> tex[texIndex + 1];
                texIndex += 2;
            } break;
            case 2: { 
                stream >> norm[normIndex] >> norm[normIndex + 1] >> norm[normIndex + 2];
                normIndex += 3;
            } break;
            default:
                throw Exception("Unknown data type");
        }
    }

    void Model::buildVertTex() {
        if (!vert.empty() && !tex.empty()) {
            if (vert.size() % 3 != 0 || tex.size() % 2 != 0) {
                throw Exception("Invalid vertex or texture data size");
            }
            size_t vertexCount = vert.size() / 3; 
            vert_tex.resize(vertexCount * 5);   
            for (size_t i = 0, x = 0, index = 0; i < vert.size() && x < tex.size(); i += 3, x += 2, index += 5) {
                vert_tex[index]     = vert[i];
                vert_tex[index + 1] = vert[i + 1];
                vert_tex[index + 2] = vert[i + 2];
                vert_tex[index + 3] = tex[x];
                vert_tex[index + 4] = tex[x + 1];
            }
        }
    }

    void Model::buildVertTexNorm() {
        if (!vert.empty() && !tex.empty() && !norm.empty()) {
            if (vert.size() % 3 != 0 || tex.size() % 2 != 0 || norm.size() % 3 != 0) {
                throw Exception("Invalid vertex, texture, or normal data size");
            }

            size_t vertexCount = vert.size() / 3;
            vert_tex_norm.resize(vertexCount * 8); 

            for (size_t i = 0, x = 0, index = 0; i < vert.size() && x < tex.size(); i += 3, x += 2, index += 8) {
                vert_tex_norm[index]     = vert[i];
                vert_tex_norm[index + 1] = vert[i + 1];
                vert_tex_norm[index + 2] = vert[i + 2];
                vert_tex_norm[index + 3] = tex[x];
                vert_tex_norm[index + 4] = tex[x + 1];
                vert_tex_norm[index + 5] = norm[i];
                vert_tex_norm[index + 6] = norm[i + 1];
                vert_tex_norm[index + 7] = norm[i + 2];
            }
        }
    }
}