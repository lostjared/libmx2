#include"mx.hpp"
#include"argz.hpp"
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <GLES3/gl3.h>
#endif
#include"gl.hpp"
#include"loadpng.hpp"
#include"model.hpp"
#include<random>
#define CHECK_GL_ERROR() \
{ GLenum err = glGetError(); \
if (err != GL_NO_ERROR) \
printf("OpenGL Error: %d at %s:%d\n", err, __FILE__, __LINE__); }
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
class Objects;
class Wall : public gl::GLObject {
public:
    Wall() = default;
    ~Wall() {
        if (vao != 0) glDeleteVertexArrays(1, &vao);
        if (vbo != 0) glDeleteBuffers(1, &vbo);
        if (ebo != 0) glDeleteBuffers(1, &ebo);
        if (textureId != 0) glDeleteTextures(1, &textureId);
    }
    struct WallSegment {
        glm::vec3 start;
        glm::vec3 end;
        float height;
    };
    std::vector<WallSegment> walls;
    GLuint vao = 0, vbo = 0, ebo = 0;
    GLuint textureId = 0;
    gl::ShaderProgram wallShader;
    int mazeGridX = 0;
    int mazeGridZ = 0;
    float mazeCellSize = 0.0f;
    glm::vec3 startPosition = glm::vec3(0.0f);
    glm::vec3 getStartPosition() const { return startPosition; }
    float getWallThickness() const { return wallThickness_; }
    void load(gl::GLWindow *win) override {
#ifndef __EMSCRIPTEN__
        const char *vertexShader = R"(#version 330 core
            layout(location = 0) in vec3 aPos;
            layout(location = 1) in vec2 aTexCoord;
            layout(location = 2) in vec3 aNormal;
            out vec2 TexCoord;
            out vec3 Normal;
            out vec3 FragPos;
            uniform mat4 model;
            uniform mat4 view;
            uniform mat4 projection;
            void main() {
                gl_Position = projection * view * model * vec4(aPos, 1.0);
                FragPos = vec3(model * vec4(aPos, 1.0));
                Normal = mat3(transpose(inverse(model))) * aNormal;
                TexCoord = aTexCoord;
            }
        )";
        const char *fragmentShader = R"(#version 330 core
            in vec2 TexCoord;
            out vec4 color;
            uniform sampler2D wallTexture;
            void main() {
                color = texture(wallTexture, TexCoord);
            }
        )";
#else
        const char *vertexShader = R"(#version 300 es
            precision highp float;
            layout(location = 0) in vec3 aPos;
            layout(location = 1) in vec2 aTexCoord;
            layout(location = 2) in vec3 aNormal;
            out vec2 TexCoord;
            out vec3 Normal;
            out vec3 FragPos;
            uniform mat4 model;
            uniform mat4 view;
            uniform mat4 projection;
            void main() {
                gl_Position = projection * view * model * vec4(aPos, 1.0);
                FragPos = vec3(model * vec4(aPos, 1.0));
                Normal = mat3(transpose(inverse(model))) * aNormal;
                TexCoord = aTexCoord;
            }
        )";
        const char *fragmentShader = R"(#version 300 es
            precision highp float;
            in vec2 TexCoord;
            out vec4 color;
            uniform sampler2D wallTexture;
            void main() {
                color = texture(wallTexture, TexCoord);
            }
        )";
#endif
        if (!wallShader.loadProgramFromText(vertexShader, fragmentShader)) {
            throw mx::Exception("Failed to load wall shader");
        }
        const float size = 50.0f;
        const float wallHeight = 5.0f;
        const float wallThickness = 0.5f;
        size_ = size;
        wallThickness_ = wallThickness;
        mazeGridX = 6;
        mazeGridZ = 6;
        mazeCellSize = (size * 2.0f) / (float)mazeGridX;
        const int gridX = mazeGridX;
        const int gridZ = mazeGridZ;
        const float cellSize = mazeCellSize;
        struct Cell { bool visited = false; bool walls[4] = {true,true,true,true}; };
        std::vector<Cell> grid(gridX * gridZ);
        auto idx = [&](int x,int z){ return z*gridX + x; };
        std::vector<std::pair<int,int>> stack;
        std::mt19937 rng((unsigned)time(nullptr));
        stack.emplace_back(0,0);
        grid[idx(0,0)].visited = true;
        while(!stack.empty()) {
            auto [x,z] = stack.back();
            std::vector<int> dirs;
            if(z>0 && !grid[idx(x,z-1)].visited) dirs.push_back(0);
            if(x<gridX-1 && !grid[idx(x+1,z)].visited) dirs.push_back(1);
            if(z<gridZ-1 && !grid[idx(x,z+1)].visited) dirs.push_back(2);
            if(x>0 && !grid[idx(x-1,z)].visited) dirs.push_back(3);
            if(dirs.empty()) { stack.pop_back(); continue; }
            int d = dirs[rng()%dirs.size()];
            int nx=x, nz=z;
            if(d==0) nz=z-1;
            if(d==1) nx=x+1;
            if(d==2) nz=z+1;
            if(d==3) nx=x-1;
            grid[idx(x,z)].walls[d]=false;
            grid[idx(nx,nz)].walls[(d+2)%4]=false;
            grid[idx(nx,nz)].visited=true;
            stack.emplace_back(nx,nz);
        }
        walls.clear();
        for(int z=0;z<gridZ;++z){
            for(int x=0;x<gridX;++x){
                float cx = -size + x*cellSize;
                float cz = -size + z*cellSize;
                float x0=cx, z0=cz;
                float x1=cx+cellSize, z1=cz+cellSize;
                if(grid[idx(x,z)].walls[0])
                    walls.push_back({{x0,0.0f,z0},{x1,0.0f,z0},wallHeight});
                if(grid[idx(x,z)].walls[3])
                    walls.push_back({{x0,0.0f,z1},{x0,0.0f,z0},wallHeight});
                if (x == gridX - 1 && grid[idx(x,z)].walls[1])
                    walls.push_back({{x1,0.0f,z0},{x1,0.0f,z1},wallHeight});
                if (z == gridZ - 1 && grid[idx(x,z)].walls[2])
                    walls.push_back({{x1,0.0f,z1},{x0,0.0f,z1},wallHeight});
            }
        }
        {
            const float playerRadius = 0.35f;
            const float spawnMargin = 0.5f;  
            startPosition = getSafePointInCell(0, 0, playerRadius, 1.7f, spawnMargin);
            if (checkCollision(startPosition, playerRadius)) {
                for (int z = 0; z < mazeGridZ && checkCollision(startPosition, playerRadius); ++z) {
                    for (int x = 0; x < mazeGridX && checkCollision(startPosition, playerRadius); ++x) {
                        if (x > 0 || z > 0) {
                            startPosition = getSafePointInCell(x, z, playerRadius, 1.7f, spawnMargin);
                        }
                    }
                }
            }
        }
        std::vector<float> vertices;
        std::vector<unsigned int> indices;
        unsigned int indexOffset=0;
        const float tileScale=5.0f;
        const float baseY = -0.03f;
        auto addQuad=[&](const glm::vec3&p0,const glm::vec3&p1,const glm::vec3&p2,const glm::vec3&p3,
                         const glm::vec3&normal,float uMax,float vMax){
            vertices.insert(vertices.end(),{
                p0.x,p0.y,p0.z,0.0f,0.0f,normal.x,normal.y,normal.z,
                p1.x,p1.y,p1.z,uMax,0.0f,normal.x,normal.y,normal.z,
                p2.x,p2.y,p2.z,uMax,vMax,normal.x,normal.y,normal.z,
                p3.x,p3.y,p3.z,0.0f,vMax,normal.x,normal.y,normal.z
            });
            indices.insert(indices.end(),{
                indexOffset,indexOffset+1,indexOffset+2,
                indexOffset+2,indexOffset+3,indexOffset
            });
            indexOffset+=4;
        };
        auto lengthU=[&](const glm::vec3&a,const glm::vec3&b){return glm::length(b-a)/tileScale;};
        float vRepeat=wallHeight/tileScale;
        for(auto &ws:walls){
            glm::vec3 s=ws.start,e=ws.end;
            glm::vec3 dir=e-s;
            float segLen = glm::length(dir);
            if(segLen<0.001f) continue;
            glm::vec3 dn=dir/segLen;
            glm::vec3 perp=glm::normalize(glm::vec3(-dn.z,0.0f,dn.x));
            float half=wallThickness*0.5f;
            glm::vec3 sb = glm::vec3(s.x, baseY, s.z);
            glm::vec3 eb = glm::vec3(e.x, baseY, e.z);
            glm::vec3 s1=sb+perp*half,s2=sb-perp*half,e1=eb+perp*half,e2=eb-perp*half;
            glm::vec3 s1t=s1+glm::vec3(0.0f,ws.height,0.0f),s2t=s2+glm::vec3(0.0f,ws.height,0.0f);
            glm::vec3 e1t=e1+glm::vec3(0.0f,ws.height,0.0f),e2t=e2+glm::vec3(0.0f,ws.height,0.0f);
            addQuad(s2,e2,e2t,s2t,-perp,lengthU(s2,e2),vRepeat);
            addQuad(e1,s1,s1t,e1t, perp,lengthU(e1,s1),vRepeat);
            addQuad(s1t,e1t,e2t,s2t,glm::vec3(0.0f,1.0f,0.0f),lengthU(s1t,e1t),wallThickness/tileScale);
            addQuad(s1,s2,s2t,s1t,-dn,wallThickness/tileScale,vRepeat);
            addQuad(e2,e1,e1t,e2t, dn,wallThickness/tileScale,vRepeat);
        }
        if (vao != 0) glDeleteVertexArrays(1, &vao);
        if (vbo != 0) glDeleteBuffers(1, &vbo);
        if (ebo != 0) glDeleteBuffers(1, &ebo);
        glGenVertexArrays(1,&vao);
        glGenBuffers(1,&vbo);
        glGenBuffers(1,&ebo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER,vbo);
        glBufferData(GL_ARRAY_BUFFER,(GLsizeiptr)(vertices.size()*sizeof(float)),vertices.data(),GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,(GLsizeiptr)(indices.size()*sizeof(unsigned int)),indices.data(),GL_STATIC_DRAW);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(3*sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(5*sizeof(float)));
        glEnableVertexAttribArray(2);
        glBindVertexArray(0);
        SDL_Surface* wallSurface=png::LoadPNG(win->util.getFilePath("data/ground.png").c_str());
        if(!wallSurface) throw mx::Exception("Failed to load wall texture");
        if (textureId != 0) glDeleteTextures(1, &textureId);
        glGenTextures(1,&textureId);
        glBindTexture(GL_TEXTURE_2D,textureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, wallSurface->w, wallSurface->h,
                    0, GL_RGBA, GL_UNSIGNED_BYTE, wallSurface->pixels);
        glBindTexture(GL_TEXTURE_2D, 0);
        SDL_FreeSurface(wallSurface);
        numIndices = indices.size();
    }
    void draw(gl::GLWindow *win) override { (void)win; }
    void event(gl::GLWindow *win, SDL_Event &e) override { (void)win; (void)e; }
    void draw(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos) {
        (void)cameraPos;
        wallShader.useProgram();
        glm::mat4 model = glm::mat4(1.0f);
        GLint modelLoc = glGetUniformLocation(wallShader.id(), "model");
        GLint viewLoc = glGetUniformLocation(wallShader.id(), "view");
        GLint projectionLoc = glGetUniformLocation(wallShader.id(), "projection");
        if (modelLoc >= 0) glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        if (viewLoc >= 0) glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        if (projectionLoc >= 0) glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureId);
        GLint texLoc = glGetUniformLocation(wallShader.id(), "wallTexture");
        if (texLoc >= 0) glUniform1i(texLoc, 0);
        GLboolean cullWasEnabled = glIsEnabled(GL_CULL_FACE);
        if (cullWasEnabled) glDisable(GL_CULL_FACE);
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, (GLsizei)numIndices, GL_UNSIGNED_INT, (void*)0);
        glBindVertexArray(0);
        if (cullWasEnabled) glEnable(GL_CULL_FACE);
    }
    bool checkCollision(const glm::vec3& position, float radius) const {
        const float halfThick = wallThickness_ * 0.5f;
        const float hitRadius = radius + halfThick;
        for (const auto& wall : walls) {
            glm::vec3 seg = wall.end - wall.start;
            float segLen = glm::length(seg);
            if (segLen < 0.0001f) continue;
            glm::vec3 segDir = seg / segLen;
            glm::vec3 toP = position - wall.start;
            float t = glm::dot(toP, segDir);
            t = glm::clamp(t, 0.0f, segLen);
            glm::vec3 closest = wall.start + segDir * t;
            closest.y = position.y;
            float d = glm::length(position - closest);
            if (d < hitRadius) return true;
        }
        return false;
    }
    bool resolveSphereAgainstWalls(glm::vec3 &pos, float radius, int iterations = 4) const {
        bool moved = false;
        const float halfThick = wallThickness_ * 0.5f;
        const float hitR = radius + halfThick;
        for (int it = 0; it < iterations; ++it) {
            bool anyHit = false;
            for (const auto &wall : walls) {
                glm::vec3 seg = wall.end - wall.start;
                float segLen = glm::length(seg);
                if (segLen < 0.0001f) continue;
                glm::vec3 segDir = seg / segLen;
                glm::vec3 toP = pos - wall.start;
                float t = glm::dot(toP, segDir);
                t = glm::clamp(t, 0.0f, segLen);
                glm::vec3 closest = wall.start + segDir * t;
                closest.y = pos.y;
                glm::vec3 delta = pos - closest;
                float d = glm::length(delta);
                if (d < hitR) {
                    glm::vec3 n;
                    if (d > 0.00001f) n = delta / d;
                    else n = glm::vec3(1.0f, 0.0f, 0.0f);
                    float push = (hitR - d) + 0.0005f;
                    pos += n * push;
                    anyHit = true;
                    moved = true;
                }
            }
            if (!anyHit) break;
        }
        return moved;
    }
    glm::vec3 getSafePointInCell(int cellX, int cellZ, float objectRadius, float y, float margin = 0.05f) const {
        float cellSize = mazeCellSize;
        float halfThick = wallThickness_ * 0.5f;
        float pad = objectRadius + halfThick + margin;
        float x0 = -size_ + cellX * cellSize;
        float z0 = -size_ + cellZ * cellSize;
        float x1 = x0 + cellSize;
        float z1 = z0 + cellSize;
        float px = (x0 + x1) * 0.5f;
        float pz = (z0 + z1) * 0.5f;
        px = glm::clamp(px, x0 + pad, x1 - pad);
        pz = glm::clamp(pz, z0 + pad, z1 - pad);
        return glm::vec3(px, y, pz);
    }
    glm::vec3 getSafeRandomPointInCell(int cellX, int cellZ, float objectRadius, float y, std::mt19937 &rng, float margin = 0.05f) const {
        float cellSize = mazeCellSize;
        float halfThick = wallThickness_ * 0.5f;
        float pad = objectRadius + halfThick + margin;
        float x0 = -size_ + cellX * cellSize;
        float z0 = -size_ + cellZ * cellSize;
        float x1 = x0 + cellSize;
        float z1 = z0 + cellSize;
        float minX = x0 + pad, maxX = x1 - pad;
        float minZ = z0 + pad, maxZ = z1 - pad;
        if (minX > maxX) { minX = maxX = (x0 + x1) * 0.5f; }
        if (minZ > maxZ) { minZ = maxZ = (z0 + z1) * 0.5f; }
        std::uniform_real_distribution<float> dx(minX, maxX);
        std::uniform_real_distribution<float> dz(minZ, maxZ);
        return glm::vec3(dx(rng), y, dz(rng));
    }
