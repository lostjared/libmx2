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

    // Axis enum for deformation operations
    enum class DeformAxis { X = 0, Y = 1, Z = 2 };

    class Mesh {
    public:
        std::vector<GLfloat> vert;
        std::vector<GLfloat> tex;
        std::vector<GLfloat> norm;
        std::vector<GLuint> indices;
        
        // Original vertices for morphing/resetting
        std::vector<GLfloat> originalVert;
        std::vector<GLfloat> originalNorm;
        bool hasOriginal = false;

        GLuint shape_type = 0;
        GLuint texture = 0;

        void compressIndices();
        void generateBuffers();
        void cleanup();
        void draw();
        void drawWithForcedTexture(gl::ShaderProgram &shader, GLuint texture, const std::string texture_name);
        void setShapeType(GLuint type);
        void bindTexture(gl::ShaderProgram &shader, const std::string texture_name);
        
        // Deformation functions
        void saveOriginal();                                          // Save current state as original
        void resetToOriginal();                                       // Reset to original state
        void updateBuffers();                                         // Update GPU buffers after deformation
        void recalculateNormals();                                    // Recalculate normals after deformation
        
        // Scale operations
        void scale(float factor);                                     // Uniform scale
        void scale(float sx, float sy, float sz);                     // Non-uniform scale
        
        // Bend operation - bend mesh along an axis
        void bend(DeformAxis axis, float angle, float center = 0.0f, float range = 1.0f);
        
        // Twist operation - twist/rotate vertices around an axis based on position
        void twist(DeformAxis axis, float angle, float center = 0.0f);
        
        // Wave/ripple deformation
        void wave(DeformAxis axis, float amplitude, float frequency, float phase = 0.0f);
        
        // Spherical wave (ripple from center)
        void ripple(float amplitude, float frequency, float phase = 0.0f, float cx = 0.0f, float cy = 0.0f, float cz = 0.0f);
        
        // Noise/displacement
        void noise(float amplitude, unsigned int seed = 12345);
        
        // Taper - scale vertices based on position along an axis
        void taper(DeformAxis axis, float factor, float center = 0.0f);
        
        // Bulge/pinch - spherical scaling based on distance from center
        void bulge(float factor, float cx = 0.0f, float cy = 0.0f, float cz = 0.0f, float radius = 1.0f);
        
        // Morph between original and current deformed state
        void morph(float t);  // t=0 is original, t=1 is current deformed
        
        // Translate all vertices
        void translate(float tx, float ty, float tz);
        
        // Rotate around an axis (angle in radians)
        void rotate(DeformAxis axis, float angle);
        
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
       
       // Model-level deformation functions (apply to all meshes)
       void saveOriginal();
       void resetToOriginal();
       void updateBuffers();
       void recalculateNormals();
       
       void scale(float factor);
       void scale(float sx, float sy, float sz);
       void bend(DeformAxis axis, float angle, float center = 0.0f, float range = 1.0f);
       void twist(DeformAxis axis, float angle, float center = 0.0f);
       void wave(DeformAxis axis, float amplitude, float frequency, float phase = 0.0f);
       void ripple(float amplitude, float frequency, float phase = 0.0f, float cx = 0.0f, float cy = 0.0f, float cz = 0.0f);
       void noise(float amplitude, unsigned int seed = 12345);
       void taper(DeformAxis axis, float factor, float center = 0.0f);
       void bulge(float factor, float cx = 0.0f, float cy = 0.0f, float cz = 0.0f, float radius = 1.0f);
       void morph(float t);
       void translate(float tx, float ty, float tz);
       void rotate(DeformAxis axis, float angle);
       
   private:
       void parseLine(const std::string &line, Mesh &currentMesh, int &type, size_t &count);
       gl::ShaderProgram *program = nullptr;
       std::string tex_name;
   };

}
#endif
#endif