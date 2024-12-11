#ifndef __MODEL__H__
#define __MODEL__H__

#include"mx.hpp"
#include"gl.hpp"
#include<vector>
#include<string>

namespace mx {

    class Mesh {
    public:
        std::vector<GLfloat> vert;
        std::vector<GLfloat> tex;
        std::vector<GLfloat> norm;
        GLuint shape_type;
        GLuint texture;
        void generateBuffers();
        void cleanup();
        void draw();
        void setShapeType(GLuint type);
        void bindTexture(gl::ShaderProgram &shader, const std::string texture_name);
        GLuint VAO, positionVBO, normalVBO, texCoordVBO;
        size_t vertIndex;
        size_t texIndex;
        size_t normIndex;
        Mesh()
               : shape_type(GL_TRIANGLES),texture{0},
                 VAO(0), positionVBO(0), normalVBO(0), texCoordVBO(0),
                 vertIndex(0), texIndex(0), normIndex(0) {}
        ~Mesh();
        Mesh(const Mesh &m) = delete;
        Mesh &operator=(const Mesh &m) = delete;
        Mesh(Mesh &&m);
        Mesh &operator=(Mesh &&m) noexcept;
    };

   class Model {
   public:
       std::vector<Mesh> meshes;
       Model() = default;
       Model(const std::string &filename);
       Model(const Model &m) = delete;
       Model(Model &&m);
       Model &operator=(const Model &m) = delete;
       Model &operator=(Model &&m);
       bool openModel(const std::string &filename);
       void drawArrays();
       void printData();
       void setShaderProgram(gl::ShaderProgram *shader_program, const std::string texture_name = "texture1");
       void setTextures(const std::vector<GLuint> &textures); 

   private:
       void parseLine(const std::string &line, Mesh &currentMesh, int &type, size_t &count);
       gl::ShaderProgram *program = nullptr;
       std::string tex_name;
   };

}
#endif
