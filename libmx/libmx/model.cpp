#ifdef __EMSCRIPTEN__
#include "config.hpp"
#else
#include "config.h"
#endif

#ifdef WITH_GL
#include "model.hpp"
#include <sstream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cstddef>
#include <cmath>
#include <random>
#include <algorithm>
#include <iostream>
#include <filesystem>

#ifdef __EMSCRIPTEN__
#include "glm.hpp"
#else
#include <glm/glm.hpp>
#endif

namespace mx {

    struct Vertex {
        float position[3];
        float normal[3];
        float texCoord[2];

        bool operator==(const Vertex& other) const {
            return position[0] == other.position[0] &&
                position[1] == other.position[1] &&
                position[2] == other.position[2] &&
                normal[0]   == other.normal[0] &&
                normal[1]   == other.normal[1] &&
                normal[2]   == other.normal[2] &&
                texCoord[0] == other.texCoord[0] &&
                texCoord[1] == other.texCoord[1];
        }
    };

    struct VertexHash {
        std::size_t operator()(const Vertex& v) const {
            std::size_t seed = 0;
            auto hash_combine = [&seed](float value) {
                std::hash<float> hasher;
                seed ^= hasher(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            };
            hash_combine(v.position[0]);
            hash_combine(v.position[1]);
            hash_combine(v.position[2]);
            hash_combine(v.normal[0]);
            hash_combine(v.normal[1]);
            hash_combine(v.normal[2]);
            hash_combine(v.texCoord[0]);
            hash_combine(v.texCoord[1]);
            return seed;
        }
    };

    Mesh::Mesh()
        : hasOriginal(false), shape_type(GL_TRIANGLES), texture(0),
          EBO(0), VAO(0), positionVBO(0), normalVBO(0), texCoordVBO(0),
          tangentVBO(0), bitangentVBO(0),
          normalMapTexture(0), parallaxMapTexture(0), aoMapTexture(0), displacementMapTexture(0),
          vertIndex(0), texIndex(0), normIndex(0) {}

        Mesh::Mesh(Mesh &&m) noexcept
        : vert(std::move(m.vert)),
        tex(std::move(m.tex)),
        norm(std::move(m.norm)),
        tangent(std::move(m.tangent)),
        bitangent(std::move(m.bitangent)),
        indices(std::move(m.indices)),
        originalVert(std::move(m.originalVert)),
        originalNorm(std::move(m.originalNorm)),
        hasOriginal(m.hasOriginal),
        shape_type(m.shape_type),
        texture(m.texture),
        EBO(m.EBO),
        VAO(m.VAO),
        positionVBO(m.positionVBO),
        normalVBO(m.normalVBO),
        texCoordVBO(m.texCoordVBO),
        tangentVBO(m.tangentVBO),
        bitangentVBO(m.bitangentVBO),
        normalMapTexture(m.normalMapTexture),
        parallaxMapTexture(m.parallaxMapTexture),
        aoMapTexture(m.aoMapTexture),
        displacementMapTexture(m.displacementMapTexture),
        vertIndex(m.vertIndex),
        texIndex(m.texIndex),
        normIndex(m.normIndex) {

        m.EBO = m.VAO = m.positionVBO = m.normalVBO = m.texCoordVBO = m.tangentVBO = m.bitangentVBO = 0;
        m.texture = 0;
        m.normalMapTexture = m.parallaxMapTexture = m.aoMapTexture = m.displacementMapTexture = 0;
        m.vertIndex = m.texIndex = m.normIndex = 0;
        m.shape_type = GL_TRIANGLES;
        m.hasOriginal = false;
    }

    Mesh &Mesh::operator=(Mesh &&m) noexcept {
        if (this != &m) {
            cleanup();
            vert = std::move(m.vert);
            tex = std::move(m.tex);
            norm = std::move(m.norm);
            tangent = std::move(m.tangent);
            bitangent = std::move(m.bitangent);
            indices = std::move(m.indices);
            originalVert = std::move(m.originalVert);
            originalNorm = std::move(m.originalNorm);
            hasOriginal = m.hasOriginal;
            shape_type = m.shape_type;
            texture = m.texture;
            VAO = m.VAO;
            positionVBO = m.positionVBO;
            normalVBO = m.normalVBO;
            texCoordVBO = m.texCoordVBO;
            tangentVBO = m.tangentVBO;
            bitangentVBO = m.bitangentVBO;
            EBO = m.EBO;
            normalMapTexture = m.normalMapTexture;
            parallaxMapTexture = m.parallaxMapTexture;
            aoMapTexture = m.aoMapTexture;
            displacementMapTexture = m.displacementMapTexture;
            vertIndex = m.vertIndex;
            texIndex = m.texIndex;
            normIndex = m.normIndex;

            m.VAO = m.positionVBO = m.normalVBO = m.texCoordVBO = m.tangentVBO = m.bitangentVBO = m.EBO = 0;
            m.texture = 0;
            m.normalMapTexture = m.parallaxMapTexture = m.aoMapTexture = m.displacementMapTexture = 0;
            m.vertIndex = m.texIndex = m.normIndex = 0;
            m.shape_type = GL_TRIANGLES;
            m.hasOriginal = false;
        }
        return *this;
    }

