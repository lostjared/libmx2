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

        std::vector<GLfloat> vert_tex;
        std::vector<GLfloat> vert_tex_norm;

        GLuint shape_type = GL_TRIANGLES;

        Model() = default;
        Model(const std::string &filename);
        Model(const Model &m);
        Model(Model &&m);

        Model &operator=(const Model &m);
        Model &operator=(Model &&m);

        bool openModel(const std::string &filename);
        void generateBuffers(GLuint &VAO, GLuint &positionVBO, GLuint &normalVBO, GLuint &texCoordVBO);
        void buildVertTex();
        void buildVertTexNorm();
        void drawArrays(const GLuint shape);
        void drawArrays();
    private:
        void procLine(int type, const std::string &line);
        size_t vertIndex = 0, texIndex = 0, normIndex = 0;
        
    };
}
#endif