private:
    size_t numIndices = 0;
    float wallThickness_ = 0.5f;
    float size_ = 50.0f;
};
class Pillar : public gl::GLObject {
public:
    struct PillarInstance {
        glm::vec3 position;
        float radius;
        float height;
    };
    std::vector<PillarInstance> pillars;
    GLuint vao = 0, vbo = 0, ebo = 0;
    GLuint textureId = 0;
    gl::ShaderProgram pillarShader;
    Pillar() = default;
    ~Pillar() {
        if (vao != 0) glDeleteVertexArrays(1, &vao);
        if (vbo != 0) glDeleteBuffers(1, &vbo);
        if (ebo != 0) glDeleteBuffers(1, &ebo);
    }
    void load(gl::GLWindow *win) override {}
    void load(gl::GLWindow *win, const Wall &mazeWalls) {
#ifndef __EMSCRIPTEN__
        const char *vertexShader = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos;
            layout(location = 1) in vec2 aTexCoord;
            layout(location = 2) in vec3 aNormal;
            out vec2 TexCoord;
            out vec3 Normal;
            out vec3 FragPos;
            uniform mat4 model;
            uniform mat4 view;
            uniform mat4 projection;
            void main() {
                gl_Position = projection * view * model * vec4(aPos, 1.0);
                FragPos = vec3(model * vec4(aPos, 1.0));
                Normal = mat3(transpose(inverse(model))) * aNormal;
                TexCoord = aTexCoord;
            }
        )";
        const char *fragmentShader = R"(#version 330 core
            in vec2 TexCoord;
            out vec4 color;
            uniform sampler2D pillarTexture;
            uniform float time_f;
            uniform vec2 iResolution;
            void main() {
                vec2 uv = TexCoord;
                vec2 center = vec2(0.5, 0.5);
                vec2 offset = uv - center;
                float fractalFactor = sin(length(offset) * 10.0 + time_f * 2.0) * 0.1;
                vec2 fractalUV = uv + fractalFactor * normalize(offset);
                float grid = abs(sin(fractalUV.x * 50.0) * sin(fractalUV.y * 50.0));
                grid = step(0.7, grid);
                float angle = atan(offset.y, offset.x) + fractalFactor * sin(time_f);
                float radius = length(offset);
                vec2 swirlUV = center + radius * vec2(cos(angle), sin(angle));
                vec2 combinedUV = mix(swirlUV, fractalUV, 0.5);
                vec4 texColor = texture(pillarTexture, combinedUV);
                vec3 rainbow = vec3(0.5 + 0.5 * sin(time_f + texColor.r),
                                    0.5 + 0.5 * sin(time_f + texColor.g + 2.0),
                                    0.5 + 0.5 * sin(time_f + texColor.b + 4.0));
                vec3 finalColor = mix(texColor.rgb * rainbow, vec3(grid), 0.3);
                color = vec4(finalColor, texColor.a);
            }
        )";