    Mesh::~Mesh() { cleanup(); }

    void Mesh::cleanup() {
        if (positionVBO) glDeleteBuffers(1, &positionVBO);
        if (normalVBO) glDeleteBuffers(1, &normalVBO);
        if (texCoordVBO) glDeleteBuffers(1, &texCoordVBO);
        if (tangentVBO) glDeleteBuffers(1, &tangentVBO);
        if (bitangentVBO) glDeleteBuffers(1, &bitangentVBO);
        if (EBO) glDeleteBuffers(1, &EBO);
        if (VAO) glDeleteVertexArrays(1, &VAO);
        positionVBO = normalVBO = texCoordVBO = tangentVBO = bitangentVBO = EBO = VAO = 0;
    }

    void Mesh::setShapeType(GLuint type) {
        switch (type) {
            case 0: shape_type = GL_TRIANGLES; break;
            case 1: shape_type = GL_TRIANGLE_STRIP; break;
            case 2: shape_type = GL_TRIANGLE_FAN; break;
            default: shape_type = GL_TRIANGLES; break;
        }
    }

    void Mesh::compressIndices() {
        size_t numVertices = vert.size() / 3;
        std::vector<Vertex> uniqueVertices;
        std::unordered_map<Vertex, GLuint, VertexHash> vertexToIndex;
        std::vector<GLuint> newIndices;
        newIndices.reserve(numVertices);

        for (size_t i = 0; i < numVertices; ++i) {
            Vertex vertex;
            vertex.position[0] = vert[i * 3 + 0];
            vertex.position[1] = vert[i * 3 + 1];
            vertex.position[2] = vert[i * 3 + 2];

            if (!norm.empty()) {
                vertex.normal[0] = norm[i * 3 + 0];
                vertex.normal[1] = norm[i * 3 + 1];
                vertex.normal[2] = norm[i * 3 + 2];
            } else {
                vertex.normal[0] = vertex.normal[1] = vertex.normal[2] = 0.0f;
            }

            if (!tex.empty()) {
                vertex.texCoord[0] = tex[i * 2 + 0];
                vertex.texCoord[1] = tex[i * 2 + 1];
            } else {
                vertex.texCoord[0] = vertex.texCoord[1] = 0.0f;
            }

            auto it = vertexToIndex.find(vertex);
            if (it != vertexToIndex.end()) {
                newIndices.push_back(it->second);
            } else {
                GLuint newIndex = (GLuint)uniqueVertices.size();
                uniqueVertices.push_back(vertex);
                vertexToIndex[vertex] = newIndex;
                newIndices.push_back(newIndex);
            }
        }

        std::vector<GLfloat> newVerts, newNorms, newTex;
        newVerts.reserve(uniqueVertices.size() * 3);
        if (!norm.empty()) newNorms.reserve(uniqueVertices.size() * 3);
        if (!tex.empty()) newTex.reserve(uniqueVertices.size() * 2);

        for (const auto &v : uniqueVertices) {
            newVerts.push_back(v.position[0]);
            newVerts.push_back(v.position[1]);
            newVerts.push_back(v.position[2]);

            if (!norm.empty()) {
                newNorms.push_back(v.normal[0]);
                newNorms.push_back(v.normal[1]);
                newNorms.push_back(v.normal[2]);
            }

            if (!tex.empty()) {
                newTex.push_back(v.texCoord[0]);
                newTex.push_back(v.texCoord[1]);
            }
        }

        vert = std::move(newVerts);
        if (!norm.empty()) norm = std::move(newNorms);
        if (!tex.empty()) tex = std::move(newTex);
        indices = std::move(newIndices);
    }

