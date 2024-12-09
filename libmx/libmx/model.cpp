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

        std::fstream file;
        file.open(filename, std::ios::in);
        if(!file.is_open())
            return false;

        int type = 0;

        while(!file.eof()) {
            std::string line;
            std::getline(file, line);
            if(file && !line.empty()) {
                if(line == "vert") {
                    type = 0;
                    continue;
                } else if(line == "tex") {
                    type = 1;
                    continue;
                } else if(line == "norm") {
                    type = 2;
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
        switch(type) {
            case 0: {
                float x = 0, y = 0, z = 0;
                stream >> x >> y >> z;
                vert.push_back(x);
                vert.push_back(y);
                vert.push_back(z);
            }
            break;
            case 1: {
                float u = 0, v = 0;
                stream >> u >> v;
                tex.push_back(u);
                tex.push_back(v);
            }
            break;
            case 2: {
                float x = 0, y = 0, z = 0;
                stream >> x >> y >> z;
                norm.push_back(x);
                norm.push_back(y);
                norm.push_back(z); 
            }
            break;
        }
    }
}