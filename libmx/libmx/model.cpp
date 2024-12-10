#include"model.hpp"
#include<sstream>
#include<fstream>

namespace mx {

Mesh::Mesh(Mesh &&m)
    : vert(std::move(m.vert)),
      tex(std::move(m.tex)),
      norm(std::move(m.norm)),
      shape_type(m.shape_type),
      VAO(m.VAO),
      positionVBO(m.positionVBO),
      normalVBO(m.normalVBO),
      texCoordVBO(m.texCoordVBO),
      vertIndex(m.vertIndex),
      texIndex(m.texIndex),
      normIndex(m.normIndex) {
          m.VAO = m.positionVBO = m.normalVBO = m.texCoordVBO = 0;
          m.vertIndex = m.texIndex = m.normIndex = 0;
}
    
    Mesh &Mesh::operator=(Mesh &&m) {
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
        std::ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }

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
                            throw mx::Exception("mx: Model :" + filename + " invalid vertex alignment.\n");
                        }              
                        if (count * 2 != currentMesh.tex.size()) {
                            throw mx::Exception("mx: Model :" + filename + " invalid texture coordinates alignment.\n");
                        }
                        if (count * 3 != currentMesh.norm.size()) {
                            throw mx::Exception("mx: Model :" + filename + " invalid normals alignment.\n");
                        }
                        meshes.push_back(std::move(currentMesh));
                        currentMesh = Mesh();
                    }
                    std::istringstream stream(line);
                    std::string dummy;
                    GLuint shapeType = 0;
                    stream >> dummy >> shapeType;
                    currentMesh.setShapeType(shapeType);
                    type = -1;
                    
                } else {
                    parseLine(line, currentMesh, type, count);
                }
            }
        }
        if (!currentMesh.vert.empty()) {
            if (count * 3 != currentMesh.vert.size()) {
                throw mx::Exception("mx: Model :" + filename + " invalid vertex alignment.\n");
            }              
            if (count * 2 != currentMesh.tex.size()) {
                throw mx::Exception("mx: Model :" + filename + " invalid texture coordinates alignment.\n");
            }
            if (count * 3 != currentMesh.norm.size()) {
                throw mx::Exception("mx: Model :" + filename + " invalid normals alignment.\n");
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
                if(currentMesh.vertIndex+3 > (count * 3)) {
                    throw mx::Exception("mx: Model loader: Vertex overflow");
                }
                float x, y, z;
                if (stream >> x >> y >> z) {
                    currentMesh.vert[currentMesh.vertIndex++] = x;
                    currentMesh.vert[currentMesh.vertIndex++] = y;
                    currentMesh.vert[currentMesh.vertIndex++] = z;
                }
            } break;
            case 1: {
                if(currentMesh.texIndex+2 > (count * 2)) {
                    throw mx::Exception("mx: Model loader: Texture pos overflow");
                }
                float u, v;
                if (stream >> u >> v) {
                    currentMesh.tex[currentMesh.texIndex++] = u;
                    currentMesh.tex[currentMesh.texIndex++] = v;
                }
            } break;
            case 2: {
                if(currentMesh.normIndex+3 > (count * 3)) {
                    throw mx::Exception("mx: Model loader: Noram overflow");
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

    void Model::drawArrays() {
        for (auto &mesh : meshes) {
            mesh.draw();
        }
    }

    void Model::printData() {
        
        for(auto &m : meshes) {
            for(auto &v : m.vert) {
                std::cout << "V: " << v << "\n";
            }
            for(auto &t : m.tex) {
                std::cout << "T: " << t << "\n";
            }
            for(auto &n : m.norm) {
                std::cout << "N: " << n << "\n";
            }
        }
    
    }
}