#else
        const char *vertexShader = R"(#version 300 es
            precision highp float;
            layout(location = 0) in vec3 aPos;
            layout(location = 1) in vec2 aTexCoord;
            layout(location = 2) in vec3 aNormal;
            out vec2 TexCoord;
            out vec3 Normal;
            out vec3 FragPos;
            uniform mat4 model;
            uniform mat4 view;
            uniform mat4 projection;
            void main() {
                gl_Position = projection * view * model * vec4(aPos, 1.0);
                FragPos = vec3(model * vec4(aPos, 1.0));
                Normal = mat3(transpose(inverse(model))) * aNormal;
                TexCoord = aTexCoord;
            }
        )";
        const char *fragmentShader = R"(#version 300 es
            precision highp float;
            in vec2 TexCoord;
            out vec4 color;
            uniform sampler2D pillarTexture;
            uniform float time_f;
            uniform vec2 iResolution;
            void main() {
                vec2 uv = TexCoord;
                vec2 center = vec2(0.5, 0.5);
                vec2 offset = uv - center;
                float fractalFactor = sin(length(offset) * 10.0 + time_f * 2.0) * 0.1;
                vec2 fractalUV = uv + fractalFactor * normalize(offset);
                float grid = abs(sin(fractalUV.x * 50.0) * sin(fractalUV.y * 50.0));
                grid = step(0.7, grid);
                float angle = atan(offset.y, offset.x) + fractalFactor * sin(time_f);
                float radius = length(offset);
                vec2 swirlUV = center + radius * vec2(cos(angle), sin(angle));
                vec2 combinedUV = mix(swirlUV, fractalUV, 0.5);
                vec4 texColor = texture(pillarTexture, combinedUV);
                vec3 rainbow = vec3(0.5 + 0.5 * sin(time_f + texColor.r),
                                    0.5 + 0.5 * sin(time_f + texColor.g + 2.0),
                                    0.5 + 0.5 * sin(time_f + texColor.b + 4.0));
                vec3 finalColor = mix(texColor.rgb * rainbow, vec3(grid), 0.3);
                color = vec4(finalColor, texColor.a);
            }
        )";
#endif
    if(!pillarShader.loadProgramFromText(vertexShader, fragmentShader)) {
        throw mx::Exception("Failed to load pillar shader");
    }
    int segments = 16;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    float bottomCapScale = 1.5f;
    float baseDepth = -0.05f;  
    for (int i = 0; i <= segments; ++i) {
        float angle = (float)i / segments * 2.0f * M_PI;
        float x_bottom = cos(angle) * bottomCapScale;
        float z_bottom = sin(angle) * bottomCapScale;
        float x_top = cos(angle);
        float z_top = sin(angle);
        float u = (float)i / segments;
        vertices.insert(vertices.end(), {
            x_bottom, 0.0f, z_bottom,
            u, 0.0f,
            x_bottom, 0.0f, z_bottom
        });
        vertices.insert(vertices.end(), {
            x_top, 1.0f, z_top,
            u, 1.0f,
            x_top, 0.0f, z_top
        });
    }
    for (int i = 0; i < segments; ++i) {
        int current = i * 2;
        int next = (i + 1) * 2;
        indices.insert(indices.end(), {
            (unsigned int)current, (unsigned int)(current + 1), (unsigned int)next,
            (unsigned int)next, (unsigned int)(current + 1), (unsigned int)(next + 1)
        });
    }
    int bottomCenterIndex = vertices.size() / 8;
    vertices.insert(vertices.end(), {
        0.0f, baseDepth, 0.0f,     
        0.5f, 0.5f,
        0.0f, -1.0f, 0.0f
    });
    int bottomCapStart = vertices.size() / 8;
    for (int i = 0; i <= segments; ++i) {
        float angle = (float)i / segments * 2.0f * M_PI;
        float x = cos(angle) * bottomCapScale;
        float z = sin(angle) * bottomCapScale;
        vertices.insert(vertices.end(), {
            x, 0.0f, z,  
            0.5f + x * 0.5f / bottomCapScale, 0.5f + z * 0.5f / bottomCapScale,
            0.0f, -1.0f, 0.0f
        });
    }
    for (int i = 0; i < segments; ++i) {
        indices.insert(indices.end(), {
            (unsigned int)bottomCenterIndex,
            (unsigned int)(bottomCapStart + i + 1),
            (unsigned int)(bottomCapStart + i)
        });
    }
    int topCenterIndex = vertices.size() / 8;
    vertices.insert(vertices.end(), {
        0.0f, 1.0f, 0.0f,
        0.5f, 0.5f,
        0.0f, 1.0f, 0.0f
    });
    int topCapStart = vertices.size() / 8;
    for (int i = 0; i <= segments; ++i) {
        float angle = (float)i / segments * 2.0f * M_PI;
        float x = cos(angle);
        float z = sin(angle);
        vertices.insert(vertices.end(), {
            x, 1.0f, z,
            0.5f + x * 0.5f, 0.5f + z * 0.5f,
            0.0f, 1.0f, 0.0f
        });
    }
    for (int i = 0; i < segments; ++i) {
        indices.insert(indices.end(), {
            (unsigned int)topCenterIndex,
            (unsigned int)(topCapStart + i),
            (unsigned int)(topCapStart + i + 1)
        });
    }
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
    SDL_Surface* pillarSurface = png::LoadPNG(win->util.getFilePath("data/ground.png").c_str());
    if(!pillarSurface) {
        throw mx::Exception("Failed to load pillar texture");
    }
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pillarSurface->w, pillarSurface->h,
                0, GL_RGBA, GL_UNSIGNED_BYTE, pillarSurface->pixels);
    SDL_FreeSurface(pillarSurface);
    numIndices = indices.size();
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> radiusDist(0.5f, 1.5f);
    std::uniform_real_distribution<float> heightDist(3.0f, 6.0f);
    int numPillars = 15;
    int pillarPlacementAttempts = 0;
    const int maxAttempts = numPillars * 5;  
    for (int i = 0; i < numPillars && pillarPlacementAttempts < maxAttempts; ++pillarPlacementAttempts) {
        PillarInstance pillar;
        pillar.radius = radiusDist(gen);
        pillar.height = heightDist(gen);
        int cellX = gen() % mazeWalls.mazeGridX;
        int cellZ = gen() % mazeWalls.mazeGridZ;
        pillar.position = mazeWalls.getSafeRandomPointInCell(
            cellX,
            cellZ,
            pillar.radius,
            0.0f,
            gen,
            0.3f  
        );
        if (!mazeWalls.checkCollision(pillar.position, pillar.radius)) {
            pillars.push_back(pillar);
            ++i;
        }
    }
    std::cout << "Generated " << numPillars << " pillars with caps\n";
}
    void draw(gl::GLWindow *win) override {
    }
    void event(gl::GLWindow *win, SDL_Event &e) override {
    }
      void draw(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos) {
        pillarShader.useProgram();
        GLuint viewLoc = glGetUniformLocation(pillarShader.id(), "view");
        GLuint projectionLoc = glGetUniformLocation(pillarShader.id(), "projection");
        GLuint lightPosLoc = glGetUniformLocation(pillarShader.id(), "lightPos");
        GLuint viewPosLoc = glGetUniformLocation(pillarShader.id(), "viewPos");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3f(lightPosLoc, 0.0f, 15.0f, 0.0f);
        glUniform3f(viewPosLoc, cameraPos.x, cameraPos.y, cameraPos.z);
        glUniform1f(glGetUniformLocation(pillarShader.id(), "time_f"), SDL_GetTicks() / 1000.0f);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glUniform1i(glGetUniformLocation(pillarShader.id(), "pillarTexture"), 0);
        glBindVertexArray(vao);
        for (const auto& pillar : pillars) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, pillar.position);
            model = glm::scale(model, glm::vec3(pillar.radius, pillar.height, pillar.radius));
            GLuint modelLoc = glGetUniformLocation(pillarShader.id(), "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, 0);
        }
        glBindVertexArray(0);
    }
    bool checkCollision(const glm::vec3& position, float playerRadius) {
        for (const auto& pillar : pillars) {
            glm::vec2 playerPos2D(position.x, position.z);
            glm::vec2 pillarPos2D(pillar.position.x, pillar.position.z);
            float distance = glm::length(playerPos2D - pillarPos2D);
            if (distance < (pillar.radius + playerRadius)) {
                return true;
            }
        }
        return false;
    }
private:
    size_t numIndices = 0;
};
class Explosion {
public:
    struct Particle {
        glm::vec3 position;
        glm::vec3 velocity;
        glm::vec4 color;
        float lifetime;
        float maxLifetime;
        float size;
        bool active;
    };
    std::vector<Particle> particles;
    std::vector<uint32_t> freeList;
    GLuint vao = 0, vbo = 0;
    gl::ShaderProgram explosionShader;
    const size_t MAX_PARTICLES = 500000;
    bool poolReady = false;
    Explosion() = default;
    ~Explosion() {
        if (vao != 0) glDeleteVertexArrays(1, &vao);
        if (vbo != 0) glDeleteBuffers(1, &vbo);
    }
    void load(gl::GLWindow *win) {
#ifndef __EMSCRIPTEN__
        const char *vertexShader = R"(#version 330 core
            layout(location = 0) in vec3 aPos;
            layout(location = 1) in vec4 aColor;
            layout(location = 2) in float aSize;
            out vec4 particleColor;
            uniform mat4 view;
            uniform mat4 projection;
            void main() {
                gl_Position = projection * view * vec4(aPos, 1.0);
                gl_PointSize = aSize;
                particleColor = aColor;
            }
        )";
        const char *fragmentShader = R"(#version 330 core
            in vec4 particleColor;
            out vec4 FragColor;
            void main() {
                vec2 coord = gl_PointCoord - vec2(0.5);
                float dist = length(coord);
                if (dist > 0.5) discard;
                float fade = 1.0 - smoothstep(0.0, 0.5, dist);
                FragColor = vec4(particleColor.rgb, particleColor.a * fade);
            }
        )";
#else
        const char *vertexShader = R"(#version 300 es
            precision highp float;
            layout(location = 0) in vec3 aPos;
            layout(location = 1) in vec4 aColor;
            layout(location = 2) in float aSize;
            out vec4 particleColor;
            uniform mat4 view;
            uniform mat4 projection;
            void main() {
                gl_Position = projection * view * vec4(aPos, 1.0);
                gl_PointSize = aSize;
                particleColor = aColor;
            }
        )";
        const char *fragmentShader = R"(#version 300 es
            precision highp float;
            in vec4 particleColor;
            out vec4 FragColor;
            void main() {
                vec2 coord = gl_PointCoord - vec2(0.5);
                float dist = length(coord);
                if (dist > 0.5) discard;
                float fade = 1.0 - smoothstep(0.0, 0.5, dist);
                FragColor = vec4(particleColor.rgb, particleColor.a * fade);
            }
        )";