    void Mesh::generateBuffers() {
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        auto setupAttr = [](GLuint &vbo, const std::vector<GLfloat> &d, int idx, int sz) {
            if (d.empty()) return;
            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, d.size() * sizeof(GLfloat), d.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(idx, sz, GL_FLOAT, GL_FALSE, 0, (void*)0);
            glEnableVertexAttribArray(idx);
        };

        setupAttr(positionVBO, vert, 0, 3);
        setupAttr(normalVBO, norm, 1, 3);
        setupAttr(texCoordVBO, tex, 2, 2);
        setupAttr(tangentVBO, tangent, 3, 3);
        setupAttr(bitangentVBO, bitangent, 4, 3);

        if (!indices.empty()) {
            glGenBuffers(1, &EBO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
        }

        glBindVertexArray(0);
    }

    void Mesh::updateBuffers() {
        if (!VAO) return;
        glBindVertexArray(VAO);

        if (positionVBO && !vert.empty()) {
            glBindBuffer(GL_ARRAY_BUFFER, positionVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, vert.size() * sizeof(GLfloat), vert.data());
        }

        if (normalVBO && !norm.empty()) {
            glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, norm.size() * sizeof(GLfloat), norm.data());
        }

        if (texCoordVBO && !tex.empty()) {
            glBindBuffer(GL_ARRAY_BUFFER, texCoordVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, tex.size() * sizeof(GLfloat), tex.data());
        }

        glBindVertexArray(0);
    }

