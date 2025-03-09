#ifdef __EMSCRIPTEN__
#include"config.hpp"
#else
#include"config.h"
#endif
#ifdef WITH_GL
#include"model.hpp"
#include<sstream>
#include<fstream>
#include<vector>
#include<unordered_map>
#include<functional>
#include<cstddef>

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

    

    Mesh::Mesh(Mesh &&m)
        : vert(std::move(m.vert)),
          tex(std::move(m.tex)),
          norm(std::move(m.norm)),
          shape_type(m.shape_type),
          texture{m.texture},
          VAO(m.VAO),
          positionVBO(m.positionVBO),
          normalVBO(m.normalVBO),
          texCoordVBO(m.texCoordVBO),
          vertIndex(m.vertIndex),
          texIndex(m.texIndex),
          normIndex(m.normIndex) {
          m.VAO = m.positionVBO = m.normalVBO = m.texCoordVBO = 0;
          m.vertIndex = m.texIndex = m.normIndex = 0;
          m.shape_type = 0;
          m.texture = 0;
    }

    Mesh &Mesh::operator=(Mesh &&m) noexcept {
        if(this != &m) {
            vert = std::move(m.vert);
            tex = std::move(m.tex);
            norm = std::move(m.norm);
            VAO = m.VAO;
            positionVBO = m.positionVBO;
            normalVBO = m.normalVBO;
            texCoordVBO = m.texCoordVBO;
            shape_type = m.shape_type;
            vertIndex = m.vertIndex;
            texIndex = m.texIndex;
            normIndex = m.normIndex;
            texture = m.texture;
            m.VAO = 0;
            m.positionVBO = 0;
            m.normalVBO = 0;
            m.texCoordVBO = 0;
            m.shape_type = 0;
            m.vertIndex = 0;
            m.texIndex = 0;
            m.normIndex = 0;
        }
        return *this;
    }

    Mesh::~Mesh() {
        cleanup();
    }

    void Mesh::cleanup() {
        if (VAO) {
            glDeleteVertexArrays(1, &VAO);
            VAO = 0;
        }
        if (positionVBO) {
            glDeleteBuffers(1, &positionVBO);
            positionVBO = 0;
        }
        if (normalVBO) {
            glDeleteBuffers(1, &normalVBO);
            normalVBO = 0;
        }
        if (texCoordVBO) {
            glDeleteBuffers(1, &texCoordVBO);
            texCoordVBO = 0;
        }
        if (EBO) {
            glDeleteBuffers(1, &EBO);
            EBO = 0;
        }
        if(texture) {
             glDeleteTextures(1, &texture);
        }
    }

    void Mesh::compressIndices() {
        std::cout << "mx: Uncompressed Mesh: V: " << vert.size() << " T: " << tex.size() << " N: " << norm.size() << "\n";
        size_t numVertices =vert.size() / 3;
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
                GLuint newIndex = static_cast<GLuint>(uniqueVertices.size());
                uniqueVertices.push_back(vertex);
                vertexToIndex[vertex] = newIndex;
                newIndices.push_back(newIndex);
            }
        }
        std::vector<GLfloat> newVerts;
        std::vector<GLfloat> newNorms;
        std::vector<GLfloat> newTex;
        newVerts.reserve(uniqueVertices.size() * 3);
        if (!norm.empty()) newNorms.reserve(uniqueVertices.size() * 3);
        if (!tex.empty()) newTex.reserve(uniqueVertices.size() * 2);
        for (const auto& v : uniqueVertices) {
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
        if (!norm.empty())
            norm = std::move(newNorms);
        if (!tex.empty())
            tex = std::move(newTex);
        indices = std::move(newIndices);
        std::cout << "mx: Compressed Mesh: V: " << vert.size() << " T: " << tex.size() << " N: " << norm.size() << "\n";
        std::cout << "mx: Mesh Indices: " << indices.size() << "\n";
    }

    void Mesh::generateBuffers() {
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);
        glGenBuffers(1, &positionVBO);
        glBindBuffer(GL_ARRAY_BUFFER, positionVBO);
        glBufferData(GL_ARRAY_BUFFER, vert.size() * sizeof(GLfloat), vert.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
        glEnableVertexAttribArray(0);
        if (!norm.empty()) {
            glGenBuffers(1, &normalVBO);
            glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
            glBufferData(GL_ARRAY_BUFFER, norm.size() * sizeof(GLfloat), norm.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
            glEnableVertexAttribArray(1);
        }
        if (!tex.empty()) {
            glGenBuffers(1, &texCoordVBO);
            glBindBuffer(GL_ARRAY_BUFFER, texCoordVBO);
            glBufferData(GL_ARRAY_BUFFER, tex.size() * sizeof(GLfloat), tex.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
            glEnableVertexAttribArray(2);
        }
        if (!indices.empty()) {
            glGenBuffers(1, &EBO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
        }    
        glBindVertexArray(0);
    }

    void Mesh::draw() {
        glBindVertexArray(VAO);
        if (!indices.empty()) {
            glDrawElements(shape_type, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
        } else {
            glDrawArrays(shape_type, 0, static_cast<GLsizei>(vert.size() / 3));
        }
        glBindVertexArray(0);
    }
    
    void Mesh::drawWithForcedTexture(gl::ShaderProgram &shader, GLuint texture, const std::string texture_name) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        shader.setUniform(texture_name, 0);
        glBindVertexArray(VAO);
        if (!indices.empty()) {
            glDrawElements(shape_type, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
        } else {
            glDrawArrays(shape_type, 0, static_cast<GLsizei>(vert.size() / 3));
        }
        glBindVertexArray(0);
    }

    void Mesh::setShapeType(GLuint type) {
        switch(type) {
            case 0:
                shape_type = GL_TRIANGLES;
                break;
            case 1:
                shape_type = GL_TRIANGLE_STRIP;
                break;
            case 2:
                shape_type = GL_TRIANGLE_FAN;
                break;
        }
    }

    void Mesh::bindTexture(gl::ShaderProgram &shader, const std::string texture_name) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        shader.setUniform(texture_name, 0);
    }

    Model::Model(Model &&m) : meshes{std::move(m.meshes)} {}

    Model &Model::operator=(Model &&m) {
        meshes = std::move(m.meshes);
        return *this;
    }

    Model::Model(const std::string &filename) {
        if (!openModel(filename)) {
            throw mx::Exception("Error: Could not load model");
        }
    }

    bool Model::openCompressedModel(const std::string &filename) {
        std::vector<char> buffer = mx::readFile(filename);
        std::string text = decompressString(buffer.data(), static_cast<uLong>(buffer.size()));
        return openModelString(filename, text, true);
    }

    bool Model::openModel(const std::string &filename, bool compress) {
        if(filename.rfind("mxmod.z") != std::string::npos) {
            return openCompressedModel(filename);
        }
        std::vector<char> buffer = mx::readFile(filename);
        std::string text(buffer.begin(), buffer.end());
        return openModelString(filename, text, compress);
    }

    bool Model::openModelString(const std::string &filename, const std::string &text, bool compress) {
        std::istringstream file(text);
        Mesh currentMesh;
        int type = -1;
        size_t count = 0;

        while (!file.eof()) {
            std::string line;
            std::getline(file, line);
            if (!line.empty()) {
                auto pos = line.find('#');                
                if(pos != std::string::npos) {
                    line = line.substr(0, pos);
                    if(line.empty())
                        continue;
                }
                if (line.find("tri") == 0) {
                    if (!currentMesh.vert.empty()) {
                        if (count * 3 != currentMesh.vert.size()) {
                            std::ostringstream stream;
                            stream << "mx: Model :" << filename << " invalid vertex count unaligned expected: " << (count) << " found: " << currentMesh.vert.size()/3;
                            throw mx::Exception(stream.str());
                        }              
                        if (count * 2 != currentMesh.tex.size()) {
                            std::ostringstream stream;
                            stream << "mx: Model :" << filename << " invalid texture count unaligned expected: " << (count) << " found: " << currentMesh.tex.size()/3;
                            throw mx::Exception(stream.str());
                        }
                        if (count * 3 != currentMesh.norm.size()) {
                            std::ostringstream stream;
                            stream << "mx: Model :" << filename << " invalid normal count unaligned expected: " << (count) << " found: " << currentMesh.norm.size()/3;
                            throw mx::Exception(stream.str());
                        }
                        meshes.push_back(std::move(currentMesh));
                        currentMesh = Mesh();
                    }
                    std::istringstream stream(line);
                    std::string dummy;
                    GLuint shapeType = 0;
                    GLuint texture_index = 0;
                    stream >> dummy >> shapeType>> texture_index;
                    currentMesh.texture = texture_index;
                    currentMesh.setShapeType(shapeType);
                    type = -1;
                } else {
                    parseLine(line, currentMesh, type, count);
                }
            }
        }
        if (!currentMesh.vert.empty()) {
            if (count * 3 != currentMesh.vert.size()) {
                std::ostringstream stream;
                stream << "mx: Model :" << filename << " invalid vertex count unaligned expected: " << (count) << " found: " << currentMesh.vert.size()/3;
                throw mx::Exception(stream.str());
            }              
            if (count * 2 != currentMesh.tex.size()) {
                std::ostringstream stream;
                stream << "mx: Model :" << filename << " invalid texture count unaligned expected: " << (count) << " found: " << currentMesh.tex.size()/3;
                throw mx::Exception(stream.str());
            }
            if (count * 3 != currentMesh.norm.size()) {
                std::ostringstream stream;
                stream << "mx: Model :" << filename << " invalid normal count unaligned expected: " << (count) << " found: " << currentMesh.norm.size()/3;
                throw mx::Exception(stream.str());
            }
            meshes.push_back(std::move(currentMesh));

        }

        mx::system_out << "mx: Model Loaded: " << filename << "\n";
        if(compress) {
            mx::system_out << "mx: Compressing Indices\n";
        }
        for(auto &m : meshes) {
            if(compress) {
                m.compressIndices();
            }
            m.generateBuffers();
        }
        return true;
    }

    void Model::parseLine(const std::string &line, Mesh &currentMesh, int &type, size_t &count) {
        if (line.empty()) return;

        if (line.find("vert") == 0) {
            std::istringstream stream(line);
            std::string dummy;
            stream >> dummy >> count;
            currentMesh.vert.resize(count * 3);
            currentMesh.vertIndex = 0;
            type = 0;
            return;
        }

        if (line.find("tex") == 0) {
            std::istringstream stream(line);
            std::string dummy;
            stream >> dummy >> count;
            currentMesh.tex.resize(count * 2);
            currentMesh.texIndex = 0;
            type = 1;
            return;
        }

        if (line.find("norm") == 0) {
            std::istringstream stream(line);
            std::string dummy;
            stream >> dummy >> count;
            currentMesh.norm.resize(count * 3);
            currentMesh.normIndex = 0;
            type = 2;
            return;
        }

        std::istringstream stream(line);
        switch (type) {
            case 0: {
                if(currentMesh.vertIndex != count * 3) {
                    if(currentMesh.vertIndex+3 > (count * 3)) {
                        throw mx::Exception("mx: Model loader: Vertex coordinate index overflow: " + std::to_string(count * 3) + "/" + std::to_string(currentMesh.vertIndex+3));
                    }
                }
                float x, y, z;
                if (stream >> x >> y >> z) {
                    currentMesh.vert[currentMesh.vertIndex++] = x;
                    currentMesh.vert[currentMesh.vertIndex++] = y;
                    currentMesh.vert[currentMesh.vertIndex++] = z;
                }
            } break;
            case 1: {
                if(currentMesh.texIndex != count * 2) {
                    if(currentMesh.texIndex+2 > (count * 2)) {
                        throw mx::Exception("mx: Model loader: Texture coordinate index overflow: " + std::to_string(count * 3) + "/" + std::to_string(currentMesh.texIndex+2));
                    }
                }
                float u, v;
                if (stream >> u >> v) {
                    currentMesh.tex[currentMesh.texIndex++] = u;
                    currentMesh.tex[currentMesh.texIndex++] = v;
                }
                
            } break;
            case 2: {
                if(currentMesh.normIndex != count * 3) {
                    if(currentMesh.normIndex+3 > (count * 3)) {
                        throw mx::Exception("mx: Model loader: Norm coordinate index overflow: " + std::to_string(count * 3) + "/" + std::to_string(currentMesh.normIndex+3));
                    }
                }
                float nx, ny, nz;
                if (stream >> nx >> ny >> nz) {
                    currentMesh.norm[currentMesh.normIndex++] = nx;
                    currentMesh.norm[currentMesh.normIndex++] = ny;
                    currentMesh.norm[currentMesh.normIndex++] = nz;
                }
            } break;
            default:
                break;
        }
    }

    void Model::setShaderProgram(gl::ShaderProgram *shader_program, const std::string texture_name) {
        program = shader_program;
        tex_name = texture_name;
    }


    void Model::drawArrays() {
        if(program == nullptr)
            throw mx::Exception("Shader program mut be set in Model before call to drawArrays");

        for (auto &mesh : meshes) {
            mesh.bindTexture(*program, tex_name);
            mesh.draw();
        }
    }

    void Model::drawArraysWithTexture(GLuint texture, const std::string texture_name = "texture1") {
        if(program == nullptr)
            throw mx::Exception("Shader program must be set in model before call to drawArraysWithTexture");
        for(auto &mesh: meshes) {
            mesh.drawWithForcedTexture(*program, texture, texture_name);
        }
    }

    void Model::printData(std::ostream &out) {
        size_t index = 0;
        for(auto &m : meshes) {
            out << "Mesh Index: " << index << " -> {\n";
            for(auto &v : m.vert) {
                out << "\tV: " << v << "\n";
            }
            for(auto &t : m.tex) {
                out << "\tT: " << t << "\n";
            }
            for(auto &n : m.norm) {
                out << "\tN: " << n << "\n";
            }
            out << "\n}\n";
            index++;
        }
    }

    void Model::setTextures(const std::vector<GLuint> &textures) {
        if(textures.empty()) {
            throw mx::Exception("At least one texture is required");
        }
        for(size_t i = 0; i < meshes.size(); ++i) {
            GLuint pos = meshes.at(i).texture;
            if(pos < textures.size())
                meshes.at(i).texture = textures[meshes.at(i).texture];
            else
                throw mx::Exception("Texture index in file on tri statement out of range not enough defined textures");
        }
    }

    void Model::setTextures(gl::GLWindow *win, const std::string &filename, const std::string prefix) {
        std::fstream file;
        file.open(filename, std::ios::in);
        if(!file.is_open()) {
            throw mx::Exception("Error could not open file: " + filename + " for texture");
        }
        std::vector<GLuint> text;
        while(!file.eof()) {
            std::string line;
            std::getline(file, line);
            if(file) {
                std::string final_path = prefix+"/"+line;
                GLuint tex = gl::loadTexture(final_path);
                text.push_back(tex);
                mx::system_out << "mx: Loaded Texture: " << tex << " -> " << final_path << std::endl;
                mx::system_out.flush();
            }
        }
        file.close();
        setTextures(text);
    }
}
#endif