#endif
        if(!explosionShader.loadProgramFromText(vertexShader, fragmentShader)) {
            throw mx::Exception("Failed to load explosion shader");
        }
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * 8 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(7 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glBindVertexArray(0);
        particles.resize(MAX_PARTICLES);
        freeList.clear();
        freeList.reserve(MAX_PARTICLES);
        for (uint32_t i = 0; i < (uint32_t)MAX_PARTICLES; ++i) {
            particles[i].active = false;
            freeList.push_back((uint32_t)(MAX_PARTICLES - 1 - i));
        }
        poolReady = true;
        (void)win;
    }
    void createExplosion(const glm::vec3& position, int numParticles, bool isRed = false) {
        if (!poolReady || numParticles <= 0) return;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> speed(3.0f, 15.0f);
        std::uniform_real_distribution<float> angle(0.0f, 2.0f * M_PI);
        std::uniform_real_distribution<float> elevation(-M_PI/6.0f, M_PI/3.0f);
        std::uniform_real_distribution<float> colorVariation(0.7f, 1.0f);
        int created = 0;
        int canCreate = std::min(numParticles, (int)freeList.size());
        for (int i = 0; i < canCreate; ++i) {
            uint32_t idx = freeList.back();
            freeList.pop_back();
            Particle &p = particles[idx];
            p.position = position;
            float theta = angle(gen);
            float phi = elevation(gen);
            float v = speed(gen);
            p.velocity = glm::vec3(
                v * cos(phi) * cos(theta),
                v * sin(phi),
                v * cos(phi) * sin(theta)
            );
            if (isRed) {
                float r = colorVariation(gen);
                float g = colorVariation(gen) * 0.3f;
                float b = colorVariation(gen) * 0.1f;
                p.color = glm::vec4(r, g, b, 1.0f);
            } else {
                float r = colorVariation(gen);
                float g = colorVariation(gen) * 0.7f;
                float b = colorVariation(gen) * 0.2f;
                p.color = glm::vec4(r, g, b, 1.0f);
            }
            p.lifetime = 0.0f;
            p.maxLifetime = 0.55f;
            p.size = 12.0f + v * 3.0f;
            p.active = true;
            created++;
        }
        std::cout << "Explosion created at (" << position.x << ", " << position.y << ", " << position.z
                  << ") with " << created << " particles (free: " << freeList.size() << ")\n";
    }
    void draw(const glm::mat4& view, const glm::mat4& projection) {
        if (!poolReady) return;
#ifndef __EMSCRIPTEN__
        glEnable(GL_PROGRAM_POINT_SIZE);
#endif
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);
        explosionShader.useProgram();
        GLuint viewLoc = glGetUniformLocation(explosionShader.id(), "view");
        GLuint projectionLoc = glGetUniformLocation(explosionShader.id(), "projection");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        std::vector<float> vertexData;
        vertexData.reserve(200000 * 8);
        int drawCount = 0;
        for (const auto& particle : particles) {
            if (!particle.active) continue;
            vertexData.push_back(particle.position.x);
            vertexData.push_back(particle.position.y);
            vertexData.push_back(particle.position.z);
            vertexData.push_back(particle.color.r);
            vertexData.push_back(particle.color.g);
            vertexData.push_back(particle.color.b);
            vertexData.push_back(particle.color.a);
            vertexData.push_back(particle.size);
            drawCount++;
            if (drawCount >= 200000) break;
        }
        if (!vertexData.empty()) {
            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, vertexData.size() * sizeof(float), vertexData.data());
            glDrawArrays(GL_POINTS, 0, drawCount);
            glBindVertexArray(0);
        }
        glDepthMask(GL_TRUE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    void update(float deltaTime, Pillar& pillars, Wall& walls) {
        if (!poolReady) return;
        for (uint32_t i = 0; i < (uint32_t)particles.size(); ++i) {
            Particle &particle = particles[i];
            if (!particle.active) continue;
            particle.position += particle.velocity * deltaTime;
            particle.velocity.y -= 9.8f * deltaTime;
            for (const auto& pillar : pillars.pillars) {
                glm::vec2 particlePos2D(particle.position.x, particle.position.z);
                glm::vec2 pillarPos2D(pillar.position.x, pillar.position.z);
                float distance = glm::length(particlePos2D - pillarPos2D);
                if (distance < pillar.radius && particle.position.y > 0.0f && particle.position.y < pillar.height) {
                    glm::vec2 normal = glm::normalize(particlePos2D - pillarPos2D);
                    glm::vec2 velocity2D(particle.velocity.x, particle.velocity.z);
                    glm::vec2 reflected = velocity2D - 2.0f * glm::dot(velocity2D, normal) * normal;
                    particle.velocity.x = reflected.x * 0.5f;
                    particle.velocity.z = reflected.y * 0.5f;
                    glm::vec2 correction = normal * (pillar.radius - distance + 0.1f);
                    particle.position.x += correction.x;
                    particle.position.z += correction.y;
                }
            }
            for (const auto& wall : walls.walls) {
                glm::vec3 wallDir = wall.end - wall.start;
                glm::vec3 pointToStart = particle.position - wall.start;
                float wallLength = glm::length(wallDir);
                wallDir = glm::normalize(wallDir);
                float proj = glm::dot(pointToStart, wallDir);
                proj = glm::clamp(proj, 0.0f, wallLength);
                glm::vec3 closestPoint = wall.start + wallDir * proj;
                closestPoint.y = particle.position.y;
                float distance = glm::length(particle.position - closestPoint);
                if (distance < 0.5f && particle.position.y >= 0.0f && particle.position.y <= wall.height) {
                    glm::vec3 wallNormal = glm::normalize(particle.position - closestPoint);
                    glm::vec3 reflected = glm::reflect(particle.velocity, wallNormal);
                    particle.velocity = reflected * 0.5f;
                    particle.position += wallNormal * 0.2f;
                }
            }
            if (particle.position.y < 0.0f) {
                particle.position.y = 0.0f;
                particle.velocity.y = -particle.velocity.y * 0.3f;
                particle.velocity.x *= 0.8f;
                particle.velocity.z *= 0.8f;
            }
            particle.lifetime += deltaTime;
            float progress = particle.lifetime / particle.maxLifetime;
            particle.color.a = 1.0f - progress;
            particle.size *= 0.98f;
            if (particle.lifetime >= particle.maxLifetime || particle.color.a <= 0.001f) {
                particle.active = false;
                freeList.push_back(i);
            }
        }
    }
};
class Floor {
public:
    Floor() = default;
    ~Floor() {
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
    }
    size_t index_count = 0;
    void load(gl::GLWindow *win) {
#ifndef __EMSCRIPTEN__
        const char *vertexShader = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos;
            layout(location = 1) in vec2 aTexCoord;
            out vec2 TexCoord;
            uniform mat4 model;
            uniform mat4 view;
            uniform mat4 projection;
            void main() {
                gl_Position = projection * view * model * vec4(aPos, 1.0);
                TexCoord = aTexCoord;
            }
        )";
        const char *fragmentShader = R"(#version 330 core
            in vec2 TexCoord;
            out vec4 color;
            uniform sampler2D floorTexture;
            uniform float time_f;
            uniform vec2 iResolution;
            void main() {
                vec2 uv = TexCoord;
                vec2 center = vec2(0.5, 0.5);
                vec2 offset = uv - center;
                float fractalFactor = sin(length(offset) * 10.0 + time_f * 2.0) * 0.1;
                vec2 fractalUV = uv + fractalFactor * normalize(offset);
                float grid = abs(sin(fractalUV.x * 50.0) * sin(fractalUV.y * 50.0));
                grid = step(0.7, grid);
                float angle = atan(offset.y, offset.x) + fractalFactor * sin(time_f);
                float radius = length(offset);
                vec2 swirlUV = center + radius * vec2(cos(angle), sin(angle));
                vec2 combinedUV = mix(swirlUV, fractalUV, 0.5);
                vec4 texColor = texture(floorTexture, combinedUV);
                vec3 rainbow = vec3(0.5 + 0.5 * sin(time_f + texColor.r),
                                    0.5 + 0.5 * sin(time_f + texColor.g + 2.0),
                                    0.5 + 0.5 * sin(time_f + texColor.b + 4.0));
                vec3 finalColor = mix(texColor.rgb * rainbow, vec3(grid), 0.3);
                color = vec4(finalColor, texColor.a);
            }
  )"; 
#else
    const char *vertexShader = R"(#version 300 es
        precision highp float;
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec2 aTexCoord;
        out vec2 TexCoord;
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        void main() {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
            TexCoord = aTexCoord;
        }
    )";
    const char *fragmentShader = R"(#version 300 es
        precision highp float;
            in vec2 TexCoord;
            out vec4 color;
            uniform sampler2D floorTexture;
            uniform float time_f;
            uniform vec2 iResolution;
            void main() {
                vec2 uv = TexCoord;
                vec2 center = vec2(0.5, 0.5);
                vec2 offset = uv - center;
                float fractalFactor = sin(length(offset) * 10.0 + time_f * 2.0) * 0.1;
                vec2 fractalUV = uv + fractalFactor * normalize(offset);
                float grid = abs(sin(fractalUV.x * 50.0) * sin(fractalUV.y * 50.0));
                grid = step(0.7, grid);
                float angle = atan(offset.y, offset.x) + fractalFactor * sin(time_f);
                float radius = length(offset);
                vec2 swirlUV = center + radius * vec2(cos(angle), sin(angle));
                vec2 combinedUV = mix(swirlUV, fractalUV, 0.5);
                vec4 texColor = texture(floorTexture, combinedUV);
                vec3 rainbow = vec3(0.5 + 0.5 * sin(time_f + texColor.r),
                                    0.5 + 0.5 * sin(time_f + texColor.g + 2.0),
                                    0.5 + 0.5 * sin(time_f + texColor.b + 4.0));
                vec3 finalColor = mix(texColor.rgb * rainbow, vec3(grid), 0.3);
                color = vec4(finalColor, texColor.a);
            }
  )";
