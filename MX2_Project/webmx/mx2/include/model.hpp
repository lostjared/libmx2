/**
 * @file model.hpp
 * @brief OpenGL 3-D mesh and model classes with deformation support.
 *
 * Provides Mesh (a single sub-mesh with its own buffers) and Model (a
 * collection of meshes loaded from a Wavefront OBJ file).  Both classes
 * offer a rich set of CPU-side geometry deformation operations (scale,
 * bend, twist, wave, noise, etc.) as well as texture-map management
 * (normal, parallax, AO, displacement maps).
 *
 * Requires the WITH_GL compile-time flag.
 */
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

    /** @brief Axis selector for deformation operations. */
    enum class DeformAxis { X = 0, Y = 1, Z = 2 };

/**
 * @class Mesh
 * @brief A single OpenGL sub-mesh: geometry buffers, texture bindings, and deformers.
 *
 * Stores vertex, normal, UV, tangent, and bitangent arrays together with
 * an optional index list.  All geometry lives in CPU memory and must be
 * synchronised to the GPU via generateBuffers() / updateBuffers().  A
 * snapshot of the original geometry can be preserved with saveOriginal()
 * and restored with resetToOriginal().
 */
    class Mesh {
    public:
        std::vector<GLfloat> vert;       ///< Vertex position data (x,y,z triples).
        std::vector<GLfloat> tex;        ///< Texture coordinate data (u,v pairs).
        std::vector<GLfloat> norm;       ///< Normal vector data (x,y,z triples).
        std::vector<GLfloat> tangent;    ///< Tangent vector data.
        std::vector<GLfloat> bitangent;  ///< Bitangent vector data.
        std::vector<GLuint> indices;     ///< Optional index list for indexed drawing.
        std::vector<GLfloat> originalVert; ///< Saved copy of vertex positions.
        std::vector<GLfloat> originalNorm; ///< Saved copy of normals.
        
        bool hasOriginal;       ///< Whether an original snapshot exists.
        GLuint shape_type;      ///< GL primitive type (e.g. GL_TRIANGLES).
        GLuint texture;         ///< Primary diffuse texture ID.

        GLuint EBO, VAO, positionVBO, normalVBO, texCoordVBO, tangentVBO, bitangentVBO;
        
        GLuint normalMapTexture;       ///< Normal map texture ID.
        GLuint parallaxMapTexture;     ///< Parallax/height map texture ID.
        GLuint aoMapTexture;           ///< Ambient occlusion map texture ID.
        GLuint displacementMapTexture; ///< Displacement map texture ID.

        size_t vertIndex;  ///< Current write position in the vertex array.
        size_t texIndex;   ///< Current write position in the UV array.
        size_t normIndex;  ///< Current write position in the normal array.


        /** @brief Default constructor — zeroes all buffer IDs. */
        Mesh(); 
        /** @brief Destructor — calls cleanup(). */
        ~Mesh();
        Mesh(const Mesh &m) = delete;
        Mesh &operator=(const Mesh &m) = delete;
        /** @brief Move constructor. */
        Mesh(Mesh &&m) noexcept;
        /** @brief Move-assign operator. */
        Mesh &operator=(Mesh &&m) noexcept;

        /** @brief Build an optimised index list from the flat vertex arrays. */
        void compressIndices();

        /** @brief Upload vertex data to the GPU (must be called after populating arrays). */
        void generateBuffers();

        /** @brief Delete all GL objects (VAO, VBOs, textures). */
        void cleanup();

        /** @brief Issue a GL draw call using the stored VAO/index count. */
        void draw();

        /**
         * @brief Draw with a texture override without changing the mesh's stored texture.
         * @param shader       Bound shader program.
         * @param texture      Temporary GL texture to use.
         * @param texture_name Uniform sampler name in the shader.
         */
        void drawWithForcedTexture(gl::ShaderProgram &shader, GLuint texture, const std::string texture_name);

        /**
         * @brief Set the GL primitive drawing type.
         * @param type GL_TRIANGLES, GL_LINES, etc.
         */
        void setShapeType(GLuint type);

        /**
         * @brief Bind the mesh's primary texture to a shader sampler.
         * @param shader       Bound shader program.
         * @param texture_name Uniform sampler name.
         */
        void bindTexture(gl::ShaderProgram &shader, const std::string texture_name);

        /**
         * @brief Bind a cubemap texture to a shader sampler.
         * @param shader       Bound shader program.
         * @param cubemap      GL cubemap texture ID.
         * @param texture_name Uniform sampler name.
         */
        void bindCubemapTexture(gl::ShaderProgram &shader, GLuint cubemap, const std::string texture_name);

        /** @brief Take a snapshot of the current vertex and normal arrays. */
        void saveOriginal();

        /** @brief Restore vertex and normal arrays from the last saveOriginal() snapshot. */
        void resetToOriginal();

        /** @brief Synchronise CPU-side vertex data to the GPU buffers. */
        void updateBuffers();

        /** @brief Recompute per-vertex normals from the current vertex positions. */
        void recalculateNormals();

        /** @param normalMapTexture GL texture ID to use as a normal map. */
        void setNormalMap(GLuint normalMapTexture);
        /** @param heightMapTexture GL texture ID to use as a parallax/height map. */
        void setParallaxMap(GLuint heightMapTexture);
        /** @param displacementMapTexture GL texture ID to use as a displacement map. */
        void setDisplacementMap(GLuint displacementMapTexture);
        /** @param aoMapTexture GL texture ID to use as an ambient-occlusion map. */
        void setAmbientOcclusionMap(GLuint aoMapTexture);

        /** @brief Compute tangent and bitangent vectors for normal mapping. */
        void generateTangentBitangent();

        /**
         * @brief Scroll UV coordinates by an offset.
         * @param uOffset U-axis scroll amount.
         * @param vOffset V-axis scroll amount.
         */
        void uvScroll(float uOffset, float vOffset);

        /**
         * @brief Scale UV coordinates.
         * @param uScale U-axis scale factor.
         * @param vScale V-axis scale factor.
         */
        void uvScale(float uScale, float vScale);

        /**
         * @brief Rotate UV coordinates around the origin.
         * @param angle Rotation angle in degrees.
         */
        void uvRotate(float angle);

        /**
         * @brief Apply a chromatic aberration effect to vertex colours.
         * @param intensity Aberration magnitude.
         */
        void applyChromaticAberration(float intensity);

        /**
         * @brief Shift vertex colours by a multiplied colour vector.
         * @param colorShift RGB colour shift (each channel 0–1 or beyond).
         */
        void applyColorGrading(const glm::vec3 &colorShift);

        /**
         * @brief Apply hue/saturation/value adjustment to vertex colours.
         * @param hueShift   Hue rotation in degrees.
         * @param saturation Saturation scale (1.0 = unchanged).
         * @param value      Value scale (1.0 = unchanged).
         */
        void applyHSV(float hueShift, float saturation, float value);

        /**
         * @brief Blend between two cubemap textures in the shader.
         * @param cubemap1    First cubemap GL ID.
         * @param cubemap2    Second cubemap GL ID.
         * @param blendFactor Interpolation factor (0 = fully cubemap1, 1 = fully cubemap2).
         */
        void setCubemapBlending(GLuint cubemap1, GLuint cubemap2, float blendFactor);

        /** @return Stored normal map texture ID. */
        GLuint getNormalMap() const { return normalMapTexture; }
        /** @return Stored parallax map texture ID. */
        GLuint getParallaxMap() const { return parallaxMapTexture; }
        /** @return Stored ambient-occlusion map texture ID. */
        GLuint getAmbientOcclusionMap() const { return aoMapTexture; }

        /**
         * @brief Uniformly scale all vertices.
         * @param factor Scale multiplier.
         */
        void scale(float factor);

        /**
         * @brief Non-uniformly scale vertices along each axis.
         * @param sx X scale.
         * @param sy Y scale.
         * @param sz Z scale.
         */
        void scale(float sx, float sy, float sz);

        /**
         * @brief Bend the mesh around an axis.
         * @param axis   Axis to bend around.
         * @param angle  Bend angle in degrees.
         * @param center Centre point of the bend region.
         * @param range  Radial range of influence.
         */
        void bend(DeformAxis axis, float angle, float center = 0.0f, float range = 1.0f);

        /**
         * @brief Twist vertices around an axis.
         * @param axis   Twist axis.
         * @param angle  Maximum twist angle in degrees.
         * @param center Centre of the twist.
         */
        void twist(DeformAxis axis, float angle, float center = 0.0f);

        /**
         * @brief Apply a sinusoidal wave deformation.
         * @param axis      Axis along which the wave travels.
         * @param amplitude Wave amplitude.
         * @param frequency Spatial frequency.
         * @param phase     Phase offset in radians.
         */
        void wave(DeformAxis axis, float amplitude, float frequency, float phase = 0.0f);

        /**
         * @brief Apply a radial ripple deformation centred on a point.
         * @param amplitude Wave amplitude.
         * @param frequency Spatial frequency.
         * @param phase     Phase offset.
         * @param cx,cy,cz  Centre of the ripple.
         */
        void ripple(float amplitude, float frequency, float phase = 0.0f, float cx = 0.0f, float cy = 0.0f, float cz = 0.0f);

        /**
         * @brief Add random noise to vertex positions.
         * @param amplitude Noise magnitude.
         * @param seed      RNG seed for reproducibility.
         */
        void noise(float amplitude, unsigned int seed = 12345);

        /**
         * @brief Taper (scale cross-section) along an axis.
         * @param axis   Taper axis.
         * @param factor Taper rate.
         * @param center Centre position.
         */
        void taper(DeformAxis axis, float factor, float center = 0.0f);

        /**
         * @brief Bulge vertices outward from a sphere centre.
         * @param factor Bulge strength.
         * @param cx,cy,cz Bulge centre position.
         * @param radius  Sphere radius of influence.
         */
        void bulge(float factor, float cx = 0.0f, float cy = 0.0f, float cz = 0.0f, float radius = 1.0f);

        /**
         * @brief Interpolate (morph) vertices between original and current.
         * @param t Interpolation factor (0 = original, 1 = current).
         */
        void morph(float t);

        /**
         * @brief Translate all vertices by an offset.
         * @param tx,ty,tz Translation vector.
         */
        void translate(float tx, float ty, float tz);

        /**
         * @brief Rotate all vertices around an axis.
         * @param axis  Rotation axis.
         * @param angle Angle in degrees.
         */
        void rotate(DeformAxis axis, float angle);
    };

