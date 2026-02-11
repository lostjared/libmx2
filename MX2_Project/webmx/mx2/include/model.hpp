#ifndef __MODEL__H__
#define __MODEL__H__

#ifdef __EMSCRIPTEN__
#include "config.hpp"
#else
#include "config.h"
#endif

#ifdef WITH_GL

#include "mx.hpp"
#include "gl.hpp"
#include "util.hpp"
#include <vector>
#include <string>

#ifdef __EMSCRIPTEN__
#include "glm.hpp"
#else
#include <glm/glm.hpp>
#endif

namespace mx {
    enum class DeformAxis { X = 0, Y = 1, Z = 2 };

    class Mesh {
    public:
        std::vector<GLfloat> vert;
        std::vector<GLfloat> tex;
        std::vector<GLfloat> norm;
        std::vector<GLfloat> tangent;
        std::vector<GLfloat> bitangent;
        std::vector<GLuint> indices;
        std::vector<GLfloat> originalVert;
        std::vector<GLfloat> originalNorm;
        
        bool hasOriginal;
        GLuint shape_type;
        GLuint texture;

        GLuint EBO, VAO, positionVBO, normalVBO, texCoordVBO, tangentVBO, bitangentVBO;
        
        GLuint normalMapTexture, parallaxMapTexture, aoMapTexture, displacementMapTexture;

        size_t vertIndex;
        size_t texIndex;
        size_t normIndex;


        Mesh(); 
        ~Mesh();
        Mesh(const Mesh &m) = delete;
        Mesh &operator=(const Mesh &m) = delete;
        Mesh(Mesh &&m) noexcept;
        Mesh &operator=(Mesh &&m) noexcept;

        void compressIndices();
        void generateBuffers();
        void cleanup();
        void draw();
        void drawWithForcedTexture(gl::ShaderProgram &shader, GLuint texture, const std::string texture_name);
        void setShapeType(GLuint type);
        void bindTexture(gl::ShaderProgram &shader, const std::string texture_name);
        void bindCubemapTexture(gl::ShaderProgram &shader, GLuint cubemap, const std::string texture_name);
        void saveOriginal();                                          
        void resetToOriginal();                                       
        void updateBuffers();                                         
        void recalculateNormals();
        
        void setNormalMap(GLuint normalMapTexture);
        void setParallaxMap(GLuint heightMapTexture);
        void setDisplacementMap(GLuint displacementMapTexture);
        void setAmbientOcclusionMap(GLuint aoMapTexture);
        void generateTangentBitangent();
        void uvScroll(float uOffset, float vOffset);
        void uvScale(float uScale, float vScale);
        void uvRotate(float angle);
        void applyChromaticAberration(float intensity);
        void applyColorGrading(const glm::vec3 &colorShift);
        void applyHSV(float hueShift, float saturation, float value);
        void setCubemapBlending(GLuint cubemap1, GLuint cubemap2, float blendFactor);
        
        GLuint getNormalMap() const { return normalMapTexture; }
        GLuint getParallaxMap() const { return parallaxMapTexture; }
        GLuint getAmbientOcclusionMap() const { return aoMapTexture; }
        
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
    };

    class Model {
    public:
        std::vector<Mesh> meshes;

        Model() = default;
        Model(const std::string &filename);
        Model(const Model &m) = delete;
        Model &operator=(const Model &m) = delete;
        Model(Model &&m) noexcept;
        Model &operator=(Model &&m) noexcept;
        void dumpMeshStats(const Mesh &m, size_t meshIndex, bool didCompress, bool compressRequested);
        bool openCompressedModel(const std::string &filename);
        bool openModelString(const std::string &filename, const std::string &text, bool compress = true);
        bool openModel(const std::string &filename, bool compress = true);
        
        void drawArrays();
        void drawArraysWithTexture(GLuint texture, const std::string texture_name);
        void drawArraysWithCubemap(GLuint cubemap, const std::string texture_name);
        void printData(std::ostream &out);
        void setShaderProgram(gl::ShaderProgram *shader_program, const std::string texture_name = "texture1");
        void setTextures(const std::vector<GLuint> &textures); 
        void setTextures(gl::GLWindow *win, const std::string &filename, std::string prefix);
        
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
        
        void setNormalMap(GLuint normalMapTexture);
        void setParallaxMap(GLuint heightMapTexture);
        void setDisplacementMap(GLuint displacementMapTexture);
        void setAmbientOcclusionMap(GLuint aoMapTexture);
        void generateTangentBitangent();
        void uvScroll(float uOffset, float vOffset);
        void uvScale(float uScale, float vScale);
        void uvRotate(float angle);
        void applyChromaticAberration(float intensity);
        void applyColorGrading(const glm::vec3 &colorShift);
        void applyHSV(float hueShift, float saturation, float value);
        void setCubemapBlending(GLuint cubemap1, GLuint cubemap2, float blendFactor);

    private:
        void parseLine(const std::string &line, Mesh &currentMesh, int &type, size_t &count);
        gl::ShaderProgram *program = nullptr;
        std::string tex_name;
    };
}

#endif
#endif