#endif
    if(floorShader.loadProgramFromText(vertexShader, fragmentShader) == false) {
        throw mx::Exception("Failed to load floor shader program");
    }
    const int N = 64;                
    const float half = 50.0f;
    const float y = -0.01f;
    const float uvTiles = 20.0f;
    std::vector<float> verts;
    std::vector<unsigned int> idx;
    verts.reserve((N+1)*(N+1)*5);
    idx.reserve(N*N*6);
    for (int z=0; z<=N; ++z) {
    float tz = float(z)/N;
    float Z = -half + tz * (2.0f*half);
        for (int x=0; x<=N; ++x) {
            float tx = float(x)/N;
            float X = -half + tx * (2.0f*half);
            float u = tx * uvTiles;
            float v = tz * uvTiles;
            verts.insert(verts.end(), { X, y, Z, u, v });
        }
    }
    auto vid = [&](int x,int z){ return z*(N+1)+x; };
    for (int z=0; z<N; ++z) {
        for (int x=0; x<N; ++x) {
            unsigned a = vid(x,   z);
            unsigned b = vid(x+1, z);
            unsigned c = vid(x+1, z+1);
            unsigned d = vid(x,   z+1);
            idx.insert(idx.end(), { a,b,c,  c,d,a });
        }
    }
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned int), idx.data(), GL_STATIC_DRAW);
    index_count = idx.size();
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    SDL_Surface* floorSurface = png::LoadPNG(win->util.getFilePath("data/ground.png").c_str());
    if(!floorSurface) {
        throw mx::Exception("Failed to load floor texture");
    }
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifndef __EMSCRIPTEN__
    #ifdef GL_EXT_texture_filter_anisotropic
    GLfloat maxAnisotropy;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
    #endif
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, floorSurface->w, floorSurface->h,
                0, GL_RGBA, GL_UNSIGNED_BYTE, floorSurface->pixels);
    glGenerateMipmap(GL_TEXTURE_2D); 
    SDL_FreeSurface(floorSurface);
}
    void update(float deltaTime) {
    }
    void draw(gl::GLWindow* win, const glm::mat4& view, const glm::mat4& projection) {
        glm::mat4 model(1.0f);
        floorShader.useProgram();
        glUniformMatrix4fv(glGetUniformLocation(floorShader.id(), "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(floorShader.id(), "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(floorShader.id(), "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform2f(glGetUniformLocation(floorShader.id(), "iResolution"), win->w, win->h);
        float time_f = SDL_GetTicks() / 1000.0f;
        floorShader.setUniform("time_f", time_f);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glUniform1i(glGetUniformLocation(floorShader.id(), "floorTexture"), 0);
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
    void setCameraPosition(const glm::vec3& pos) { cameraPos = pos; }
    void setCameraFront(const glm::vec3& front) { cameraFront = front; }
    glm::vec3 getCameraPosition() const { return cameraPos; }
    glm::vec3 getCameraFront() const { return cameraFront; }
private:
    gl::ShaderProgram floorShader;
    GLuint vao = 0, vbo = 0, ebo = 0;
    GLuint textureId = 0;
    glm::vec3 cameraPos = glm::vec3(0.0f, 1.7f, 3.0f); 
    glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f); 
};
class Objects {
public:
    static const int MAX_OBJECTS = 10;
    Objects() = default;
    ~Objects() {
    }
    mx::Model saturn;
    mx::Model bird;
    struct Object {
        mx::Model *model;
        glm::vec3 position;
        glm::vec3 rotation;
        glm::vec3 scale;
        float rotationSpeed;
        bool active;  
        float radius; 
    };
    std::vector<Object> objects;
    gl::ShaderProgram obj_shader;
    void load(gl::GLWindow *win, const Wall &mazeWalls) {
#ifndef __EMSCRIPTEN__
        const char *vertexShader = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos;
            layout(location = 1) in vec3 aNormal;
            layout(location = 2) in vec2 aTexCoord;
            out vec2 TexCoord;
            out vec3 Normal;
            out vec3 FragPos;
            uniform mat4 model;
            uniform mat4 view;
            uniform mat4 projection;
            void main() {
                gl_Position = projection * view * model * vec4(aPos, 1.0);
                FragPos = vec3(model * vec4(aPos, 1.0));
                Normal = mat3(transpose(inverse(model))) * aNormal;
                TexCoord = aTexCoord;
            }
        )";
        const char *fragmentShader = R"(
            #version 330 core
            out vec4 FragColor;
            in vec2 TexCoord;
            in vec3 Normal;
            in vec3 FragPos;
            uniform sampler2D objectTexture;
            uniform vec3 lightPos;
            uniform vec3 viewPos;
            uniform vec3 lightColor;
            void main() {
                float ambientStrength = 0.3;
                vec3 ambient = ambientStrength * lightColor;
                vec3 norm = normalize(Normal);
                vec3 lightDir = normalize(lightPos - FragPos);
                float diff = max(dot(norm, lightDir), 0.0);
                vec3 diffuse = diff * lightColor;
                float specularStrength = 0.5;
                vec3 viewDir = normalize(viewPos - FragPos);
                vec3 reflectDir = reflect(-lightDir, norm);
                float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
                vec3 specular = specularStrength * spec * lightColor;
                vec4 texColor = texture(objectTexture, TexCoord);
                vec3 result = (ambient + diffuse + specular) * vec3(texColor);
                FragColor = vec4(result, texColor.a);
            }
        )";
#else
        const char *vertexShader = R"(#version 300 es
            precision highp float;
            layout(location = 0) in vec3 aPos;
            layout(location = 1) in vec3 aNormal;
            layout(location = 2) in vec2 aTexCoord;
            out vec2 TexCoord;
            out vec3 Normal;
            out vec3 FragPos;
            uniform mat4 model;
            uniform mat4 view;
            uniform mat4 projection;
            void main() {
                gl_Position = projection * view * model * vec4(aPos, 1.0);
                FragPos = vec3(model * vec4(aPos, 1.0));
                Normal = mat3(transpose(inverse(model))) * aNormal;
                TexCoord = aTexCoord;
            }
        )";
        const char *fragmentShader = R"(#version 300 es
            precision highp float;
            out vec4 FragColor;
            in vec2 TexCoord;
            in vec3 Normal;
            in vec3 FragPos;
            uniform sampler2D objectTexture;
            uniform vec3 lightPos;
            uniform vec3 viewPos;
            uniform vec3 lightColor;
            void main() {
                float ambientStrength = 0.3;
                vec3 ambient = ambientStrength * lightColor;    
                vec3 norm = normalize(Normal);
                vec3 lightDir = normalize(lightPos - FragPos);
                float diff = max(dot(norm, lightDir), 0.0);
                vec3 diffuse = diff * lightColor;
                float specularStrength = 0.5;
                vec3 viewDir = normalize(viewPos - FragPos);
                vec3 reflectDir = reflect(-lightDir, norm);
                float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
                vec3 specular = specularStrength * spec * lightColor;
                vec4 texColor = texture(objectTexture, TexCoord);
                vec3 result = (ambient + diffuse + specular) * vec3(texColor);
                FragColor = vec4(result, texColor.a);
            }
        )";