/**
 * @class Model
 * @brief A collection of Mesh objects loaded from an OBJ file.
 *
 * Model is the top-level scene node.  It opens an OBJ file (optionally
 * with index compression), provides shader and texture bindings, and
 * proxies every deformation and map operation to all contained meshes.
 */
    class Model {
    public:
        std::vector<Mesh> meshes; ///< All sub-meshes in load order.

        /** @brief Default constructor. */
        Model() = default;
        /** @brief Destructor — calls cleanup on all meshes. */
        ~Model() = default;

        /**
         * @brief Load an OBJ file synchronously.
         * @param filename Path to the .obj file.
         */
        Model(const std::string &filename);
        Model(const Model &m) = delete;
        Model &operator=(const Model &m) = delete;
        /** @brief Move constructor. */
        Model(Model &&m) noexcept;
        /** @brief Move-assign operator. */
        Model &operator=(Model &&m) noexcept;

        /**
         * @brief Print mesh statistics to a stream.
         * @param m               Mesh to inspect.
         * @param meshIndex       Zero-based mesh number.
         * @param didCompress     Whether compression was performed.
         * @param compressRequested Whether compression was requested.
         */
        void dumpMeshStats(const Mesh &m, size_t meshIndex, bool didCompress, bool compressRequested);

        /**
         * @brief Load a pre-compressed binary model format.
         * @param filename Path to the compressed model file.
         * @return @c true on success.
         */
        bool openCompressedModel(const std::string &filename);

        /**
         * @brief Parse an OBJ model from an in-memory string.
         * @param filename Path (used only for error messages).
         * @param text     OBJ content as a string.
         * @param compress Apply index compression if @c true.
         * @return @c true on success.
         */
        bool openModelString(const std::string &filename, const std::string &text, bool compress = true);

        /**
         * @brief Load an OBJ file from disk.
         * @param filename Path to the .obj file.
         * @param compress Apply index compression if @c true.
         * @return @c true on success.
         */
        bool openModel(const std::string &filename, bool compress = true);

        /** @brief Draw all meshes using stored bindings. */
        void drawArrays();

        /**
         * @brief Draw all meshes with a texture override.
         * @param texture      GL texture ID.
         * @param texture_name Uniform sampler name.
         */
        void drawArraysWithTexture(GLuint texture, const std::string texture_name);

        /**
         * @brief Draw all meshes with a cubemap.
         * @param cubemap      GL cubemap ID.
         * @param texture_name Uniform sampler name.
         */
        void drawArraysWithCubemap(GLuint cubemap, const std::string texture_name);

        /**
         * @brief Print per-mesh vertex/index statistics.
         * @param out Output stream.
         */
        void printData(std::ostream &out);

        /**
         * @brief Assign a shader program to be used for all draw calls.
         * @param shader_program Pointer to a compiled ShaderProgram.
         * @param texture_name   Sampler uniform name (default "texture1").
         */
        void setShaderProgram(gl::ShaderProgram *shader_program, const std::string texture_name = "texture1");

        /**
         * @brief Assign a pre-loaded list of GL texture IDs to meshes in order.
         * @param textures Vector of GL texture IDs.
         */
        void setTextures(const std::vector<GLuint> &textures);

        /**
         * @brief Load and assign textures from a directory using a file prefix.
         * @param win      Owner window (for asset path).
         * @param filename Base filename.
         * @param prefix   Directory prefix.
         */
        void setTextures(gl::GLWindow *win, const std::string &filename, std::string prefix);

        /** @brief Take a snapshot of all mesh geometries. */
        void saveOriginal();
        /** @brief Restore all meshes from the last saveOriginal() snapshot. */
        void resetToOriginal();
        /** @brief Push all mesh geometry changes to the GPU. */
        void updateBuffers();
        /** @brief Recompute normals for all meshes. */
        void recalculateNormals();

        /** @brief Uniformly scale all meshes. @param factor Scale multiplier. */
        void scale(float factor);
        /** @brief Non-uniformly scale all meshes. */
        void scale(float sx, float sy, float sz);
        /** @brief Bend all meshes. @see Mesh::bend */
        void bend(DeformAxis axis, float angle, float center = 0.0f, float range = 1.0f);
        /** @brief Twist all meshes. @see Mesh::twist */
        void twist(DeformAxis axis, float angle, float center = 0.0f);
        /** @brief Wave-deform all meshes. @see Mesh::wave */
        void wave(DeformAxis axis, float amplitude, float frequency, float phase = 0.0f);
        /** @brief Ripple-deform all meshes. @see Mesh::ripple */
        void ripple(float amplitude, float frequency, float phase = 0.0f, float cx = 0.0f, float cy = 0.0f, float cz = 0.0f);
        /** @brief Add noise to all meshes. @see Mesh::noise */
        void noise(float amplitude, unsigned int seed = 12345);
        /** @brief Taper all meshes. @see Mesh::taper */
        void taper(DeformAxis axis, float factor, float center = 0.0f);
        /** @brief Bulge all meshes. @see Mesh::bulge */
        void bulge(float factor, float cx = 0.0f, float cy = 0.0f, float cz = 0.0f, float radius = 1.0f);
        /** @brief Morph all meshes. @see Mesh::morph */
        void morph(float t);
        /** @brief Translate all meshes. */
        void translate(float tx, float ty, float tz);
        /** @brief Rotate all meshes. */
        void rotate(DeformAxis axis, float angle);

        /** @brief Set the normal map for all meshes. */
        void setNormalMap(GLuint normalMapTexture);
        /** @brief Set the parallax map for all meshes. */
        void setParallaxMap(GLuint heightMapTexture);
        /** @brief Set the displacement map for all meshes. */
        void setDisplacementMap(GLuint displacementMapTexture);
        /** @brief Set the ambient-occlusion map for all meshes. */
        void setAmbientOcclusionMap(GLuint aoMapTexture);
        /** @brief Generate tangent/bitangent for all meshes. */
        void generateTangentBitangent();
        /** @brief Scroll UVs for all meshes. */
        void uvScroll(float uOffset, float vOffset);
        /** @brief Scale UVs for all meshes. */
        void uvScale(float uScale, float vScale);
        /** @brief Rotate UVs for all meshes. */
        void uvRotate(float angle);
        /** @brief Apply chromatic aberration to all meshes. */
        void applyChromaticAberration(float intensity);
        /** @brief Apply colour grading to all meshes. */
        void applyColorGrading(const glm::vec3 &colorShift);
        /** @brief Apply HSV adjustment to all meshes. */
        void applyHSV(float hueShift, float saturation, float value);
        /** @brief Set cubemap blending for all meshes. */
        void setCubemapBlending(GLuint cubemap1, GLuint cubemap2, float blendFactor);

    private:
        /** @brief Parse a single MXMOD line and accumulate into currentMesh. */
        void parseLine(const std::string &line, Mesh &currentMesh, int &type, size_t &count);
        /** @brief Load a Wavefront OBJ file into meshes. */
        bool openOBJ(const std::string &filename, bool compress);
        gl::ShaderProgram *program = nullptr; ///< Active shader program.
        std::string tex_name;                 ///< Sampler uniform name.
    };
}

#endif
#endif