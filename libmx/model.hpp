#ifndef __MODEL__H__
#define __MODEL__H__
#ifdef __EMSCRIPTEN__
#include"config.hpp"
#else
#include"config.h"
#endif
#ifdef WITH_GL

#include"mx.hpp"
#include"gl.hpp"
#include"util.hpp"
#include<vector>
#include<string>

namespace mx {

    class Mesh {
    public:
        std::vector<GLfloat> vert;
        std::vector<GLfloat> tex;
        std::vector<GLfloat> norm;
        std::vector<GLuint> indices;

        GLuint shape_type = 0;
        GLuint texture = 0;

        void compressIndices();
        void generateBuffers();
        void cleanup();
        void draw();
        void drawWithForcedTexture(gl::ShaderProgram &shader, GLuint texture, const std::string texture_name);
        void setShapeType(GLuint type);
        void bindTexture(gl::ShaderProgram &shader, const std::string texture_name);
        GLuint EBO = 0, VAO = 0, positionVBO = 0, normalVBO = 0, texCoordVBO = 0;
        size_t vertIndex = 0;
        size_t texIndex = 0;
        size_t normIndex = 0;
        Mesh()
               : shape_type(GL_TRIANGLES),texture{0},
                 EBO(0), VAO(0), positionVBO(0), normalVBO(0), texCoordVBO(0),
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
       bool openCompressedModel(const std::string &filename);
       bool openModelString(const std::string &filename, const std::string &text, bool compress = true);
       bool openModel(const std::string &filename, bool compress = true);
       void drawArrays();
       void drawArraysWithTexture(GLuint texture, const std::string texture_name);
       void printData(std::ostream &out);
       void setShaderProgram(gl::ShaderProgram *shader_program, const std::string texture_name = "texture1");
       void setTextures(const std::vector<GLuint> &textures); 
       void setTextures(gl::GLWindow *win, const std::string &filename, std::string prefix);
   private:
       void parseLine(const std::string &line, Mesh &currentMesh, int &type, size_t &count);
       gl::ShaderProgram *program = nullptr;
       std::string tex_name;
   };

}
#endif
#endif