#endif
        if(obj_shader.loadProgramFromText(vertexShader, fragmentShader) == false) {
            throw mx::Exception("Failed to load object shader program");
        }
        objects.resize(MAX_OBJECTS);
        std::vector<int> modelTypes(MAX_OBJECTS);
        for(int i = 0; i < MAX_OBJECTS / 2; i++) {
            modelTypes[i] = 0; 
        }
        for(int i = MAX_OBJECTS / 2; i < MAX_OBJECTS; i++) {
            modelTypes[i] = 1; 
        }
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(modelTypes.begin(), modelTypes.end(), g);
        if(!saturn.openModel(win->util.getFilePath("data/saturn.mxmod.z"))) {
            throw mx::Exception("Failed to load saturn model");
        }
        saturn.setTextures(win, win->util.getFilePath("data/planet.tex"), 
                                             win->util.getFilePath("data"));
        if(!bird.openModel(win->util.getFilePath("data/bird.mxmod.z"))) {
            throw mx::Exception("Failed to load bird model");
        }
        bird.setTextures(win, win->util.getFilePath("data/bird.tex"), 
                                        win->util.getFilePath("data"));
        std::mt19937 rng(std::random_device{}());
        for(int i = 0; i < MAX_OBJECTS; ++i) {
            objects[i].active = true; 
            if(modelTypes[i] == 0) { 
                objects[i].model = &saturn;  
                float scale = 0.4f + ((rand() % 1000) / 1000.0f) * 0.4f;
                objects[i].scale = glm::vec3(scale, scale, scale);
                objects[i].rotationSpeed = 5.0f + ((rand() % 1000) / 1000.0f) * 10.0f;
                objects[i].radius = 2.0f * scale; 
            } 
            else { 
                objects[i].model = &bird;    
                float scale = 0.3f + ((rand() % 1000) / 1000.0f) * 0.2f;
                objects[i].scale = glm::vec3(scale, scale, scale);
                objects[i].rotationSpeed = 20.0f + ((rand() % 1000) / 1000.0f) * 40.0f;
                objects[i].radius = 1.5f * scale; 
            }
            bool positionFound = false;
            int attemptCount = 0;
            const int maxAttempts = 20;
            while (!positionFound && attemptCount < maxAttempts) {
                int cellX = rng() % mazeWalls.mazeGridX;
                int cellZ = rng() % mazeWalls.mazeGridZ;
                float y = (modelTypes[i] == 1) ? 1.5f + (rand() % 100) / 100.0f : 2.5f;
                glm::vec3 position = mazeWalls.getSafeRandomPointInCell(
                    cellX, cellZ,
                    objects[i].radius,
                    y,
                    rng,
                    0.5f  
                );
                if (!mazeWalls.checkCollision(position, objects[i].radius)) {
                    objects[i].position = position;
                    positionFound = true;
                }
                attemptCount++;
            }
            if (!positionFound) {
                objects[i].position = glm::vec3(0.0f, (modelTypes[i] == 1) ? 1.5f : 2.5f, 0.0f);
            }
            objects[i].rotation = glm::vec3(0.0f, static_cast<float>(rand() % 360), 0.0f);
        }
    }
    bool checkCollision(const glm::vec3& point, float& hitObjectIndex) {
        for(size_t i = 0; i < objects.size(); ++i) {
            if (!objects[i].active) continue;
            float distance = glm::length(objects[i].position - point);
            if (distance < objects[i].radius) {
                hitObjectIndex = i;
                return true;
            }
        }
        return false;
    }
    void removeObject(int index) {
        if (index >= 0 && index < static_cast<int>(objects.size())) {
            objects[index].active = false;
            std::cout << "Object " << index << " destroyed!\n";
        }
    }
    int getActiveCount() const {
        int count = 0;
        for (const auto& obj : objects) {
            if (obj.active) count++;
        }
        return count;
    }
    void update(float deltaTime) {
        for(auto &obj : objects) {
            if (!obj.active) continue; 
            obj.rotation.y += obj.rotationSpeed * deltaTime;
            if(obj.rotation.y > 360.0f) {
                obj.rotation.y -= 360.0f;
            }
        }
    }
    void draw(gl::GLWindow *win, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos) {
        obj_shader.useProgram();
        GLuint viewLoc = glGetUniformLocation(obj_shader.id(), "view");
        GLuint projectionLoc = glGetUniformLocation(obj_shader.id(), "projection");
        GLuint lightPosLoc = glGetUniformLocation(obj_shader.id(), "lightPos");
        GLuint viewPosLoc = glGetUniformLocation(obj_shader.id(), "viewPos");
        GLuint lightColorLoc = glGetUniformLocation(obj_shader.id(), "lightColor");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3f(lightPosLoc, 0.0f, 15.0f, 0.0f);
        glUniform3f(viewPosLoc, cameraPos.x, cameraPos.y, cameraPos.z);
        glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);
        for(auto &obj : objects) {
            if (!obj.active) continue; 
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, obj.position);
            model = glm::rotate(model, glm::radians(obj.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(obj.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(obj.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, obj.scale);
            GLuint modelLoc = glGetUniformLocation(obj_shader.id(), "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            obj.model->setShaderProgram(&obj_shader, "objectTexture");
            obj.model->drawArrays();
        }
    }
};
class Projectile {
public:
    struct TrailPoint {
        glm::vec3 position;
        float lifetime;
        float maxLifetime;
    };
    struct Bullet {
        glm::vec3 position;
        glm::vec3 direction;
        float speed;
        float lifetime;
        float maxLifetime;
        bool active;
        std::vector<TrailPoint> trail;
        float trailTimer;
    };
    std::vector<Bullet> bullets;
    GLuint vao, vbo, ebo;
    GLuint trailVao, trailVbo;
    gl::ShaderProgram bulletShader;
    gl::ShaderProgram trailShader;
    Projectile() = default;
    ~Projectile() {
        if (vao != 0) glDeleteVertexArrays(1, &vao);
        if (vbo != 0) glDeleteBuffers(1, &vbo);
        if (ebo != 0) glDeleteBuffers(1, &ebo);
        if (trailVao != 0) glDeleteVertexArrays(1, &trailVao);
        if (trailVbo != 0) glDeleteBuffers(1, &trailVbo);
    }
    void load(gl::GLWindow *win) {
#ifndef __EMSCRIPTEN__
    const char *vertexShader = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        void main() {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";
    const char *fragmentShader = R"(
        #version 330 core
        out vec4 FragColor;
        uniform float alpha;
        void main() {
            FragColor = vec4(1.0, 0.1, 0.0, alpha);
        }
    )";
    const char *trailVertexShader = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in float aAlpha;
        out float trailAlpha;
        uniform mat4 view;
        uniform mat4 projection;
        void main() {
            gl_Position = projection * view * vec4(aPos, 1.0);
            gl_PointSize = 12.0;
            trailAlpha = aAlpha;
        }
    )";
    const char *trailFragmentShader = R"(
        #version 330 core
        in float trailAlpha;
        out vec4 FragColor;
        void main() {
            vec2 coord = gl_PointCoord - vec2(0.5);
            float dist = length(coord);
            if (dist > 0.5) discard;
            float fade = 1.0 - smoothstep(0.0, 0.5, dist);
            vec3 color = vec3(1.0, 0.2, 0.0);
            FragColor = vec4(color, trailAlpha * fade);
        }
    )";
#else
    const char *vertexShader = R"(#version 300 es
        precision highp float;
        layout(location = 0) in vec3 aPos;
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        void main() {
            gl_Position = projection * view * vec4(aPos, 1.0);
        }
    )";
    const char *fragmentShader = R"(#version 300 es
        precision highp float;
        out vec4 FragColor;
        uniform float alpha;
        void main() {
            FragColor = vec4(1.0, 0.1, 0.0, alpha);
        }
    )";
    const char *trailVertexShader = R"(#version 300 es
        precision highp float;
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in float aAlpha;
        out float trailAlpha;
        uniform mat4 view;
        uniform mat4 projection;
        void main() {
            gl_Position = projection * view * vec4(aPos, 1.0);
            gl_PointSize = 12.0;
            trailAlpha = aAlpha;
        }
    )";
    const char *trailFragmentShader = R"(#version 300 es
        precision highp float;
        in float trailAlpha;
        out vec4 FragColor;
        void main() {
            vec2 coord = gl_PointCoord - vec2(0.5);
            float dist = length(coord);
            if (dist > 0.5) discard;
            float fade = 1.0 - smoothstep(0.0, 0.5, dist);
            vec3 color = vec3(1.0, 0.2, 0.0);
            FragColor = vec4(color, trailAlpha * fade);
        }
    )";
