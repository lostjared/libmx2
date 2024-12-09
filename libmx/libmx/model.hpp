#ifndef __MODEL__H__
#define __MODEL__H__

#include"mx.hpp"
#include"gl.hpp"
#include<vector>
#include<string>

namespace mx {

    class Model {
    public:
        std::vector<GLfloat> vert;
        std::vector<GLfloat> tex;
        std::vector<GLfloat> norm;

        Model() = default;
        Model(const std::string &filename);

        bool openModel(const std::string &filename);
    private:
        void procLine(int type, const std::string &line);
    };
}
#endif