    void Mesh::draw() {
        glBindVertexArray(VAO);
        if (!indices.empty()) glDrawElements(shape_type, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
        else glDrawArrays(shape_type, 0, (GLsizei)(vert.size() / 3));
        glBindVertexArray(0);
    }

    void Mesh::bindTexture(gl::ShaderProgram &shader, const std::string texture_name) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        shader.setUniform(texture_name, 0);

        if (normalMapTexture) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, normalMapTexture);
            shader.setUniform("normalMap", 1);
        }
    }

    void Mesh::drawWithForcedTexture(gl::ShaderProgram &shader, GLuint forced_texture, const std::string texture_name) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, forced_texture);
        shader.setUniform(texture_name, 0);

        if (normalMapTexture) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, normalMapTexture);
            shader.setUniform("normalMap", 1);
        }

        draw();
    }

    void Mesh::bindCubemapTexture(gl::ShaderProgram &shader, GLuint cubemap, const std::string texture_name) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
        shader.setUniform(texture_name, 1);
    }

    void Mesh::saveOriginal() {
        originalVert = vert;
        originalNorm = norm;
        hasOriginal = true;
    }

    void Mesh::resetToOriginal() {
        if (hasOriginal) {
            vert = originalVert;
            norm = originalNorm;
            updateBuffers();
        }
    }

    void Mesh::recalculateNormals() {
        if (vert.empty()) return;

        norm.assign(vert.size(), 0.0f);

        if (!indices.empty()) {
            for (size_t i = 0; i + 2 < indices.size(); i += 3) {
                int i0 = (int)indices[i] * 3;
                int i1 = (int)indices[i + 1] * 3;
                int i2 = (int)indices[i + 2] * 3;

                glm::vec3 v0(vert[i0], vert[i0 + 1], vert[i0 + 2]);
                glm::vec3 v1(vert[i1], vert[i1 + 1], vert[i1 + 2]);
                glm::vec3 v2(vert[i2], vert[i2 + 1], vert[i2 + 2]);

                glm::vec3 n = glm::normalize(glm::cross(v1 - v0, v2 - v0));

                norm[i0] += n.x; norm[i0 + 1] += n.y; norm[i0 + 2] += n.z;
                norm[i1] += n.x; norm[i1 + 1] += n.y; norm[i1 + 2] += n.z;
                norm[i2] += n.x; norm[i2 + 1] += n.y; norm[i2 + 2] += n.z;
            }
        } else {
            for (size_t i = 0; i + 8 < vert.size(); i += 9) {
                glm::vec3 v0(vert[i], vert[i + 1], vert[i + 2]);
                glm::vec3 v1(vert[i + 3], vert[i + 4], vert[i + 5]);
                glm::vec3 v2(vert[i + 6], vert[i + 7], vert[i + 8]);
                glm::vec3 n = glm::normalize(glm::cross(v1 - v0, v2 - v0));
                norm[i] = norm[i + 3] = norm[i + 6] = n.x;
                norm[i + 1] = norm[i + 4] = norm[i + 7] = n.y;
                norm[i + 2] = norm[i + 5] = norm[i + 8] = n.z;
            }
        }

        for (size_t i = 0; i < norm.size(); i += 3) {
            glm::vec3 n(norm[i], norm[i + 1], norm[i + 2]);
            float len = glm::length(n);
            if (len > 1e-8f) {
                n /= len;
                norm[i] = n.x; norm[i + 1] = n.y; norm[i + 2] = n.z;
            }
        }
    }

    void Mesh::generateTangentBitangent() {
        if (vert.empty() || tex.empty()) return;

        tangent.assign(vert.size(), 0.0f);
        bitangent.assign(vert.size(), 0.0f);

        auto addTri = [&](GLuint i0, GLuint i1, GLuint i2) {
            size_t p0 = (size_t)i0 * 3, p1 = (size_t)i1 * 3, p2 = (size_t)i2 * 3;
            size_t t0 = (size_t)i0 * 2, t1 = (size_t)i1 * 2, t2 = (size_t)i2 * 2;
            if (p2 + 2 >= vert.size()) return;
            if (t2 + 1 >= tex.size()) return;

            glm::vec3 v0(vert[p0], vert[p0 + 1], vert[p0 + 2]);
            glm::vec3 v1(vert[p1], vert[p1 + 1], vert[p1 + 2]);
            glm::vec3 v2(vert[p2], vert[p2 + 1], vert[p2 + 2]);

            glm::vec2 uv0(tex[t0], tex[t0 + 1]);
            glm::vec2 uv1(tex[t1], tex[t1 + 1]);
            glm::vec2 uv2(tex[t2], tex[t2 + 1]);

            glm::vec3 e1 = v1 - v0;
            glm::vec3 e2 = v2 - v0;

            glm::vec2 duv1 = uv1 - uv0;
            glm::vec2 duv2 = uv2 - uv0;

            float denom = duv1.x * duv2.y - duv2.x * duv1.y;
            if (std::abs(denom) < 1e-8f) return;

            float f = 1.0f / denom;
            glm::vec3 tan = f * (duv2.y * e1 - duv1.y * e2);
            glm::vec3 bitan = f * (-duv2.x * e1 + duv1.x * e2);

            tangent[p0] += tan.x; tangent[p0 + 1] += tan.y; tangent[p0 + 2] += tan.z;
            tangent[p1] += tan.x; tangent[p1 + 1] += tan.y; tangent[p1 + 2] += tan.z;
            tangent[p2] += tan.x; tangent[p2 + 1] += tan.y; tangent[p2 + 2] += tan.z;

            bitangent[p0] += bitan.x; bitangent[p0 + 1] += bitan.y; bitangent[p0 + 2] += bitan.z;
            bitangent[p1] += bitan.x; bitangent[p1 + 1] += bitan.y; bitangent[p1 + 2] += bitan.z;
            bitangent[p2] += bitan.x; bitangent[p2 + 1] += bitan.y; bitangent[p2 + 2] += bitan.z;
        };

        if (!indices.empty()) {
            for (size_t i = 0; i + 2 < indices.size(); i += 3) {
                addTri(indices[i], indices[i + 1], indices[i + 2]);
            }
        } else {
            GLuint vcount = (GLuint)(vert.size() / 3);
            for (GLuint i = 0; i + 2 < vcount; i += 3) {
                addTri(i, i + 1, i + 2);
            }
        }

        for (size_t i = 0; i < tangent.size(); i += 3) {
            glm::vec3 t(tangent[i], tangent[i + 1], tangent[i + 2]);
            float len = glm::length(t);
            if (len > 1e-8f) {
                t /= len;
                tangent[i] = t.x; tangent[i + 1] = t.y; tangent[i + 2] = t.z;
            }
        }

        for (size_t i = 0; i < bitangent.size(); i += 3) {
            glm::vec3 b(bitangent[i], bitangent[i + 1], bitangent[i + 2]);
            float len = glm::length(b);
            if (len > 1e-8f) {
                b /= len;
                bitangent[i] = b.x; bitangent[i + 1] = b.y; bitangent[i + 2] = b.z;
            }
        }
    }

    void Mesh::scale(float factor) { scale(factor, factor, factor); }

    void Mesh::scale(float sx, float sy, float sz) {
        for (size_t i = 0; i < vert.size(); i += 3) {
            vert[i] *= sx;
            vert[i + 1] *= sy;
            vert[i + 2] *= sz;
        }
    }

    void Mesh::translate(float tx, float ty, float tz) {
        for (size_t i = 0; i < vert.size(); i += 3) {
            vert[i] += tx;
            vert[i + 1] += ty;
            vert[i + 2] += tz;
        }
    }

    void Mesh::bend(DeformAxis axis, float angle, float center, float range) {
        for (size_t i = 0; i < vert.size(); i += 3) {
            float x = vert[i], y = vert[i + 1], z = vert[i + 2];
            if (axis == DeformAxis::Y) {
                float dist = y - center;
                if (range == 0.0f || std::abs(dist) < range) {
                    float theta = dist * angle;
                    vert[i] = x * std::cos(theta) - dist * std::sin(theta);
                    vert[i + 1] = center + x * std::sin(theta) + dist * std::cos(theta);
                }
            }
            (void)z;
        }
    }

    void Mesh::twist(DeformAxis axis, float angle, float center) {
        for (size_t i = 0; i < vert.size(); i += 3) {
            if (axis == DeformAxis::Y) {
                float theta = (vert[i + 1] - center) * angle;
                float s = std::sin(theta), c = std::cos(theta);
                float x = vert[i], z = vert[i + 2];
                vert[i] = x * c - z * s;
                vert[i + 2] = x * s + z * c;
            }
        }
    }

    void Mesh::wave(DeformAxis axis, float amplitude, float frequency, float phase) {
        for (size_t i = 0; i < vert.size(); i += 3) {
            float x = vert[i], y = vert[i + 1];
            if (axis == DeformAxis::Y) vert[i + 1] += amplitude * std::sin(x * frequency + phase);
            else if (axis == DeformAxis::X) vert[i] += amplitude * std::sin(y * frequency + phase);
            else if (axis == DeformAxis::Z) vert[i + 2] += amplitude * std::sin(y * frequency + phase);
        }
    }

    void Mesh::ripple(float amplitude, float frequency, float phase, float cx, float cy, float cz) {
        for (size_t i = 0; i < vert.size(); i += 3) {
            float dx = vert[i] - cx;
            float dy = vert[i + 1] - cy;
            float dz = vert[i + 2] - cz;
            float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
            float displacement = amplitude * std::sin(dist * frequency + phase);
            if (dist > 1e-4f) {
                vert[i] += (dx / dist) * displacement;
                vert[i + 1] += (dy / dist) * displacement;
                vert[i + 2] += (dz / dist) * displacement;
            }
        }
    }

    void Mesh::bulge(float factor, float cx, float cy, float cz, float radius) {
        for (size_t i = 0; i < vert.size(); i += 3) {
            float dx = vert[i] - cx;
            float dy = vert[i + 1] - cy;
            float dz = vert[i + 2] - cz;
            float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

            if (dist < radius) {
                float weight = 1.0f - (dist / radius);
                float amount = factor * weight;
                vert[i] += dx * amount;
                vert[i + 1] += dy * amount;
                vert[i + 2] += dz * amount;
            }
        }
    }

    void Mesh::uvScroll(float uOffset, float vOffset) {
        for (size_t i = 0; i < tex.size(); i += 2) {
            tex[i] += uOffset;
            tex[i + 1] += vOffset;
        }
        if (texCoordVBO && !tex.empty()) {
            glBindBuffer(GL_ARRAY_BUFFER, texCoordVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, tex.size() * sizeof(GLfloat), tex.data());
        }
    }

    void Mesh::applyHSV(float h, float s, float v) { (void)h; (void)s; (void)v; }
    void Mesh::applyColorGrading(const glm::vec3 &c) { (void)c; }
    void Mesh::applyChromaticAberration(float intensity) { (void)intensity; }

    void Mesh::setNormalMap(GLuint t) { normalMapTexture = t; }
    void Mesh::setParallaxMap(GLuint t) { parallaxMapTexture = t; }
    void Mesh::setDisplacementMap(GLuint t) { displacementMapTexture = t; }
    void Mesh::setAmbientOcclusionMap(GLuint t) { aoMapTexture = t; }

    Model::Model(const std::string &filename) {
        if (!openModel(filename)) throw mx::Exception("Error: Could not load model");
    }

    Model::Model(Model &&m) noexcept
        : meshes(std::move(m.meshes)), program(m.program), tex_name(std::move(m.tex_name)) {
        m.program = nullptr;
    }

    Model &Model::operator=(Model &&m) noexcept {
        if (this != &m) {
            meshes = std::move(m.meshes);
            program = m.program;
            tex_name = std::move(m.tex_name);
            m.program = nullptr;
        }
        return *this;
    }

    static bool ends_with(const std::string &s, const std::string &suf) {
        return s.size() >= suf.size() && s.compare(s.size() - suf.size(), suf.size(), suf) == 0;
    }

    bool Model::openModel(const std::string &filename, bool compress) {
        std::cout << "mx: Open model: " << filename << std::endl;

        meshes.clear();

        std::vector<char> buffer;
        std::string text;

        try {
            buffer = mx::readFile(filename);
            if (buffer.empty()) {
                std::cerr << "mx: Error - File not found or empty buffer: " << filename << std::endl;
                return false;
            }

            if (ends_with(filename, ".mxmod.z")) {
                text = decompressString(buffer.data(), static_cast<uLong>(buffer.size()));
            } else {
                text.assign(buffer.begin(), buffer.end());
            }

            return openModelString(filename, text, compress);
        }
        catch (const mx::Exception &e) {
            std::cerr << "mx: Exception in openModel(" << filename << "): " << e.text() << std::endl;
            return false;
        }
        catch (const std::exception &e) {
            std::cerr << "mx: std::exception in openModel(" << filename << "): " << e.what() << std::endl;
            return false;
        }
        catch (...) {
            std::cerr << "mx: Unknown exception in openModel(" << filename << ")" << std::endl;
            return false;
        }
    }

    void Model::dumpMeshStats(const Mesh &m, size_t meshIndex, bool didCompress, bool compressRequested) {
        mx::system_out
            << "mx: Mesh[" << meshIndex << "] "
            << (compressRequested ? "[compress=on]" : "[compress=off]") << " "
            << (didCompress ? "[compressed]" : "[not-compressed]") << "\n{\n"
            << "    verts: " << (m.vert.size() / 3) << "\n"
            << "    norms: " << (m.norm.size() / 3) <<  "\n"
            << "    tex:   " << (m.tex.size() / 2) << "\n"
            << "    tang:  " << (m.tangent.size() / 3) << "\n"
            << "    bitan: " << (m.bitangent.size() / 3) << "\n"
            << "    index: " << m.indices.size() << "\n}\n";
    }

    bool Model::openModelString(const std::string &filename, const std::string &text, bool compress) {
        std::string cleaned = text;

        if (cleaned.size() >= 3 &&
            (unsigned char)cleaned[0] == 0xEF &&
            (unsigned char)cleaned[1] == 0xBB &&
            (unsigned char)cleaned[2] == 0xBF) {
            cleaned.erase(0, 3);
        }

        mx::system_out
            << "mx: Parsing model: " << filename << "\n"
            << "mx: compress flag: " << (compress ? "true" : "false") << "\n";

        std::istringstream file(cleaned);
        Mesh currentMesh;
        int type = -1;
        size_t count = 0;
        std::string line;

        auto trim = [](std::string &s) {
            size_t b = s.find_first_not_of(" \t\r\n");
            if (b == std::string::npos) { s.clear(); return; }
            size_t e = s.find_last_not_of(" \t\r\n");
            s = s.substr(b, e - b + 1);
        };

        auto finalize = [&]() {
            if (currentMesh.vert.empty()) return;

            bool didCompress = false;

            mx::system_out << "mx: Finalizing mesh\n";

            if (currentMesh.norm.empty()) {
                mx::system_out << "mx: normals missing, recalculating\n";
                currentMesh.recalculateNormals();
            }

            if (!currentMesh.tex.empty()) {
                if (currentMesh.tangent.empty() || currentMesh.bitangent.empty()) {
                    mx::system_out << "mx: generating tangent/bitangent\n";
                    currentMesh.generateTangentBitangent();
                }
            }

            if (compress && currentMesh.indices.empty()) {
                mx::system_out << "mx: compressing vertices\n";
                currentMesh.compressIndices();
                didCompress = true;
            } else {
                if (compress && !currentMesh.indices.empty())
                    mx::system_out << "mx: compress requested but indices already exist, skipping\n";
                if (!compress)
                    mx::system_out << "mx: compression disabled\n";
            }

            dumpMeshStats(currentMesh, meshes.size(), didCompress, compress);
            currentMesh.generateBuffers();
            meshes.push_back(std::move(currentMesh));

            currentMesh = Mesh();
            type = -1;
            count = 0;
        };

        while (std::getline(file, line)) {
            if (line.empty()) continue;

            size_t commentPos = line.find('#');
            if (commentPos != std::string::npos) line = line.substr(0, commentPos);

            trim(line);
            if (line.empty()) continue;

            if (line.size() >= 3 &&
                (unsigned char)line[0] == 0xEF &&
                (unsigned char)line[1] == 0xBB &&
                (unsigned char)line[2] == 0xBF) {
                line.erase(0, 3);
                trim(line);
                if (line.empty()) continue;
            }

            std::istringstream s(line);
            std::string tag;
            s >> tag;
            if (tag.empty()) continue;

            if (tag == "tri") {
                finalize();
                GLuint st = 0, ti = 0;
                s >> st >> ti;
                currentMesh.texture = ti;
                currentMesh.setShapeType(st);
                type = -1;
                continue;
            }

            parseLine(line, currentMesh, type, count);
        }

        finalize();

        if (meshes.empty()) {
            std::cerr << "mx: Warning - No meshes loaded from " << filename << std::endl;
            return false;
        }

        size_t totalVerts = 0;
        size_t totalIdx = 0;
        for (const auto &m : meshes) {
            totalVerts += m.vert.size() / 3;
            totalIdx += m.indices.size();
        }

        mx::system_out
            << "mx: Model summary\n"
            << "mx: meshes: " << meshes.size() << "\n"
            << "mx: total vertices: " << totalVerts << "\n"
            << "mx: total indices:  " << totalIdx << "\n"
            << "mx: Model Loaded: " << filename << "\n";

        return true;
    }
    
    void Model::parseLine(const std::string &line, Mesh &currentMesh, int &type, size_t &count) {
        if (line.empty()) return;

        auto is_data_line = [&](char c) {
            return (c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.';
        };

        size_t p = line.find_first_not_of(" \t\r\n");
        if (p == std::string::npos) return;

        std::istringstream stream(line);

        if (is_data_line(line[p])) {
            float x = 0.0f, y = 0.0f, z = 0.0f;

            switch (type) {
                case 0:
                    if (stream >> x >> y >> z) {
                        if (currentMesh.vertIndex + 3 > currentMesh.vert.size()) {
                            currentMesh.vert.push_back(x);
                            currentMesh.vert.push_back(y);
                            currentMesh.vert.push_back(z);
                        } else {
                            currentMesh.vert[currentMesh.vertIndex++] = x;
                            currentMesh.vert[currentMesh.vertIndex++] = y;
                            currentMesh.vert[currentMesh.vertIndex++] = z;
                        }
                    }
                    break;

                case 1:
                    if (stream >> x >> y) {
                        if (currentMesh.texIndex + 2 > currentMesh.tex.size()) {
                            currentMesh.tex.push_back(x);
                            currentMesh.tex.push_back(y);
                        } else {
                            currentMesh.tex[currentMesh.texIndex++] = x;
                            currentMesh.tex[currentMesh.texIndex++] = y;
                        }
                    }
                    break;

                case 2:
                    if (stream >> x >> y >> z) {
                        if (currentMesh.normIndex + 3 > currentMesh.norm.size()) {
                            currentMesh.norm.push_back(x);
                            currentMesh.norm.push_back(y);
                            currentMesh.norm.push_back(z);
                        } else {
                            currentMesh.norm[currentMesh.normIndex++] = x;
                            currentMesh.norm[currentMesh.normIndex++] = y;
                            currentMesh.norm[currentMesh.normIndex++] = z;
                        }
                    }
                    break;

                case 3:
                    if (stream >> x >> y >> z) {
                        currentMesh.tangent.push_back(x);
                        currentMesh.tangent.push_back(y);
                        currentMesh.tangent.push_back(z);
                    }
                    break;

                case 4:
                    if (stream >> x >> y >> z) {
                        currentMesh.bitangent.push_back(x);
                        currentMesh.bitangent.push_back(y);
                        currentMesh.bitangent.push_back(z);
                    }
                    break;

                case 5: {
                    GLuint idx = 0;
                    while (stream >> idx) currentMesh.indices.push_back(idx);
                } break;

                default:
                    break;
            }
            return;
        }

        std::string tag;
        stream >> tag;
        if (tag.empty()) return;

        if (tag == "vert") {
            stream >> count;
            currentMesh.vert.clear();
            currentMesh.vert.resize(count * 3);
            currentMesh.vertIndex = 0;
            type = 0;
            return;
        }

        if (tag == "tex") {
            stream >> count;
            currentMesh.tex.clear();
            currentMesh.tex.resize(count * 2);
            currentMesh.texIndex = 0;
            type = 1;
            return;
        }

        if (tag == "norm") {
            stream >> count;
            currentMesh.norm.clear();
            currentMesh.norm.resize(count * 3);
            currentMesh.normIndex = 0;
            type = 2;
            return;
        }

        if (tag == "tangent") {
            stream >> count;
            currentMesh.tangent.clear();
            currentMesh.tangent.reserve(count * 3);
            type = 3;
            return;
        }

        if (tag == "bitangent") {
            stream >> count;
            currentMesh.bitangent.clear();
            currentMesh.bitangent.reserve(count * 3);
            type = 4;
            return;
        }

        if (tag == "indices") {
            stream >> count;
            currentMesh.indices.clear();
            currentMesh.indices.reserve(count);
            type = 5;
            return;
        }
    }

    void Model::setShaderProgram(gl::ShaderProgram *shader_program, const std::string texture_name) {
        program = shader_program;
        tex_name = texture_name;
    }

    void Model::drawArrays() {
        if (!program) throw mx::Exception("Shader program must be set in Model before drawArrays");
        for (auto &mesh : meshes) {
            mesh.bindTexture(*program, tex_name);
            mesh.draw();
        }
    }

    void Model::drawArraysWithTexture(GLuint texture, const std::string texture_name) {
        if (!program) throw mx::Exception("Shader program must be set in Model before drawArraysWithTexture");
        for (auto &mesh : meshes) mesh.drawWithForcedTexture(*program, texture, texture_name);
    }

    void Model::drawArraysWithCubemap(GLuint cubemap, const std::string texture_name) {
        if (!program) throw mx::Exception("Shader program must be set in Model before drawArraysWithCubemap");
        for (auto &mesh : meshes) {
            mesh.bindCubemapTexture(*program, cubemap, texture_name);
            mesh.draw();
        }
    }

    void Model::setTextures(const std::vector<GLuint> &textures) {
        if (textures.empty()) throw mx::Exception("At least one texture is required");
        for (size_t i = 0; i < meshes.size(); ++i) {
            GLuint pos = meshes.at(i).texture;
            if (pos < textures.size()) meshes.at(i).texture = textures[pos];
            else throw mx::Exception("Texture index out of range for this model");
        }
    }

    void Model::setTextures(gl::GLWindow *win, const std::string &filename, std::string prefix) {
        if (!win) return;
        std::vector<GLuint> text;
        std::fstream file;
        file.open(filename, std::ios::in);
        if (!file.is_open()) throw mx::Exception("Error could not open file: " + filename + " for texture");

        while (!file.eof()) {
            std::string line;
            std::getline(file, line);
            if (file && !line.empty()) {
                std::string final_path = prefix + "/" + line;
                GLuint texid = gl::loadTexture(final_path);
                text.push_back(texid);
                mx::system_out << "mx: Loaded Texture: " << texid << " -> " << final_path << "\n";
            }
        }
        file.close();
        setTextures(text);
    }

    void Model::saveOriginal() { for (auto &m : meshes) m.saveOriginal(); }
    void Model::resetToOriginal() { for (auto &m : meshes) m.resetToOriginal(); }
    void Model::updateBuffers() { for (auto &m : meshes) m.updateBuffers(); }
    void Model::recalculateNormals() { for (auto &m : meshes) m.recalculateNormals(); }

    void Model::scale(float factor) { for (auto &m : meshes) m.scale(factor); }
    void Model::scale(float sx, float sy, float sz) { for (auto &m : meshes) m.scale(sx, sy, sz); }
    void Model::translate(float tx, float ty, float tz) { for (auto &m : meshes) m.translate(tx, ty, tz); }
    void Model::bend(DeformAxis axis, float angle, float center, float range) { for (auto &m : meshes) m.bend(axis, angle, center, range); }
    void Model::twist(DeformAxis axis, float angle, float center) { for (auto &m : meshes) m.twist(axis, angle, center); }
    void Model::wave(DeformAxis axis, float amplitude, float frequency, float phase) { for (auto &m : meshes) m.wave(axis, amplitude, frequency, phase); }
    void Model::ripple(float amplitude, float frequency, float phase, float cx, float cy, float cz) { for (auto &m : meshes) m.ripple(amplitude, frequency, phase, cx, cy, cz); }
    void Model::bulge(float factor, float cx, float cy, float cz, float radius) { for (auto &m : meshes) m.bulge(factor, cx, cy, cz, radius); }
    void Model::uvScroll(float uOffset, float vOffset) { for (auto &m : meshes) m.uvScroll(uOffset, vOffset); }
    void Model::applyHSV(float hueShift, float saturation, float value) { for (auto &m : meshes) m.applyHSV(hueShift, saturation, value); }
    void Model::applyColorGrading(const glm::vec3 &colorShift) { for (auto &m : meshes) m.applyColorGrading(colorShift); }
    void Model::applyChromaticAberration(float intensity) { for (auto &m : meshes) m.applyChromaticAberration(intensity); }

    void Model::setNormalMap(GLuint normalMapTexture) { for (auto &m : meshes) m.setNormalMap(normalMapTexture); }
    void Model::setParallaxMap(GLuint heightMapTexture) { for (auto &m : meshes) m.setParallaxMap(heightMapTexture); }
    void Model::setDisplacementMap(GLuint displacementMapTexture) { for (auto &m : meshes) m.setDisplacementMap(displacementMapTexture); }
    void Model::setAmbientOcclusionMap(GLuint aoMapTexture) { for (auto &m : meshes) m.setAmbientOcclusionMap(aoMapTexture); }
    void Model::generateTangentBitangent() { for (auto &m : meshes) m.generateTangentBitangent(); }

}
#endif