#endif
        if(!bulletShader.loadProgramFromText(vertexShader, fragmentShader)) {
            throw mx::Exception("Failed to load bullet shader");
        }
        if(!trailShader.loadProgramFromText(trailVertexShader, trailFragmentShader)) {
            throw mx::Exception("Failed to load trail shader");
        }
    float length = 0.15f;  
    float width = 0.015f;  
    float vertices[] = {
        -width, -width, 0.0f,
         width, -width, 0.0f,
         width,  width, 0.0f,
        -width,  width, 0.0f,
        -width, -width, -length,
         width, -width, -length,
         width,  width, -length,
        -width,  width, -length
    };
    unsigned int indices[] = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
        3, 2, 6, 6, 7, 3,
        0, 1, 5, 5, 4, 0,
        1, 2, 6, 6, 5, 1,
        0, 3, 7, 7, 4, 0
    };
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    glGenVertexArrays(1, &trailVao);
    glGenBuffers(1, &trailVbo);
    glBindVertexArray(trailVao);
    glBindBuffer(GL_ARRAY_BUFFER, trailVbo);
    glBufferData(GL_ARRAY_BUFFER, 1000 * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    std::cout << "Laser projectile system initialized\n";
}
void fire(const glm::vec3& position, const glm::vec3& direction) {
    Bullet bullet;
    bullet.position = position;
    bullet.direction = glm::normalize(direction);
    bullet.speed = 100.0f;  
    bullet.lifetime = 0.0f;
    bullet.maxLifetime = 10.0f;
    bullet.active = true;
    bullet.trailTimer = 0.0f;
    bullets.push_back(bullet);
    std::cout << "Laser bolt fired from (" << position.x << ", " << position.y << ", " << position.z 
              << ") in direction (" << direction.x << ", " << direction.y << ", " << direction.z << ")\n";
}
void update(float deltaTime, Objects& objects, Explosion& explosion, Pillar& pillars, Wall &walls) {
    for (auto& bullet : bullets) {
        if (!bullet.active) continue;
        glm::vec3 oldPosition = bullet.position;
        bullet.position += bullet.direction * bullet.speed * deltaTime;
        bullet.lifetime += deltaTime;
        bullet.trailTimer += deltaTime;
        if (bullet.trailTimer >= 0.02f) {
            TrailPoint trailPoint;
            trailPoint.position = bullet.position;
            trailPoint.lifetime = 0.0f;
            trailPoint.maxLifetime = 0.5f;
            bullet.trail.push_back(trailPoint);
            bullet.trailTimer = 0.0f;
        }
        for (auto& point : bullet.trail) {
            point.lifetime += deltaTime;
        }
        bullet.trail.erase(
            std::remove_if(bullet.trail.begin(), bullet.trail.end(),
                [](const TrailPoint& p) { return p.lifetime >= p.maxLifetime; }),
            bullet.trail.end()
        );
        if (bullet.position.y <= 0.0f) {
            glm::vec3 impactPos = glm::vec3(bullet.position.x, 0.0f, bullet.position.z);
            explosion.createExplosion(impactPos, 1500, true); 
            bullet.active = false;
            std::cout << "Bullet #" << &bullet - &bullets[0] << " hit floor at (" 
                      << impactPos.x << ", " << impactPos.y << ", " << impactPos.z << ")\n";
            continue; 
        }
        bool hitWall = false;
        for (const auto& wall : walls.walls) {
            int steps = 5;
            for (int i = 0; i <= steps; ++i) {
                float t = static_cast<float>(i) / steps;
                glm::vec3 checkPos = oldPosition + (bullet.position - oldPosition) * t;
                glm::vec3 wallDir = wall.end - wall.start;
                glm::vec3 pointToStart = checkPos - wall.start;
                float wallLength = glm::length(wallDir);
                wallDir = glm::normalize(wallDir);
                float proj = glm::dot(pointToStart, wallDir);
                proj = glm::clamp(proj, 0.0f, wallLength);
                glm::vec3 closestPoint = wall.start + wallDir * proj;
                float distance = glm::length(glm::vec2(checkPos.x, checkPos.z) - glm::vec2(closestPoint.x, closestPoint.z));
                if (distance < 0.5f && checkPos.y >= 0.0f && checkPos.y <= wall.height) {
                    explosion.createExplosion(checkPos, 1500, true); 
                    bullet.active = false;
                    hitWall = true;
                    std::cout << "Bullet #" << &bullet - &bullets[0] << " hit wall at (" 
                            << checkPos.x << ", " << checkPos.y << ", " << checkPos.z << ")\n";
                    break;
                }
            }
            if (hitWall) break;
        }
        if (hitWall) continue;
        bool hitPillar = false;
        for (const auto& pillar : pillars.pillars) {
            int steps = 5;
            for (int i = 0; i <= steps; ++i) {
                float t = static_cast<float>(i) / steps;
                glm::vec3 checkPos = oldPosition + (bullet.position - oldPosition) * t;
                glm::vec2 bulletPos2D(checkPos.x, checkPos.z);
                glm::vec2 pillarPos2D(pillar.position.x, pillar.position.z);
                float distance = glm::length(bulletPos2D - pillarPos2D);
                if (distance < pillar.radius && checkPos.y > 0.0f && checkPos.y < pillar.height) {
                    explosion.createExplosion(checkPos, 1500, true); 
                    bullet.active = false;
                    hitPillar = true;
                    std::cout << "Bullet #" << &bullet - &bullets[0] << " hit pillar at (" 
                              << checkPos.x << ", " << checkPos.y << ", " << checkPos.z << ")\n";
                    break; 
                }
            }
            if (hitPillar) break; 
        }
        if (!bullet.active) continue; 
        int steps = 5;
        for (int i = 0; i <= steps; ++i) {
            float t = static_cast<float>(i) / steps;
            glm::vec3 checkPos = oldPosition + (bullet.position - oldPosition) * t;
            float hitIndex = -1;
            if (objects.checkCollision(checkPos, hitIndex)) {
                explosion.createExplosion(checkPos, 5000, false); 
                objects.removeObject(static_cast<int>(hitIndex));
                bullet.active = false;
                std::cout << "Bullet #" << &bullet - &bullets[0] << " hit object " 
                          << static_cast<int>(hitIndex) << "!\n";
                break; 
            }
        }
        if (!bullet.active) continue; 
        if (bullet.lifetime >= bullet.maxLifetime) {
            bullet.active = false;
            std::cout << "Bullet #" << &bullet - &bullets[0] << " dissolved after " 
                      << bullet.lifetime << " seconds\n";
        }
    }
    bullets.erase(
        std::remove_if(bullets.begin(), bullets.end(),
            [](const Bullet& b) { return !b.active; }),
        bullets.end()
    );
}
    void draw(gl::GLWindow *win, const glm::mat4& view, const glm::mat4& projection) {
        if (bullets.empty()) return;
#ifndef __EMSCRIPTEN__
        glEnable(GL_PROGRAM_POINT_SIZE);
#endif
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        trailShader.useProgram();
        GLuint viewLoc = glGetUniformLocation(trailShader.id(), "view");
        GLuint projectionLoc = glGetUniformLocation(trailShader.id(), "projection");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        for (const auto& bullet : bullets) {
            if (!bullet.active || bullet.trail.empty()) continue;
            std::vector<float> trailData;
            for (const auto& point : bullet.trail) {
                float fadeProgress = point.lifetime / point.maxLifetime;
                float alpha = (1.0f - fadeProgress) * 0.8f; 
                trailData.push_back(point.position.x);
                trailData.push_back(point.position.y);
                trailData.push_back(point.position.z);
                trailData.push_back(alpha);
            }
            if (!trailData.empty()) {
                glBindVertexArray(trailVao);
                glBindBuffer(GL_ARRAY_BUFFER, trailVbo);
                glBufferSubData(GL_ARRAY_BUFFER, 0, trailData.size() * sizeof(float), trailData.data());
                glDrawArrays(GL_POINTS, 0, bullet.trail.size());
            }
        }
        bulletShader.useProgram();
        viewLoc = glGetUniformLocation(bulletShader.id(), "view");
        projectionLoc = glGetUniformLocation(bulletShader.id(), "projection");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glBindVertexArray(vao);
        for (const auto& bullet : bullets) {
            if (!bullet.active) continue;
            if (bullet.lifetime < 0.05f) continue;
            float fadeProgress = bullet.lifetime / bullet.maxLifetime;
            float alpha = 1.0f - fadeProgress;
            glm::vec3 up = glm::vec3(0.0f,  1.0f, 0.0f);
            glm::vec3 right = glm::normalize(glm::cross(up, bullet.direction));
            if (glm::length(right) < 0.001f) {
                right = glm::vec3(1.0f, 0.0f, 0.0f);
            }
            up = glm::normalize(glm::cross(bullet.direction, right));
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, bullet.position);
            glm::mat4 rotation = glm::mat4(1.0f);
            rotation[0] = glm::vec4(right, 0.0f);
            rotation[1] = glm::vec4(up, 0.0f);
            rotation[2] = glm::vec4(bullet.direction, 0.0f);
            model = model * rotation;
            model = glm::scale(model, glm::vec3(1.5f, 1.5f, 3.0f)); 
            GLuint modelLoc = glGetUniformLocation(bulletShader.id(), "model");
            GLuint alphaLoc = glGetUniformLocation(bulletShader.id(), "alpha");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform1f(alphaLoc, alpha);
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        }
        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
};
class Crosshair {
public:
    GLuint vao, vbo;
    gl::ShaderProgram crosshairShader;
    Crosshair() = default;
    ~Crosshair() {
        if (vao != 0) glDeleteVertexArrays(1, &vao);
        if (vbo != 0) glDeleteBuffers(1, &vbo);
    }
    void load(gl::GLWindow *win) {
#ifndef __EMSCRIPTEN__
        const char *vertexShader = R"(
            #version 330 core
            layout(location = 0) in vec2 aPos;
            void main() {
                gl_Position = vec4(aPos, 0.0, 1.0);
            }
        )";
        const char *fragmentShader = R"(
            #version 330 core
            out vec4 FragColor;
            void main() {
                FragColor = vec4(1.0, 0.0, 0.0, 0.8);
            }
        )";
#else
        const char *vertexShader = R"(#version 300 es
            precision highp float;
            layout(location = 0) in vec2 aPos;
            void main() {
                gl_Position = vec4(aPos, 0.0, 1.0);
            }
        )";
        const char *fragmentShader = R"(#version 300 es
            precision highp float;
            out vec4 FragColor;
            void main() {
                FragColor = vec4(1.0, 0.0, 0.0, 0.8);
            }
        )";
