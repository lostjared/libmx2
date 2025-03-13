#include"mx.hpp"
#include"argz.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <GLES3/gl3.h>
#endif

#include"gl.hpp"
#include"loadpng.hpp"


#define CHECK_GL_ERROR() \
{ GLenum err = glGetError(); \
if (err != GL_NO_ERROR) \
printf("OpenGL Error: %d at %s:%d\n", err, __FILE__, __LINE__); }

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class Matrix3D : public gl::GLObject {
public:
    Matrix3D() = default;
    ~Matrix3D() override {
        release();
    }

    void load(gl::GLWindow *win) override {
        font.loadFont(win->util.getFilePath("data/keifont.ttf"), 16);
        cameraPosition = glm::vec3(0.0f, 0.0f, 0.0f); 
        cameraTarget = glm::vec3(0.0f, 0.0f, -1.0f);
        cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
        
        cameraDistance = 15.0f;
        cameraRotationY = 0.0f;
        cameraRotationX = 0.0f;
        
        insideMatrix = true;
        
        createTextureAtlas(win);
#ifndef __EMSCRIPTEN__
        const char *vertexShaderSource = R"(#version 330 core
            layout (location = 0) in vec3 aPos;
            layout (location = 1) in vec2 aTexCoord;
            layout (location = 2) in vec4 aColor;
            layout (location = 3) in vec2 aCharTexCoord;
            
            out vec2 TexCoord;
            out vec4 Color;
            
            uniform mat4 model;
            uniform mat4 view;
            uniform mat4 projection;
            
            void main() {
                gl_Position = projection * view * model * vec4(aPos, 1.0);
                TexCoord = aTexCoord + aCharTexCoord;
                Color = aColor;
            }
        )";
        
        const char *fragmentShaderSource = R"(#version 330 core
            in vec2 TexCoord;
            in vec4 Color;
            
            out vec4 FragColor;
            
            uniform sampler2D fontTexture;
            
            void main() {
                vec4 texColor = texture(fontTexture, TexCoord);
                
                float glowIntensity = texColor.r * 2.0;  // Double brightness
                
                vec3 matrixGreen = vec3(Color.r * 0.4, Color.g * 2.0, Color.b * 0.4);
                
                FragColor = vec4(matrixGreen, glowIntensity * Color.a * 2.0);
            }
        )";
    #else
    const char *vertexShaderSource = R"(#version 300 es
        precision highp float;
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec2 aTexCoord;
        layout (location = 2) in vec4 aColor;
        layout (location = 3) in vec2 aCharTexCoord;
        
        out vec2 TexCoord;
        out vec4 Color;
        
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        
        void main() {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
            TexCoord = aTexCoord + aCharTexCoord;
            Color = aColor;
        }
    )";
    
    const char *fragmentShaderSource = R"(#version 300 es
        precision highp float;
        in vec2 TexCoord;
        in vec4 Color;
        
        out vec4 FragColor;
        
        uniform sampler2D fontTexture;
        
        void main() {
            vec4 texColor = texture(fontTexture, TexCoord);
            
            float glowIntensity = texColor.r * 2.0;  // Double brightness
            
            vec3 matrixGreen = vec3(Color.r * 0.4, Color.g * 2.0, Color.b * 0.4);
            
            FragColor = vec4(matrixGreen, glowIntensity * Color.a * 2.0);
        }
    )";
    #endif
        
        if (!shader.loadProgramFromText(vertexShaderSource, fragmentShaderSource)) {
            throw mx::Exception("Failed to load matrix shader");
        }
        
        initMatrix3DData(win->w, win->h);
    }

    void draw(gl::GLWindow *win) override {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f;
        lastUpdateTime = currentTime;
        
        
        glEnable(GL_DEPTH_TEST);
        
        updateMatrix3DData(deltaTime, win->w, win->h);
        drawMatrix3D(win);
        win->text.setColor({0, 255, 0, 255});
        int fps = deltaTime > 0 ? (int)(1.0f / deltaTime) : 0;
        win->text.printText_Solid(font, 10.0f, 10.0f, "FPS: " + std::to_string(fps));
        

        win->text.setColor({255, 255, 255, 255});
        std::string cameraInfo = "Rotation: " + std::to_string(cameraRotationY) + 
                               ", Zoom: " + std::to_string(cameraDistance);
        win->text.printText_Solid(font, 10.0f, 40.0f, cameraInfo);
        win->text.printText_Solid(font, 10.0f, 70.0f, "Use Arrow Keys to Navigate");
    }
    
    void event(gl::GLWindow *win, SDL_Event &e) override {
        const float moveSpeed = 1.0f;
        
        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_LEFT:
                    if (insideMatrix) {
                        glm::vec3 dir = glm::normalize(cameraTarget - cameraPosition);
                        glm::mat4 rot = glm::rotate(glm::mat4(1.0f), 0.1f, glm::vec3(0, 1, 0));
                        glm::vec4 rotated = rot * glm::vec4(dir, 0.0f);
                        cameraTarget = cameraPosition + glm::vec3(rotated);
                    } else {
                        cameraRotationY -= 0.1f;
                        updateCameraPosition();
                    }
                    break;
                case SDLK_RIGHT:
                    if (insideMatrix) {
                        glm::vec3 dir = glm::normalize(cameraTarget - cameraPosition);
                        glm::mat4 rot = glm::rotate(glm::mat4(1.0f), -0.1f, glm::vec3(0, 1, 0));
                        glm::vec4 rotated = rot * glm::vec4(dir, 0.0f);
                        cameraTarget = cameraPosition + glm::vec3(rotated);
                    } else {
                        cameraRotationY += 0.1f;
                        updateCameraPosition();
                    }
                    break;
                case SDLK_w:
                    if (insideMatrix) {
                        glm::vec3 dir = glm::normalize(cameraTarget - cameraPosition);
                        cameraPosition += dir * moveSpeed;
                        cameraTarget += dir * moveSpeed;
                    } else {
                        cameraRotationX = std::min(0.8f, cameraRotationX + 0.1f);
                        updateCameraPosition();
                    }
                    break;
                case SDLK_s:
                    if (insideMatrix) {
                        glm::vec3 dir = glm::normalize(cameraTarget - cameraPosition);
                        cameraPosition -= dir * moveSpeed;
                        cameraTarget -= dir * moveSpeed;
                    } else {
                        cameraRotationX = std::max(-0.8f, cameraRotationX - 0.1f);
                        updateCameraPosition();
                    }
                    break;
                case SDLK_a:
                    if (insideMatrix) {
                        glm::vec3 dir = glm::normalize(cameraTarget - cameraPosition);
                        glm::vec3 right = glm::cross(glm::vec3(0, 1, 0), dir);
                        cameraPosition += right * moveSpeed;
                        cameraTarget += right * moveSpeed;
                    }
                    break;
                case SDLK_d:
                    if (insideMatrix) {
                        glm::vec3 dir = glm::normalize(cameraTarget - cameraPosition);
                        glm::vec3 right = glm::cross(glm::vec3(0, 1, 0), dir);
                        cameraPosition -= right * moveSpeed;
                        cameraTarget -= right * moveSpeed;
                    }
                    break;
                case SDLK_SPACE:
                    insideMatrix = !insideMatrix;
                    if (!insideMatrix) {
                        updateCameraPosition();
                    }
                    break;
            }
        }
    }

