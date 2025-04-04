#include"model.hpp"
#include<sstream>
#include<fstream>

namespace mx {

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
        if(texture) {
             glDeleteTextures(1, &texture);
        }
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
        glBindVertexArray(0);
    }

    void Mesh::draw() {
        glBindVertexArray(VAO);
        glDrawArrays(shape_type, 0, static_cast<GLsizei>(vert.size() / 3));
        glBindVertexArray(0);
    }

    void Mesh::drawWithForcedTexture(gl::ShaderProgram &shader, GLuint texture, const std::string texture_name) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        shader.setUniform(texture_name, 0);
        glBindVertexArray(VAO);
        glDrawArrays(shape_type, 0, static_cast<GLsizei>(vert.size() / 3));
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

    bool Model::openModel(const std::string &filename) {
        std::string s = mx::LoadTextFile(filename.c_str());
        std::istringstream file(s);
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

        for(auto &m : meshes) {
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
        std::string f = mx::LoadTextFile(filename.c_str());
        if(f.empty()) {
            throw mx::Exception("Error loading texture file: " + filename);
        }
        std::istringstream file(f);
        std::vector<GLuint> text;
        while(!file.eof()) {
            std::string line;
            std::getline(file, line);
            if(file) {
                std::string final_path = prefix+"/"+line;
                GLuint tex = gl::loadTexture(final_path);
                text.push_back(tex);
                // mx::system_out << "mx: Loaded Texture: " << tex << " -> " << final_path << std::endl;
                // mx::system_out.flush();
            }
        }
        setTextures(text);
    }
}