#endif
        if(!crosshairShader.loadProgramFromText(vertexShader, fragmentShader)) {
            throw mx::Exception("Failed to load crosshair shader");
        }
        float size = 0.01f;
        float gap = 0.01f;
        float vertices[] = {
            -size - gap, 0.0f,
            -gap, 0.0f,
            gap, 0.0f,
            size + gap, 0.0f,
            0.0f, -size - gap,
            0.0f, -gap,
            0.0f, gap,
            0.0f, size + gap
        };
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }
    void draw() {
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        crosshairShader.useProgram();
        glBindVertexArray(vao);
        glLineWidth(6.0f);
        glDrawArrays(GL_LINES, 0, 8);
        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
    }
};
class Game : public gl::GLObject {
public:
    Game() = default;
    virtual ~Game() override {
        if (gameController) {
            SDL_GameControllerClose(gameController);
            gameController = nullptr;
        }
    }
    void load(gl::GLWindow *win) override {
        font.loadFont(win->util.getFilePath("data/font.ttf"), 20);
        game_floor.load(win);
        game_walls.load(win);
        game_floor.setCameraPosition(game_walls.getStartPosition());
        game_pillars.load(win, game_walls);  
        game_objects.load(win, game_walls);
        projectiles.load(win);
        explosion.load(win); 
        crosshair.load(win); 
        SDL_SetRelativeMouseMode(SDL_TRUE);
        lastX = win->w / 2.0f;
        lastY = win->h / 2.0f;
        yaw = -90.0f; 
        pitch = 0.0f;
        updateCameraVectors();
        for (int i = 0; i < SDL_NumJoysticks(); ++i) {
            if (SDL_IsGameController(i)) {
                gameController = SDL_GameControllerOpen(i);
                if (gameController) {
                    std::cout << "Game controller connected: " << SDL_GameControllerName(gameController) << "\n";
                    break;
                }
            }
        }
    }
    void updateCameraVectors() {
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFront = glm::normalize(front);
        game_floor.setCameraFront(cameraFront);
    }
    void draw(gl::GLWindow *win) override {
        glClearColor(0.392f, 0.710f, 0.965f, 1.0f);  
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST); 
        glDepthFunc(GL_LESS);
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; 
        lastUpdateTime = currentTime;
        update(deltaTime);
        glm::mat4 view = glm::lookAt(
            glm::vec3(game_floor.getCameraPosition().x, game_floor.getCameraPosition().y, game_floor.getCameraPosition().z),
            glm::vec3(game_floor.getCameraPosition().x + game_floor.getCameraFront().x, game_floor.getCameraPosition().y + game_floor.getCameraFront().y, game_floor.getCameraPosition().z + game_floor.getCameraFront().z),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        glm::mat4 projection = glm::perspective(glm::radians(45.0f),
                                static_cast<float>(win->w) / static_cast<float>(win->h),
                                0.1f, 1000.0f);
        game_floor.draw(win, view, projection);
        glEnable(GL_CULL_FACE);
        game_walls.draw(view, projection, game_floor.getCameraPosition());    
        glCullFace(GL_BACK);
        game_pillars.draw(view, projection, game_floor.getCameraPosition());  
        glDisable(GL_CULL_FACE);
        game_objects.draw(win, view, projection, game_floor.getCameraPosition());
        projectiles.draw(win, view, projection);
        explosion.draw(view, projection); 
        crosshair.draw(); 
        win->text.setColor({255, 255, 255, 255});
        win->text.printText_Solid(font, 25.0f, 25.0f, 
                               "3D Room - WASD/Left Stick move, Mouse/Right Stick look, Click/RB shoot, Back/Start quit");
        if (showFPS) {
            float fps = 1.0f / deltaTime;
            win->text.printText_Solid(font,   25.0f, 65.0f, 
                                  "FPS: " + std::to_string(static_cast<int>(fps)));
            win->text.printText_Solid(font, 25.0f, 95.0f, 
                                  "Active Bullets: " + std::to_string(projectiles.bullets.size()));
        }
    }
    void event(gl::GLWindow *win, SDL_Event &e) override {
        if (e.type == SDL_MOUSEMOTION && mouseCapture) {
            if (firstMouse) {
                firstMouse = false;
                return;
            }
            float xoffset = e.motion.xrel * mouseSensitivity;
            float yoffset = e.motion.yrel * mouseSensitivity;
            yaw += xoffset;
            pitch -= yoffset;
            if(pitch > 89.0f) pitch = 89.0f;
            if(pitch < -89.0f) pitch = -89.0f;
            updateCameraVectors();
        }
        else if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.sym == SDLK_ESCAPE) {
                mouseCapture = !mouseCapture;
                if (mouseCapture) {
                    SDL_SetRelativeMouseMode(SDL_TRUE);
                    SDL_WarpMouseInWindow(win->getWindow(), lastX, lastY);
                }
                else {
                    SDL_SetRelativeMouseMode(SDL_FALSE);
                }
            }
            else if (e.key.keysym.sym == SDLK_f) {
                showFPS = !showFPS;
            }
        }
        else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT && mouseCapture) {
            glm::vec3 firePosition = game_floor.getCameraPosition();
            glm::vec3 fireDirection = cameraFront;
            projectiles.fire(firePosition, fireDirection);
        }
        else if (e.type == SDL_CONTROLLERDEVICEADDED) {
            if (!gameController) {
                gameController = SDL_GameControllerOpen(e.cdevice.which);
                if (gameController) {
                    std::cout << "Game controller connected: " << SDL_GameControllerName(gameController) << "\n";
                }
            }
        }
        else if (e.type == SDL_CONTROLLERDEVICEREMOVED) {
            if (gameController && e.cdevice.which == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(gameController))) {
                SDL_GameControllerClose(gameController);
                gameController = nullptr;
                std::cout << "Game controller disconnected\n";
            }
        }
        else if (e.type == SDL_CONTROLLERBUTTONDOWN) {
            if (e.cbutton.button == SDL_CONTROLLER_BUTTON_BACK || e.cbutton.button == SDL_CONTROLLER_BUTTON_START) {
                win->quit();
            }
            else if (e.cbutton.button == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) {
                glm::vec3 firePosition = game_floor.getCameraPosition();
                glm::vec3 fireDirection = cameraFront;
                projectiles.fire(firePosition, fireDirection);
            }
            else if (e.cbutton.button == SDL_CONTROLLER_BUTTON_A) {
                glm::vec3 cPos = game_floor.getCameraPosition();
                if (cPos.y <= 1.71f) {
                    jumpVelocity = 0.3f;
                }
            }
        }
    }
    void update(float deltaTime) {
        this->deltaTime = deltaTime;
        if (gameController) {
            int rightX = SDL_GameControllerGetAxis(gameController, SDL_CONTROLLER_AXIS_RIGHTX);
            int rightY = SDL_GameControllerGetAxis(gameController, SDL_CONTROLLER_AXIS_RIGHTY);
            if (abs(rightX) > STICK_DEAD_ZONE || abs(rightY) > STICK_DEAD_ZONE) {
                float normRX = (abs(rightX) > STICK_DEAD_ZONE) ? rightX / 32768.0f : 0.0f;
                float normRY = (abs(rightY) > STICK_DEAD_ZONE) ? rightY / 32768.0f : 0.0f;
                yaw += normRX * controllerLookSensitivity;
                pitch -= normRY * controllerLookSensitivity;
                if (pitch > 89.0f) pitch = 89.0f;
                if (pitch < -89.0f) pitch = -89.0f;
                updateCameraVectors();
            }
        }
        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        glm::vec3 cameraPos = game_floor.getCameraPosition();
        glm::vec3 cameraFront = game_floor.getCameraFront();
        const float cameraSpeed = 0.1f;
        const float minHeight = 1.7f;
        const float playerRadius = 0.5f;
        glm::vec3 horizontalFront = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));
        glm::vec3 cameraRight = glm::normalize(glm::cross(horizontalFront, glm::vec3(0.0f, 1.0f, 0.0f)));
        isSprinting = keys[SDL_SCANCODE_LSHIFT];
        if (gameController && SDL_GameControllerGetButton(gameController, SDL_CONTROLLER_BUTTON_LEFTSTICK)) {
            isSprinting = true;
        }
        isCrouching = keys[SDL_SCANCODE_LCTRL];
        glm::vec3 newPos = cameraPos;
        if (keys[SDL_SCANCODE_W]) {
            newPos += (isSprinting ? cameraSpeed * 2.0f : cameraSpeed) * horizontalFront;
        }
        if (keys[SDL_SCANCODE_S]) {
            newPos -= cameraSpeed * horizontalFront;
        }
        if (keys[SDL_SCANCODE_A]) {
            newPos -= cameraSpeed * cameraRight;
        }
        if (keys[SDL_SCANCODE_D]) {
            newPos += cameraSpeed * cameraRight;
        }
        if (gameController) {
            int leftX = SDL_GameControllerGetAxis(gameController, SDL_CONTROLLER_AXIS_LEFTX);
            int leftY = SDL_GameControllerGetAxis(gameController, SDL_CONTROLLER_AXIS_LEFTY);
            float stickSpeed = isSprinting ? cameraSpeed * 2.0f : cameraSpeed;
            if (abs(leftX) > STICK_DEAD_ZONE) {
                float normX = leftX / 32768.0f;
                newPos += stickSpeed * normX * cameraRight;
            }
            if (abs(leftY) > STICK_DEAD_ZONE) {
                float normY = leftY / 32768.0f;
                newPos -= stickSpeed * normY * horizontalFront;
            }
        }
        if (!game_walls.checkCollision(newPos, playerRadius) && 
            !game_pillars.checkCollision(newPos, playerRadius)) {
            cameraPos = newPos;
        }
        if (keys[SDL_SCANCODE_SPACE]) {
            if (cameraPos.y <= minHeight + 0.01f) {
                jumpVelocity = 0.3f;
            }
        }
        cameraPos.y += jumpVelocity * deltaTime * 60.0f;
        jumpVelocity -= gravity * deltaTime * 60.0f;
        float currentMinHeight = isCrouching ? 0.8f : minHeight;
        if (cameraPos.y < currentMinHeight) {
            cameraPos.y = currentMinHeight;
            jumpVelocity = 0.0f;
        }
        game_floor.setCameraPosition(cameraPos);
        game_floor.update(deltaTime);
        game_objects.update(deltaTime);
        projectiles.update(deltaTime, game_objects, explosion, game_pillars, game_walls); 
        explosion.update(deltaTime, game_pillars, game_walls); 
    }
private:
    mx::Font font;
    Uint32 lastUpdateTime = SDL_GetTicks();
    Floor game_floor;
    Wall game_walls;       
    Pillar game_pillars;   
    Objects game_objects;
    Projectile projectiles;
    Explosion explosion; 
    Crosshair crosshair;
    glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    float lastX = 0.0f, lastY = 0.0f;
    float yaw = -90.0f; 
    float pitch = 0.0f;
    bool mouseCapture = true;
    bool firstMouse = true;
    float mouseSensitivity = 0.15f;
    bool showFPS = true;
    float deltaTime =  0.0f;
    float jumpVelocity = 0.0f;  
    const float gravity = 0.015f;
    bool isCrouching = false;  
    bool isSprinting = false;
    SDL_GameController* gameController = nullptr;
    static constexpr int STICK_DEAD_ZONE = 8000;
    float controllerLookSensitivity = 2.0f;
};
class MainWindow : public gl::GLWindow {
public:
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("FPS Maze Room - OpenGL ES3/WebGL 2.0", tw, th, true) {
        setPath(path);
        setObject(new Game());
        object->load(this);
    }
    ~MainWindow() override {}
    virtual void event(SDL_Event &e) override {}
    virtual void draw() override {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glViewport(0, 0, w, h);
        object->draw(this);
        swap();
        delay();
    }
};
MainWindow *main_w = nullptr;
void eventProc() {
    main_w->proc();
}
int main(int argc, char **argv) {
#ifdef __EMSCRIPTEN__
    try {
        MainWindow main_window("", 1920, 1080);
        main_w =&main_window;
        emscripten_set_main_loop(eventProc, 0, 1);
    } catch(const mx::Exception &e) {
        mx::system_err << "mx: Exception: " << e.text() << "\n";
        mx::system_err.flush();
        exit(EXIT_FAILURE);
    }
#else
    Arguments args = proc_args(argc, argv);
    try {
        MainWindow main_window(args.path, args.width, args.height);
        main_window.setFullScreen(true);
        main_window.loop();
    } catch(const mx::Exception &e) {
        mx::system_err << "mx: Exception: " << e.text() << "\n";
        mx::system_err.flush();
        exit(EXIT_FAILURE);
    }
#endif
    return 0;
}