private:
    mx::Font font;
    gl::ShaderProgram shader;
    GLuint vao = 0, vbo = 0;
    GLuint fontTextureId = 0;
    Uint32 lastUpdateTime = SDL_GetTicks();
    glm::vec3 cameraPosition;
    glm::vec3 cameraTarget;
    glm::vec3 cameraUp;
    float cameraDistance;
    float cameraRotationY;
    float cameraRotationX;
    bool insideMatrix; 

    struct Char3DData {
        glm::vec3 position;  
        float speed;         
        int codepoint;       
        float opacity;       
        int trailIndex;      
        bool active;         
    };
    
    
    int gridSizeX = 40;  
    int gridSizeY = 40;  
    int gridSizeZ = 40;  
    float gridSpacing = 0.8f;  
    std::vector<Char3DData> characters;
    std::vector<int> columnHeads;  
    std::vector<int> trailLengths; 
    
    struct CharInfo {
        int codepoint;
        glm::vec2 texOffset;
        glm::vec2 texSize;
    };
    
    std::vector<CharInfo> charAtlas;
    int atlasSize;
    float charWidth, charHeight;
    int numColumns, numRows;
    
    
    std::vector<std::pair<int, int>> codepointRanges = {
        {0x3040, 0x309F},  
        {0x30A0, 0x30FF},  
        {0x4E00, 0x4FFF}   
    };
    
    void updateCameraPosition() {
        float x = cameraDistance * sin(cameraRotationY) * cos(cameraRotationX);
        float y = cameraDistance * sin(cameraRotationX);
        float z = cameraDistance * cos(cameraRotationY) * cos(cameraRotationX);
        cameraPosition = glm::vec3(x, y, z);
        cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f); 
        cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);     
    }
    
    void release() {
        if (vbo != 0) {
            glDeleteBuffers(1, &vbo);
            vbo = 0;
        }
        if (vao != 0) {
            glDeleteVertexArrays(1, &vao);
            vao = 0;
        }
        if (fontTextureId != 0) {
            glDeleteTextures(1, &fontTextureId);
            fontTextureId = 0;
        }
    }
    
    int getRandomCodepoint() {
        int rangeIndex = rand() % codepointRanges.size();
        int start = codepointRanges[rangeIndex].first;
        int end = codepointRanges[rangeIndex].second;
        return start + rand() % (end - start + 1);
    }
    
    std::string unicodeToUTF8(int codepoint) {
        std::string utf8;
        if (codepoint <= 0x7F) {
            utf8 += static_cast<char>(codepoint);
        } else if (codepoint <= 0x7FF) {
            utf8 += static_cast<char>((codepoint >> 6) | 0xC0);
            utf8 += static_cast<char>((codepoint & 0x3F) | 0x80);
        } else if (codepoint <= 0xFFFF) {
            utf8 += static_cast<char>((codepoint >> 12) | 0xE0);
            utf8 += static_cast<char>(((codepoint >> 6) & 0x3F) | 0x80);
            utf8 += static_cast<char>((codepoint & 0x3F) | 0x80);
        } else if (codepoint <= 0x10FFFF) {
            utf8 += static_cast<char>((codepoint >> 18) | 0xF0);
            utf8 += static_cast<char>(((codepoint >> 12) & 0x3F) | 0x80);
            utf8 += static_cast<char>(((codepoint >> 6) & 0x3F) | 0x80);
            utf8 += static_cast<char>((codepoint & 0x3F) | 0x80);
        }
        return utf8;
    }
    
    void createTextureAtlas(gl::GLWindow *win) {
        std::vector<int> charCodes;
        for (auto &range : codepointRanges) {
            for (int code = range.first; code <= range.second; ++code) {
                charCodes.push_back(code);
            }
        }
        
        atlasSize = (int)ceil(sqrt(charCodes.size() + 1));
        int texSize = atlasSize * 64;  
        SDL_Surface *atlasSurface = SDL_CreateRGBSurface(0, texSize, texSize, 32, 
                                                       0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
        SDL_FillRect(atlasSurface, NULL, 0x00000000);
        
        charAtlas.clear();
        int x = 0, y = 0;
        for (int code : charCodes) {
            std::string utf8Char = unicodeToUTF8(code);
            SDL_Surface *charSurface = TTF_RenderUTF8_Blended(font.wrapper().unwrap(), 
                                                           utf8Char.c_str(), {255, 255, 255, 255});
            if (!charSurface) continue;
            CharInfo info;
            info.codepoint = code;
            info.texOffset = glm::vec2((float)x / texSize, (float)y / texSize);
            info.texSize = glm::vec2((float)charSurface->w / texSize, (float)charSurface->h / texSize);
            charAtlas.push_back(info);
            SDL_Rect dstRect = {x, y, charSurface->w, charSurface->h};
            SDL_BlitSurface(charSurface, NULL, atlasSurface, &dstRect);
            SDL_FreeSurface(charSurface);
            
            x += 32;
            if (x + 32 > texSize) {
                x = 0;
                y += 32;
            }
        }
        
        glGenTextures(1, &fontTextureId);
        glBindTexture(GL_TEXTURE_2D, fontTextureId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, atlasSurface->w, atlasSurface->h, 
                    0, GL_RGBA, GL_UNSIGNED_BYTE, atlasSurface->pixels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        SDL_FreeSurface(atlasSurface);
        
        
        int cellSize = 24; 
        
        charWidth = static_cast<float>(cellSize);
        charHeight = static_cast<float>(cellSize);
        std::cout << "Character cell size: " << charWidth << "x" << charHeight << std::endl;
    }
    
    void initMatrix3DData(int screenWidth, int screenHeight) {
        int totalColumns = gridSizeX * gridSizeZ; 
        columnHeads.resize(totalColumns, -1);
        trailLengths.resize(totalColumns);
        
        
        int maxChars = totalColumns * gridSizeY * 15; 
        characters.resize(maxChars);
        
        for (int i = 0; i < maxChars; ++i) {
            characters[i].active = false;
        }
        
        
        for (int x = 0; x < gridSizeX; ++x) {
            for (int z = 0; z < gridSizeZ; ++z) {
                int columnIndex = x * gridSizeZ + z;
                trailLengths[columnIndex] = rand() % 30 + 15; 
                float startY = (rand() % (2 * gridSizeY)) * gridSpacing - gridSizeY * gridSpacing;
                addColumnChars3D(x, z, startY);
            }
        }
        
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
    
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, maxChars * 6 * 11 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(5 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(9 * sizeof(float)));
        glEnableVertexAttribArray(3);
    }
    
    void addColumnChars3D(int x, int z, float startY) {
        int columnIndex = x * gridSizeZ + z;
        int trailLength = trailLengths[columnIndex];
        int firstCharIndex = -1;
        
        float xPos = (x - gridSizeX/2.0f) * gridSpacing;
        float zPos = (z - gridSizeZ/2.0f) * gridSpacing;
        int i = 0;
        for (i = 0; i < static_cast<int>(characters.size()); ++i) {
            if (!characters[i].active) {
                if (firstCharIndex == -1) {
                    firstCharIndex = i;
                }
                
                characters[i].position = glm::vec3(xPos, startY - (i - firstCharIndex) * 0.4f, zPos);
                characters[i].speed = (rand() % 150 + 50) / 100.0f;  
                characters[i].codepoint = getRandomCodepoint();
                characters[i].trailIndex = i - firstCharIndex;
                            float trailFactor = (float)(i - firstCharIndex) / trailLength;
                characters[i].opacity = 1.0f - powf(trailFactor, 0.7f);  
                characters[i].active = true;
                
                if (i - firstCharIndex >= trailLength - 1) {
                    columnHeads[columnIndex] = firstCharIndex;
                    break;
                }
            }
        }
        if (firstCharIndex == -1 || i - firstCharIndex < trailLength - 1) {
            columnHeads[columnIndex] = -1;
        }
    }
    
    void updateMatrix3DData(float deltaTime, int screenWidth, int screenHeight) {
        int totalColumns = gridSizeX * gridSizeZ;
        
        
        static int refreshCounter = 0;
        refreshCounter++;
        
        
        if (refreshCounter > 120) {
            refreshCounter = 0;
            for (int columnIndex = 0; columnIndex < totalColumns; ++columnIndex) {
                if (columnHeads[columnIndex] < 0) {
                    int x = columnIndex / gridSizeZ;
                    int z = columnIndex % gridSizeZ;
                    trailLengths[columnIndex] = rand() % 10 + 10; 
                    float startY = gridSizeY * gridSpacing * 1.1f;
                    addColumnChars3D(x, z, startY);
                }
            }
        }
        
        for (int columnIndex = 0; columnIndex < totalColumns; ++columnIndex) {
            if (columnHeads[columnIndex] < 0) continue;
            
            int head = columnHeads[columnIndex];
            float speed = characters[head].speed;
            int trailLength = trailLengths[columnIndex];
            for (int i = 0; i < trailLength; ++i) {
                int idx = head - i;
                if (idx < 0 || idx >= static_cast<int>(characters.size())) continue;
                if (!characters[idx].active) continue;
                
                
                characters[idx].position.y -= speed * deltaTime * 1.2f;
                int changeFrequency;
                if (i == 0) {
                    changeFrequency = 5; 
                } else if (i < 3) {
                    changeFrequency = 15;
                } else {
                    changeFrequency = 30;
                }
                
                if (rand() % changeFrequency == 0) {
                    characters[idx].codepoint = getRandomCodepoint();
                }
            }
            
            
            if (characters[head].position.y < -gridSizeY * gridSpacing) {
            
                for (int i = 0; i < trailLength; ++i) {
                    int idx = head - i;
                    if (idx < 0 || idx >= static_cast<int>(characters.size())) continue;
                    characters[idx].active = false;
                }
                
            
                int x = columnIndex / gridSizeZ;
                int z = columnIndex % gridSizeZ;
                
                trailLengths[columnIndex] = rand() % 20 + 15;  
                float startY = gridSizeY * gridSpacing * 1.1f; 
                
                
                if (rand() % 10 > 7) { 
                    addColumnChars3D(x, z, startY);
                } else {
                    addColumnChars3D(x, z, startY);
                }
            }
        }
    }
    
    void drawMatrix3D(gl::GLWindow *win) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE); //GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        glEnable(GL_DEPTH_TEST);
        
        
        shader.useProgram();
        
        
        glm::mat4 projection = glm::perspective(glm::radians(70.0f), (float)win->w / (float)win->h, 0.1f, 100.0f);
        GLint projectionLoc = glGetUniformLocation(shader.id(), "projection");
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        
        
        glm::mat4 view;
        if (insideMatrix) {
        
            view = glm::lookAt(cameraPosition, cameraTarget, cameraUp);
        } else {
        
            updateCameraPosition();
            view = glm::lookAt(cameraPosition, glm::vec3(0.0f, 0.0f, 0.0f), cameraUp);
        }
        
        GLint viewLoc = glGetUniformLocation(shader.id(), "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fontTextureId);
        glUniform1i(glGetUniformLocation(shader.id(), "fontTexture"), 0);
        
        std::vector<float> vertices;
        int activeChars = 0;
        
        for (const auto &ch : characters) {
            if (!ch.active) continue;
            
            glm::vec2 texOffset{0,0}, texSize{0,0};
            for (const auto &info : charAtlas) {
                if (info.codepoint == ch.codepoint) {
                    texOffset = info.texOffset;
                    texSize = info.texSize;
                    break;
                }
            }
            
            glm::mat4 model = glm::mat4(1.0f);
            GLint modelLoc = glGetUniformLocation(shader.id(), "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            float greenValue = ch.trailIndex == 0 ? 1.0f : 0.5f + 0.5f * ch.opacity;
            float alpha = ch.opacity;
            
            if (ch.trailIndex == 0) {
               if (rand() % 5 == 0) {
                    greenValue = 1.5f;
                    float blueValue = 1.0f; 
                    float redValue = 0.8f;
                    
                    addCharacterQuad3D(vertices, ch.position, texOffset, texSize, 
                                      redValue, greenValue, blueValue, alpha);
                } else {
                    addCharacterQuad3D(vertices, ch.position, texOffset, texSize, 
                                      0.1f, greenValue, 0.1f, alpha);
                }
            } else {
                addCharacterQuad3D(vertices, ch.position, texOffset, texSize, 
                                  0.0f, greenValue, 0.0f, alpha);
            }
            
            activeChars++;
        }
        
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
        glDrawArrays(GL_TRIANGLES, 0, activeChars * 6);
        glBindVertexArray(0);
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
        std::string modeText = insideMatrix ? "Mode: Inside Matrix (WASD to move)" : "Mode: Orbital View";
        win->text.printText_Solid(font, 10.0f, 100.0f, modeText);
        win->text.printText_Solid(font, 10.0f, 130.0f, "Press SPACE to toggle mode");
    }
    
    void addCharacterQuad3D(std::vector<float> &vertices, 
                           const glm::vec3 &pos, 
                           const glm::vec2 &texOffset, 
                           const glm::vec2 &texSize,
                           float r, float g, float b, float a) {
        float halfSize = gridSpacing * 0.55f;          
        glm::vec3 look = glm::normalize(cameraPosition - pos);
        glm::vec3 right = glm::normalize(glm::cross(cameraUp, look));
        glm::vec3 up = glm::normalize(glm::cross(look, right));
        
        glm::vec3 bottomLeft = pos - right * halfSize - up * halfSize;
        glm::vec3 bottomRight = pos + right * halfSize - up * halfSize;
        glm::vec3 topLeft = pos - right * halfSize + up * halfSize;
        glm::vec3 topRight = pos + right * halfSize + up * halfSize;
        vertices.push_back(bottomLeft.x);
        vertices.push_back(bottomLeft.y);
        vertices.push_back(bottomLeft.z);
        vertices.push_back(0.0f);             
        vertices.push_back(0.0f);             
        vertices.push_back(r);                
        vertices.push_back(g);                
        vertices.push_back(b);                
        vertices.push_back(a);                
        vertices.push_back(texOffset.x);      
        vertices.push_back(texOffset.y);      
        
        vertices.push_back(bottomRight.x);
        vertices.push_back(bottomRight.y);
        vertices.push_back(bottomRight.z);
        vertices.push_back(1.0f);             
        vertices.push_back(0.0f);             
        vertices.push_back(r);                
        vertices.push_back(g);                
        vertices.push_back(b);                
        vertices.push_back(a);                
        vertices.push_back(texOffset.x + texSize.x); 
        vertices.push_back(texOffset.y);             
        
        vertices.push_back(topLeft.x);
        vertices.push_back(topLeft.y);
        vertices.push_back(topLeft.z);
        vertices.push_back(0.0f);             
        vertices.push_back(1.0f);             
        vertices.push_back(r);                
        vertices.push_back(g);                
        vertices.push_back(b);                
        vertices.push_back(a);                
        vertices.push_back(texOffset.x);             
        vertices.push_back(texOffset.y + texSize.y); 
        
        vertices.push_back(bottomRight.x);
        vertices.push_back(bottomRight.y);
        vertices.push_back(bottomRight.z);
        vertices.push_back(1.0f);             
        vertices.push_back(0.0f);             
        vertices.push_back(r);                
        vertices.push_back(g);                
        vertices.push_back(b);                
        vertices.push_back(a);                
        vertices.push_back(texOffset.x + texSize.x); 
        vertices.push_back(texOffset.y);             
        
        
        vertices.push_back(topRight.x);
        vertices.push_back(topRight.y);
        vertices.push_back(topRight.z);
        vertices.push_back(1.0f);             
        vertices.push_back(1.0f);             
        vertices.push_back(r);                
        vertices.push_back(g);                
        vertices.push_back(b);                
        vertices.push_back(a);                
        vertices.push_back(texOffset.x + texSize.x); 
        vertices.push_back(texOffset.y + texSize.y); 
        
        
        vertices.push_back(topLeft.x);
        vertices.push_back(topLeft.y);
        vertices.push_back(topLeft.z);
        vertices.push_back(0.0f);             
        vertices.push_back(1.0f);             
        vertices.push_back(r);                
        vertices.push_back(g);                
        vertices.push_back(b);                
        vertices.push_back(a);                
        vertices.push_back(texOffset.x);             
        vertices.push_back(texOffset.y + texSize.y); 
    }
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("The Matrix", tw, th) {
        setPath(path);
        setObject(new Matrix3D());
        object->load(this);
    }
    
    ~MainWindow() override {}
    
    virtual void event(SDL_Event &e) override {}
    
    virtual void draw() override {
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
        main_w = &main_window;
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
        main_window.loop();
    } catch(const mx::Exception &e) {
        mx::system_err << "mx: Exception: " << e.text() << "\n";
        mx::system_err.flush();
        exit(EXIT_FAILURE);
    }
#endif
    return 0;
}
