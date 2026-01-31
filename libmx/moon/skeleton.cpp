#include <SDL2/SDL.h>
#if defined(__APPLE__) || defined(_WIN32) || defined(__linux__)
#include "argz.hpp"
#endif

#include "mxcross.hpp"
#include "loadpng.hpp"
#include <random>
#include <format>
#include <fstream>
#include <sstream>
#include <fstream>
#include <sstream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


struct ModelVertex {
    float pos[3];
    float texCoord[2];
    float normal[3];
};

struct Vec2 { float x, y; };
struct Vec3 { float x, y, z; };


static void loadMXMOD(
    const std::string& path,
    std::vector<ModelVertex>& outVertices,
    std::vector<uint32_t>& outIndices,
    float positionScale = 1.0f
) {
    std::ifstream f(path);
    if (!f.is_open()) {
        throw mx::Exception("loadMXMOD: failed to open file: " + path);
    }

    
    auto readHeader = [&](const char* expectedTag, int& countOut) {
        std::string line;
        while (std::getline(f, line)) {
            size_t p = line.find_first_not_of(" \t\r\n");
            if (p == std::string::npos) continue;  
            if (line[p] == '#') continue;  
            
            std::istringstream iss(line);
            std::string tag;
            if (!(iss >> tag >> countOut)) {
                throw mx::Exception(std::string("loadMXMOD: missing header: ") + expectedTag);
            }
            if (tag != expectedTag) {
                throw mx::Exception(std::string("loadMXMOD: expected header '") + expectedTag + "', got '" + tag + "'");
            }
            return;  
        }
        throw mx::Exception(std::string("loadMXMOD: missing header: ") + expectedTag);
    };

    
    {
        std::string line;
        while (std::getline(f, line)) {
            size_t p = line.find_first_not_of(" \t\r\n");
            if (p == std::string::npos) continue;
            if (line[p] == '#') continue;
            
            std::istringstream iss(line);
            std::string triTag;
            int a = 0, b = 0;
            if (!(iss >> triTag >> a >> b)) {
                throw mx::Exception("loadMXMOD: missing tri header");
            }
            if (triTag != "tri") {
                throw mx::Exception("loadMXMOD: expected 'tri' header");
            }
            break;  
        }
    }

    
    auto readNonCommentLine = [&](std::string &line) -> bool {
        while (std::getline(f, line)) {
            
            size_t p = line.find_first_not_of(" \t\r\n");
            if (p == std::string::npos) continue;
            if (line[p] == '#') continue;
            return true;
        }
        return false;
    };

    
    int vcount = 0;
    readHeader("vert", vcount);
    if (vcount <= 0) throw mx::Exception("loadMXMOD: invalid vert count");

    std::vector<Vec3> pos(vcount);
    std::string line;
    int read = 0;
    while (read < vcount) {
        if (!readNonCommentLine(line)) throw mx::Exception("loadMXMOD: failed reading vertex positions");
        std::istringstream iss(line);
        if (!(iss >> pos[read].x >> pos[read].y >> pos[read].z)) {
            throw mx::Exception("loadMXMOD: failed reading vertex positions");
        }
        pos[read].x *= positionScale;
        pos[read].y *= positionScale;
        pos[read].z *= positionScale;
        ++read;
    }

    
    int tcount = 0;
    readHeader("tex", tcount);
    if (tcount != vcount) throw mx::Exception("loadMXMOD: tex count != vert count");

    std::vector<Vec2> uv(vcount);
    read = 0;
    while (read < vcount) {
        if (!readNonCommentLine(line)) throw mx::Exception("loadMXMOD: failed reading texcoords");
        std::istringstream iss(line);
        if (!(iss >> uv[read].x >> uv[read].y)) throw mx::Exception("loadMXMOD: failed reading texcoords");
        ++read;
    }

    
    int ncount = 0;
    readHeader("norm", ncount);
    if (ncount != vcount) throw mx::Exception("loadMXMOD: norm count != vert count");

    std::vector<Vec3> nrm(vcount);
    read = 0;
    while (read < vcount) {
        if (!readNonCommentLine(line)) throw mx::Exception("loadMXMOD: failed reading normals");
        std::istringstream iss(line);
        if (!(iss >> nrm[read].x >> nrm[read].y >> nrm[read].z)) throw mx::Exception("loadMXMOD: failed reading normals");
        ++read;
    }

    
    outVertices.clear();
    outVertices.reserve(static_cast<size_t>(vcount));
    for (int i = 0; i < vcount; ++i) {
        ModelVertex v{};
        v.pos[0] = pos[i].x;
        v.pos[1] = pos[i].y;
        v.pos[2] = pos[i].z;
        v.texCoord[0] = uv[i].x;
        v.texCoord[1] = uv[i].y;
        v.normal[0] = nrm[i].x;
        v.normal[1] = nrm[i].y;
        v.normal[2] = nrm[i].z;
        outVertices.push_back(v);
    }

    outIndices.clear();
    outIndices.reserve(static_cast<size_t>(vcount));
    for (uint32_t i = 0; i < static_cast<uint32_t>(vcount); ++i) outIndices.push_back(i);
    SDL_Log("Loaded MXMOD: %s (%d vertices)", path.c_str(), vcount);
}

struct Star {
    float x, y, z;
    float vx, vy, vz;
    float magnitude;
    float temperature;
    float twinkle;
    float size;
    float sizeFactor;  
    int starType;
    bool isCore;
    bool active;
    int textureIndex;  
    bool isConstellation;
};

struct StarVertex {
    float pos[3];
    float size;
    float color[4];
};

float generateRandomFloat(float min, float max) {
    static std::random_device rd;
    static std::default_random_engine eng(rd());
    std::uniform_real_distribution<float> dist(min, max);
    return dist(eng);
}

glm::vec3 getStarColor(float temperature) {
    float r, g, b;
    
    if (temperature < 3700) {
        r = 1.0f;
        g = temperature / 3700.0f * 0.6f;
        b = 0.0f;
    } else if (temperature < 5200) {
        r = 1.0f;
        g = 0.6f + (temperature - 3700) / 1500.0f * 0.4f;
        b = (temperature - 3700) / 1500.0f * 0.3f;
    } else if (temperature < 6000) {
        r = 1.0f;
        g = 1.0f;
        b = (temperature - 5200) / 800.0f * 0.7f;
    } else if (temperature < 7500) {
        r = 1.0f;
        g = 1.0f;
        b = 0.7f + (temperature - 6000) / 1500.0f * 0.3f;
    } else {
        r = 0.7f - (temperature - 7500) / 10000.0f * 0.4f;
        g = 0.8f + (temperature - 7500) / 10000.0f * 0.2f;
        b = 1.0f;
    }
    
    return glm::vec3(r, g, b);
}

float magnitudeToSize(float magnitude) {
    return glm::clamp(20.0f - magnitude * 3.0f, 2.0f, 35.0f);
}

float magnitudeToAlpha(float magnitude, float lightPollution) {
    float alpha = (6.5f - magnitude) / 6.5f;
    return glm::clamp(alpha - lightPollution, 0.0f, 1.0f);
}

constexpr int NUM_STARS = 20000;
constexpr int NUM_CORE = NUM_STARS / 8; 
#ifdef WITH_GL

#ifndef __EMSCRIPTEN__
const char* glStarVertSource = R"(#version 330 core
layout (location = 0) in vec3 inPosition; 
layout (location = 1) in float inSize;
layout (location = 2) in vec4 inColor;

uniform mat4 MVP;  

out vec4 fragColor;

void main() {
    gl_Position = MVP * vec4(inPosition, 1.0);
    gl_PointSize = inSize;
    fragColor = inColor;
}
)";

const char* glStarFragSource = R"(#version 330 core
in vec4 fragColor;
out vec4 FragColor;

uniform sampler2D spriteTexture;

void main() {
    vec2 coord = gl_PointCoord;
    vec4 texColor = texture(spriteTexture, coord);

    if(texColor.r < 0.5 && texColor.g < 0.5 && texColor.b < 0.5)
        discard;

    FragColor = texColor * fragColor;
    if (FragColor.a < 0.01)
        discard;
}
)";
#else
const char* glStarVertSource = R"(#version 300 es
precision highp float;
layout (location = 0) in vec3 inPosition;
layout (location = 1) in float inSize;
layout (location = 2) in vec4 inColor;

uniform mat4 MVP;
out vec4 fragColor;

void main() {
    gl_Position = MVP * vec4(inPosition, 1.0);
    gl_PointSize = inSize;
    fragColor = inColor;
}
)";

const char* glStarFragSource = R"(#version 300 es
precision highp float;
in vec4 fragColor;
out vec4 FragColor;

uniform sampler2D spriteTexture;

void main() {
    vec2 coord = gl_PointCoord;
    vec4 texColor = texture(spriteTexture, coord);
    if(texColor.r < 0.5 && texColor.g < 0.5 && texColor.b < 0.5)
        discard;

    FragColor = texColor * fragColor;
    if (FragColor.a < 0.01)
        discard;
}
)";
#endif





#ifndef __EMSCRIPTEN__
const char* glModelVertSource = R"(#version 330 core
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec2 inTexCoord;
layout (location = 2) in vec3 inNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 fragTexCoord;
out vec3 fragNormal;
out vec3 fragPos;

void main() {
    fragPos = vec3(model * vec4(inPosition, 1.0));
    fragNormal = mat3(transpose(inverse(model))) * inNormal;
    fragTexCoord = inTexCoord;
    gl_Position = projection * view * model * vec4(inPosition, 1.0);
}
)";

const char* glModelFragSource = R"(#version 330 core
in vec2 fragTexCoord;
in vec3 fragNormal;
in vec3 fragPos;

out vec4 FragColor;

uniform sampler2D modelTexture;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

void main() {
    // Ambient
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * lightColor;
    
    // Diffuse
    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = specularStrength * spec * lightColor;
    
    vec4 texColor = texture(modelTexture, fragTexCoord);
    vec3 result = (ambient + diffuse + specular) * texColor.rgb;
    FragColor = vec4(result, texColor.a);
}
)";
#else
const char* glModelVertSource = R"(#version 300 es
precision highp float;
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec2 inTexCoord;
layout (location = 2) in vec3 inNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 fragTexCoord;
out vec3 fragNormal;
out vec3 fragPos;

void main() {
    fragPos = vec3(model * vec4(inPosition, 1.0));
    fragNormal = mat3(transpose(inverse(model))) * inNormal;
    fragTexCoord = inTexCoord;
    gl_Position = projection * view * model * vec4(inPosition, 1.0);
}
)";

const char* glModelFragSource = R"(#version 300 es
precision highp float;
in vec2 fragTexCoord;
in vec3 fragNormal;
in vec3 fragPos;

out vec4 FragColor;

uniform sampler2D modelTexture;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

void main() {
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * lightColor;
    
    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = specularStrength * spec * lightColor;
    
    vec4 texColor = texture(modelTexture, fragTexCoord);
    vec3 result = (ambient + diffuse + specular) * texColor.rgb;
    FragColor = vec4(result, texColor.a);
}
)";
#endif

class GL_ModelRenderer {
public:
    gl::ShaderProgram program;
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;
    GLuint modelTexture = 0;
    
    std::vector<ModelVertex> vertices;
    std::vector<uint32_t> indices;
    
    bool initialized = false;
    float rotationX = 0.0f;
    float rotationY = 0.0f;
    float rotationZ = 0.0f;
    float modelScale = 1.0f;
    glm::vec3 modelPosition = glm::vec3(0.0f, 0.0f, -5.0f);
    
    void init(const std::string& dataPath, const std::string& modelFile, const std::string& textureFile,
              float scale = 1.0f, const glm::vec3& position = glm::vec3(0.0f, 0.0f, -5.0f)) {
        if (initialized) return;
        
        if (!program.loadProgramFromText(glModelVertSource, glModelFragSource)) {
            throw mx::Exception("Failed to load GL model shaders");
        }
        
        
        modelScale = scale;
        modelPosition = position;
        std::string modelPath = dataPath + "/data/" + modelFile;
        loadMXMOD(modelPath, vertices, indices, modelScale);
        std::string texPath = dataPath + "/data/" + textureFile;
        modelTexture = gl::loadTexture(texPath);
        if (!modelTexture) {
            SDL_Log("Warning: Could not load model texture from %s", texPath.c_str());
        }
        
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        
        glBindVertexArray(VAO);
        
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ModelVertex), vertices.data(), GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);
        
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, pos));
        glEnableVertexAttribArray(0);
        
        
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, texCoord));
        glEnableVertexAttribArray(1);
        
        
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(ModelVertex), (void*)offsetof(ModelVertex, normal));
        glEnableVertexAttribArray(2);
        
        glBindVertexArray(0);
        
        initialized = true;
        SDL_Log("GL_ModelRenderer initialized with %zu vertices", vertices.size());
    }
    
    void draw(int screenW, int screenH, const glm::vec3& cameraPos, float cameraYaw, float cameraPitch) {
        if (!initialized) return;
        
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        
        program.useProgram();
        
        
        float time = SDL_GetTicks() * 0.001f;
        float rotX = time * 10.0f;
        float rotY = time * 30.0f;
        float rotZ = time * 5.0f;
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, modelPosition);
        model = glm::rotate(model, glm::radians(rotX), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotY), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotZ), glm::vec3(0.0f, 0.0f, 1.0f));
        
        
        glm::vec3 front;
        front.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
        front.y = sin(glm::radians(cameraPitch));
        front.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
        front = glm::normalize(front);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + front, glm::vec3(0.0f, 1.0f, 0.0f));
        
        
        glm::mat4 projection = glm::perspective(
            glm::radians(60.0f),
            (float)screenW / (float)screenH,
            0.1f, 10000.0f
        );
        
        program.setUniform("model", model);
        program.setUniform("view", view);
        program.setUniform("projection", projection);
        program.setUniform("lightPos", glm::vec3(10.0f, 10.0f, 10.0f));
        program.setUniform("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
        program.setUniform("viewPos", cameraPos);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, modelTexture);
        program.setUniform("modelTexture", 0);
        
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
    
    void update(float deltaTime) {
        
        rotationY += 30.0f * deltaTime;
        if (rotationY >= 360.0f) rotationY -= 360.0f;
    }
    
    void cleanup() {
        if (!initialized) return;
        
        if (VAO) {
            glDeleteVertexArrays(1, &VAO);
            VAO = 0;
        }
        if (VBO) {
            glDeleteBuffers(1, &VBO);
            VBO = 0;
        }
        if (EBO) {
            glDeleteBuffers(1, &EBO);
            EBO = 0;
        }
        if (modelTexture) {
            glDeleteTextures(1, &modelTexture);
            modelTexture = 0;
        }
        
        vertices.clear();
        indices.clear();
        initialized = false;
    }
};

class GL_Starfield {
public:
    gl::ShaderProgram program;
    GLuint VBO[3], VAO;
    GLuint EBO = 0;  
    GLuint starTexture = 0;
    GLuint starTexture2 = 0;
    std::vector<GLuint> tex1Indices;
    std::vector<GLuint> tex2Indices;
    Star stars[NUM_STARS];
    bool initialized = false;
    Uint32 lastUpdateTime = 0;
    
    float cameraX = 0.0f, cameraY = 0.0f, cameraZ = 0.0f;
    float cameraYaw = -90.0f, cameraPitch = 0.0f;
    float cameraSpeed = 50.0f;
    float atmosphericTwinkle = 0.5f;
    float lightPollution = 0.0f;
    float warpSpeed = 175.0f;         
    float starFieldRadius = 500.0f;  
    float starFieldDepth = 1000.0f;  
    
    glm::vec3 exclusionCenter = glm::vec3(0.0f);
    float exclusionRadius = 0.0f;
    
    glm::vec3 modelPosToDisplay = glm::vec3(0.0f);
    const Uint8* keyboardState = nullptr;

    void init(const std::string& dataPath) {
        if (initialized) return;
        
        keyboardState = SDL_GetKeyboardState(nullptr);
        
        if (!program.loadProgramFromText(glStarVertSource, glStarFragSource)) {
            throw mx::Exception("Failed to load GL star shaders");
        }
        
        std::string texPath = dataPath + "/data/star.png";
        starTexture = gl::loadTexture(texPath);
        if (!starTexture) {
            SDL_Log("Warning: Could not load star texture from %s", texPath.c_str());
        }
        
        std::string texPath2 = dataPath + "/data/star2.png";
        starTexture2 = gl::loadTexture(texPath2);
        if (!starTexture2) {
            SDL_Log("Warning: Could not load star texture2 from %s", texPath2.c_str());
        }
        
        initStars();
        
        glGenVertexArrays(1, &VAO);
        glGenBuffers(3, VBO);
        
        glBindVertexArray(VAO);
        
        glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
        glBufferData(GL_ARRAY_BUFFER, NUM_STARS * 3 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(0);
        
        glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
        glBufferData(GL_ARRAY_BUFFER, NUM_STARS * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(1);
        
        glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
        glBufferData(GL_ARRAY_BUFFER, NUM_STARS * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
        glEnableVertexAttribArray(2);
        
        glBindVertexArray(0);
        
        lastUpdateTime = SDL_GetTicks();
        initialized = true;
        SDL_Log("GL Starfield initialized with %d stars", NUM_STARS);
    }

    void initStar(Star& star, bool randomZ = true, bool core = false) {
        
        for (int attempts = 0; attempts < 16; ++attempts) {
            float theta = generateRandomFloat(0.0f, 2.0f * M_PI);
            star.z = randomZ ? generateRandomFloat(-300.0f, -10.0f) : -300.0f;
            float r = sqrt(generateRandomFloat(0.0f, 1.0f)) * starFieldRadius * 0.3f;

            star.x = r * cos(theta);
            star.y = r * sin(theta);

            float dx = star.x - exclusionCenter.x;
            float dy = star.y - exclusionCenter.y;
            float dz = star.z - exclusionCenter.z;
            float d2 = dx*dx + dy*dy + dz*dz;
            if (exclusionRadius <= 0.0f || d2 > exclusionRadius*exclusionRadius) break;
        }

        star.isCore = core;
        star.active = true;

        star.vx = generateRandomFloat(-1.0f, 1.0f);
        star.vy = generateRandomFloat(-1.0f, 1.0f);
        star.vz = warpSpeed;
        
        float rand = generateRandomFloat(0.0f, 1.0f);
        if (rand < 0.05f) {
            star.magnitude = generateRandomFloat(-1.5f, 1.5f);
            star.starType = 0;
        } else if (rand < 0.25f) {
            star.magnitude = generateRandomFloat(1.5f, 4.0f);
            star.starType = 1;
        } else {
            star.magnitude = generateRandomFloat(4.0f, 6.5f);
            star.starType = 2;
        }
        
        if (star.starType == 1) {
            star.temperature = generateRandomFloat(3000.0f, 5000.0f);
        } else if (star.starType == 0) {
            star.temperature = generateRandomFloat(4000.0f, 8000.0f);
        } else {
            star.temperature = generateRandomFloat(2500.0f, 4000.0f);
        }
        
        star.twinkle = generateRandomFloat(0.5f, 3.0f);
        star.sizeFactor = generateRandomFloat(0.5f, 2.0f);  
        star.size = magnitudeToSize(star.magnitude) * star.sizeFactor;
        star.textureIndex = (generateRandomFloat(0.0f, 1.0f) < 0.5f) ? 0 : 1;
        star.isConstellation = (star.magnitude < 3.0f && generateRandomFloat(0.0f, 1.0f) < 0.3f);
    }

    void initStars() {
        
        for (int i = 0; i < NUM_CORE; ++i) {
            initStar(stars[i], true, true);
        }
        
        for (int i = NUM_CORE; i < NUM_STARS; ++i) {
            initStar(stars[i], true, false);  
        }
    }

    void update(float deltaTime) {
        if (!initialized) return;
        if (deltaTime > 0.1f) deltaTime = 0.1f;

        std::vector<float> positions;
        std::vector<float> sizes;
        std::vector<float> colors;
        positions.reserve(NUM_STARS * 3);
        sizes.reserve(NUM_STARS);
        colors.reserve(NUM_STARS * 4);
        
        tex1Indices.clear();
        tex2Indices.clear();
        tex1Indices.reserve(NUM_STARS / 2);
        tex2Indices.reserve(NUM_STARS / 2);

        float time = SDL_GetTicks() * 0.001f;

        for (GLuint i = 0; i < NUM_STARS; ++i) {
            auto& star = stars[i];

            
            float moveX = star.vx * deltaTime;
            float moveY = star.vy * deltaTime;
            float moveZ = star.vz * deltaTime;
            float moveLen = sqrt(moveX*moveX + moveY*moveY + moveZ*moveZ);
            int steps = 1;
            if (exclusionRadius > 0.0f && moveLen > 0.0f) {
                float maxStep = exclusionRadius * 0.5f;
                steps = (int)ceil(moveLen / maxStep);
                if (steps < 1) steps = 1;
                if (steps > 16) steps = 16;
            }

            bool respawned = false;
            for (int s = 0; s < steps; ++s) {
                float stepDt = deltaTime / (float)steps;
                float nextX = star.x + star.vx * stepDt;
                float nextY = star.y + star.vy * stepDt;
                float nextZ = star.z + star.vz * stepDt;

                if (exclusionRadius > 0.0f) {
                    float dx = nextX - exclusionCenter.x;
                    float dy = nextY - exclusionCenter.y;
                    float dz = nextZ - exclusionCenter.z;
                    float d2 = dx*dx + dy*dy + dz*dz;
                    if (d2 <= exclusionRadius*exclusionRadius) {
                        for (int attempts = 0; attempts < 16; ++attempts) {
                            float theta = generateRandomFloat(0.0f, 2.0f * M_PI);
                            star.z = generateRandomFloat(-400.0f, -150.0f);
                            float r = sqrt(generateRandomFloat(0.0f, 1.0f)) * starFieldRadius * 0.3f;
                            star.x = r * cos(theta);
                            star.y = r * sin(theta);
                            float tx = star.x - exclusionCenter.x;
                            float ty = star.y - exclusionCenter.y;
                            float tz = star.z - exclusionCenter.z;
                            float td2 = tx*tx + ty*ty + tz*tz;
                            if (td2 > exclusionRadius*exclusionRadius) break;
                        }
                        respawned = true;
                        break;
                    }
                }

                
                star.x = nextX;
                star.y = nextY;
                star.z = nextZ;
            }

            if (respawned) continue;
            
            
            if (star.z > 10.0f) {
                float theta = generateRandomFloat(0.0f, 2.0f * M_PI);
                
                for (int attempts = 0; attempts < 16; ++attempts) {
                    star.z = generateRandomFloat(-400.0f, -150.0f);
                    float r = sqrt(generateRandomFloat(0.0f, 1.0f)) * starFieldRadius * 0.3f;
                    star.x = r * cos(theta);
                    star.y = r * sin(theta);
                    float dx = star.x - exclusionCenter.x;
                    float dy = star.y - exclusionCenter.y;
                    float dz = star.z - exclusionCenter.z;
                    float d2 = dx*dx + dy*dy + dz*dz;
                    if (exclusionRadius <= 0.0f || d2 > exclusionRadius*exclusionRadius) break;
                }
            }

            if (!star.active) continue;

            
            float dx = star.x - exclusionCenter.x;
            float dy = star.y - exclusionCenter.y;
            float dz = star.z - exclusionCenter.z;
            float d2 = dx*dx + dy*dy + dz*dz;
            if (exclusionRadius > 0.0f && d2 <= exclusionRadius*exclusionRadius) {
                continue;
            }

            positions.push_back(star.x);
            positions.push_back(star.y);
            positions.push_back(star.z);

            float twinkleFactor = 1.0f;
            if (atmosphericTwinkle > 0.0f) {
                twinkleFactor = 0.7f + 0.3f * sin(time * star.twinkle) * atmosphericTwinkle;
            }

            float size = star.size * twinkleFactor;
            if (star.isConstellation) {
                size *= 1.2f;
            }
                float cam_dx = star.x - cameraX;
                float cam_dy = star.y - cameraY;
                float cam_dz = star.z - cameraZ;
                float dist = glm::length(glm::vec3(cam_dx, cam_dy, cam_dz));
                float perspectiveScale = glm::clamp(150.0f / glm::max(dist, 1.0f), 0.05f, 20.0f);
                size *= perspectiveScale;
            sizes.push_back(size);

            glm::vec3 starColor = getStarColor(star.temperature);
            float alpha = magnitudeToAlpha(star.magnitude, lightPollution) * twinkleFactor;

            colors.push_back(starColor.r);
            colors.push_back(starColor.g);
            colors.push_back(starColor.b);
            colors.push_back(alpha);
            
            if (star.textureIndex == 0) {
                tex1Indices.push_back(i);
            } else {
                tex2Indices.push_back(i);
            }
        }

        glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, positions.size() * sizeof(float), positions.data());

        glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizes.size() * sizeof(float), sizes.data());

        glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, colors.size() * sizeof(float), colors.data());
        
        
    }

    void draw(int screenW, int screenH) {
        if (!initialized) return;

        #ifndef __EMSCRIPTEN__
        glEnable(GL_PROGRAM_POINT_SIZE);
        #endif

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f;
        lastUpdateTime = currentTime;

        update(deltaTime);

        program.useProgram();

        glm::mat4 projection = glm::perspective(
            glm::radians(60.0f),
            (float)screenW / (float)screenH,
            0.1f,
            starFieldDepth + 500.0f
        );

        glm::vec3 front;
        front.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
        front.y = sin(glm::radians(cameraPitch));
        front.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
        front = glm::normalize(front);

        glm::vec3 cameraPos(cameraX, cameraY, cameraZ);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + front, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 mvp = projection * view;

        program.setUniform("MVP", mvp);
        program.setUniform("spriteTexture", 0);

        glBindVertexArray(VAO);

        glActiveTexture(GL_TEXTURE0);

        glBindTexture(GL_TEXTURE_2D, starTexture);
        if (!tex1Indices.empty()) {
            glDrawElements(GL_POINTS, (GLsizei)tex1Indices.size(), GL_UNSIGNED_INT, tex1Indices.data());
        }

        glBindTexture(GL_TEXTURE_2D, starTexture2);
        if (!tex2Indices.empty()) {
            glDrawElements(GL_POINTS, (GLsizei)tex2Indices.size(), GL_UNSIGNED_INT, tex2Indices.data());
        }

        glBindVertexArray(0);

        glDepthMask(GL_TRUE);
    }

    void cleanup() {
        if (!initialized) return;
        
        if (VAO) {
            glDeleteVertexArrays(1, &VAO);
            VAO = 0;
        }
        if (VBO[0]) {
            glDeleteBuffers(3, VBO);
            VBO[0] = VBO[1] = VBO[2] = 0;
        }
        if (starTexture) {
            glDeleteTextures(1, &starTexture);
            starTexture = 0;
        }
        if (starTexture2) {
            glDeleteTextures(1, &starTexture2);
            starTexture2 = 0;
        }
        initialized = false;
    }

    void processInput(float deltaTime) {
        if (!keyboardState) return;
        
        glm::vec3 front;
        front.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
        front.y = sin(glm::radians(cameraPitch));
        front.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
        front = glm::normalize(front);
        
        float velocity = cameraSpeed * deltaTime;
        
        if (keyboardState[SDL_SCANCODE_W]) {
            cameraX += front.x * velocity;
            cameraY += front.y * velocity;
            cameraZ += front.z * velocity;
        }
        if (keyboardState[SDL_SCANCODE_S]) {
            cameraX -= front.x * velocity;
            cameraY -= front.y * velocity;
            cameraZ -= front.z * velocity;
        }
        /*if (keyboardState[SDL_SCANCODE_A]) {
            cameraX -= right.x * velocity;
            cameraZ -= right.z * velocity;
        }
        if (keyboardState[SDL_SCANCODE_D]) {
            cameraX += right.x * velocity;
            cameraZ += right.z * velocity;
        }*/

        
        if (keyboardState[SDL_SCANCODE_SPACE]) {
            cameraY += velocity;
        }
        if (keyboardState[SDL_SCANCODE_LSHIFT]) {
            cameraY -= velocity;
        }

        
        float rotSpeed = 90.0f; 
        if (keyboardState[SDL_SCANCODE_LEFT]) {
            cameraYaw -= rotSpeed * deltaTime;
        }
        if (keyboardState[SDL_SCANCODE_RIGHT]) {
            cameraYaw += rotSpeed * deltaTime;
        }
        if (keyboardState[SDL_SCANCODE_UP]) {
            cameraPitch += rotSpeed * deltaTime;
        }
        if (keyboardState[SDL_SCANCODE_DOWN]) {
            cameraPitch -= rotSpeed * deltaTime;
        }
        cameraPitch = glm::clamp(cameraPitch, -89.0f, 89.0f);
    }
};

#endif 

#ifdef WITH_VK

class VK_Starfield {
public:
    mx::VKWindow* vkWindow = nullptr;
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    
    VkImage starTexture = VK_NULL_HANDLE;
    VkDeviceMemory starTextureMemory = VK_NULL_HANDLE;
    VkImageView starTextureView = VK_NULL_HANDLE;
    VkSampler starSampler = VK_NULL_HANDLE;
    
    VkImage starTexture2 = VK_NULL_HANDLE;
    VkDeviceMemory starTextureMemory2 = VK_NULL_HANDLE;
    VkImageView starTextureView2 = VK_NULL_HANDLE;
    
    VkDescriptorSetLayout starDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool starDescriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> starDescriptorSets;
    std::vector<VkDescriptorSet> starDescriptorSets2;  
    
    VkPipelineLayout starPipelineLayout = VK_NULL_HANDLE;
    VkPipeline starPipeline = VK_NULL_HANDLE;
    
    VkBuffer starVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory starVertexBufferMemory = VK_NULL_HANDLE;
    void* starVertexBufferMapped = nullptr;
    
    VkBuffer starIndexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory starIndexBufferMemory = VK_NULL_HANDLE;
    void* starIndexBufferMapped = nullptr;
    
    Star stars[NUM_STARS];
    StarVertex starVertices[NUM_STARS];
    std::vector<uint32_t> tex1Indices;  
    std::vector<uint32_t> tex2Indices;  
    bool initialized = false;
    Uint32 lastUpdateTime = 0;
    
    float cameraX = 0.0f, cameraY = 0.0f, cameraZ = 0.0f;
    float cameraYaw = -90.0f, cameraPitch = 0.0f;
    float cameraSpeed = 50.0f;
    float atmosphericTwinkle = 0.5f;
    float lightPollution = 0.0f;
    float warpSpeed = 175.0f;
    float starFieldDepth = 15000.0f;
    float starFieldRadius = 500.0f;
    
    glm::vec3 exclusionCenter = glm::vec3(0.0f);
    float exclusionRadius = 0.0f;
    
    glm::vec3 modelPosToDisplay = glm::vec3(0.0f);
    const Uint8* keyboardState = nullptr;

    void cleanup() {
        if (!initialized || device == VK_NULL_HANDLE) return;
        
        SDL_Log("VK_Starfield::cleanup() - start");
        vkDeviceWaitIdle(device);
        
        if (starPipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, starPipeline, nullptr);
            starPipeline = VK_NULL_HANDLE;
        }
        
        if (starPipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, starPipelineLayout, nullptr);
            starPipelineLayout = VK_NULL_HANDLE;
        }
        
        if (starDescriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device, starDescriptorPool, nullptr);
            starDescriptorPool = VK_NULL_HANDLE;
        }
        starDescriptorSets.clear();
        starDescriptorSets2.clear();
        
        if (starDescriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device, starDescriptorSetLayout, nullptr);
            starDescriptorSetLayout = VK_NULL_HANDLE;
        }
        
        if (starVertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, starVertexBuffer, nullptr);
            vkFreeMemory(device, starVertexBufferMemory, nullptr);
            starVertexBuffer = VK_NULL_HANDLE;
            starVertexBufferMapped = nullptr;
        }
        
        if (starIndexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, starIndexBuffer, nullptr);
            vkFreeMemory(device, starIndexBufferMemory, nullptr);
            starIndexBuffer = VK_NULL_HANDLE;
            starIndexBufferMapped = nullptr;
        }
        
        if (starSampler != VK_NULL_HANDLE) {
            vkDestroySampler(device, starSampler, nullptr);
            starSampler = VK_NULL_HANDLE;
        }
        
        if (starTextureView != VK_NULL_HANDLE) {
            vkDestroyImageView(device, starTextureView, nullptr);
            starTextureView = VK_NULL_HANDLE;
        }
        
        if (starTexture != VK_NULL_HANDLE) {
            vkDestroyImage(device, starTexture, nullptr);
            vkFreeMemory(device, starTextureMemory, nullptr);
            starTexture = VK_NULL_HANDLE;
        }
        
        if (starTextureView2 != VK_NULL_HANDLE) {
            vkDestroyImageView(device, starTextureView2, nullptr);
            starTextureView2 = VK_NULL_HANDLE;
        }
        
        if (starTexture2 != VK_NULL_HANDLE) {
            vkDestroyImage(device, starTexture2, nullptr);
            vkFreeMemory(device, starTextureMemory2, nullptr);
            starTexture2 = VK_NULL_HANDLE;
        }
        
        SDL_Log("VK_Starfield::cleanup() - complete");
        initialized = false;
        device = VK_NULL_HANDLE;
    }

    void init(mx::VKWindow* vkWin, const std::string& dataPath) {
        if (initialized) return;
        
        vkWindow = vkWin;
        device = vkWin->getDevice();
        physicalDevice = vkWin->getPhysicalDevice();
        commandPool = vkWin->getCommandPool();
        graphicsQueue = vkWin->getGraphicsQueue();
        
        keyboardState = SDL_GetKeyboardState(nullptr);
        
        initStars();
        loadStarTexture(dataPath + "/data/star.png", starTexture, starTextureMemory, starTextureView);
        loadStarTexture(dataPath + "/data/star2.png", starTexture2, starTextureMemory2, starTextureView2);
        createStarDescriptorSetLayout();
        createStarPipeline(vkWin, dataPath);
        createStarVertexBuffer();
        createStarDescriptorPool(vkWin);
        createStarDescriptorSets(vkWin);
        
        lastUpdateTime = SDL_GetTicks();
        initialized = true;
        
        SDL_Log("VK Starfield initialized with %d stars", NUM_STARS);
    }

    void initStar(Star& star, bool randomZ = true, bool core = false) {
        
        for (int attempts = 0; attempts < 16; ++attempts) {
            float theta = generateRandomFloat(0.0f, 2.0f * M_PI);
            star.z = randomZ ? generateRandomFloat(-300.0f, -10.0f) : -300.0f;
            float r = sqrt(generateRandomFloat(0.0f, 1.0f)) * starFieldRadius * 0.3f;

            star.x = r * cos(theta);
            star.y = r * sin(theta);

            float dx = star.x - exclusionCenter.x;
            float dy = star.y - exclusionCenter.y;
            float dz = star.z - exclusionCenter.z;
            float d2 = dx*dx + dy*dy + dz*dz;
            if (exclusionRadius <= 0.0f || d2 > exclusionRadius*exclusionRadius) break;
        }
        star.isCore = core;
        star.active = true;
        star.vx = generateRandomFloat(-1.0f, 1.0f);
        star.vy = generateRandomFloat(-1.0f, 1.0f);
        star.vz = warpSpeed;
        
        float rand = generateRandomFloat(0.0f, 1.0f);
        if (rand < 0.05f) {
            star.magnitude = generateRandomFloat(-1.5f, 1.5f);
            star.starType = 0;
        } else if (rand < 0.25f) {
            star.magnitude = generateRandomFloat(1.5f, 4.0f);
            star.starType = 1;
        } else {
            star.magnitude = generateRandomFloat(4.0f, 6.5f);
            star.starType = 2;
        }
        
        if (star.starType == 1) {
            star.temperature = generateRandomFloat(3000.0f, 5000.0f);
        } else if (star.starType == 0) {
            star.temperature = generateRandomFloat(4000.0f, 8000.0f);
        } else {
            star.temperature = generateRandomFloat(2500.0f, 4000.0f);
        }
        
        star.twinkle = generateRandomFloat(0.5f, 3.0f);
        star.sizeFactor = generateRandomFloat(0.5f, 2.0f);  
        star.size = magnitudeToSize(star.magnitude) * star.sizeFactor;
        star.textureIndex = (generateRandomFloat(0.0f, 1.0f) < 0.5f) ? 0 : 1;
        star.isConstellation = (star.magnitude < 3.0f && generateRandomFloat(0.0f, 1.0f) < 0.3f);
    }

    void initStars() {
        for (int i = 0; i < NUM_STARS; ++i) {
            initStar(stars[i], true);  
        }
    }

    void loadStarTexture(const std::string& path, VkImage& outTexture, 
                          VkDeviceMemory& outTextureMemory, VkImageView& outTextureView) {
        SDL_Surface* starImg = png::LoadPNG(path.c_str());
        if (!starImg || !starImg->pixels) {
            throw mx::Exception("Failed to load star texture: " + path);
        }
        
        int width = starImg->w;
        int height = starImg->h;
        VkDeviceSize imageSize = width * height * 4;
        
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = imageSize;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        
        VK_CHECK_RESULT(vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer));
        
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, stagingBuffer, &memRequirements);
        
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        
        VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &stagingBufferMemory));
        vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);
        
        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, starImg->pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(device, stagingBufferMemory);
        
        SDL_FreeSurface(starImg);
        
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = static_cast<uint32_t>(width);
        imageInfo.extent.height = static_cast<uint32_t>(height);
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        
        VK_CHECK_RESULT(vkCreateImage(device, &imageInfo, nullptr, &outTexture));
        
        vkGetImageMemoryRequirements(device, outTexture, &memRequirements);
        
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        
        VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &outTextureMemory));
        vkBindImageMemory(device, outTexture, outTextureMemory, 0);
        
        transitionImageLayout(outTexture, VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copyBufferToImage(stagingBuffer, outTexture, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
        transitionImageLayout(outTexture, VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
        
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = outTexture;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        
        VK_CHECK_RESULT(vkCreateImageView(device, &viewInfo, nullptr, &outTextureView));
        
        
        if (starSampler == VK_NULL_HANDLE) {
            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.anisotropyEnable = VK_FALSE;
            samplerInfo.maxAnisotropy = 1.0f;
            samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            
            VK_CHECK_RESULT(vkCreateSampler(device, &samplerInfo, nullptr, &starSampler));
        }
    }


    void transitionImageLayout(VkImage image, VkFormat format,
        VkImageLayout oldLayout, VkImageLayout newLayout) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;
        
        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
        
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        
        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        
        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;
        
        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
            newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
                   newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
            throw mx::Exception("Unsupported layout transition");
        }
        
        vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage,
            0, 0, nullptr, 0, nullptr, 1, &barrier);
        
        vkEndCommandBuffer(commandBuffer);
        
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        
        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);
        
        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;
        
        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
        
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        
        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {width, height, 1};
        
        vkCmdCopyBufferToImage(commandBuffer, buffer, image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        
        vkEndCommandBuffer(commandBuffer);
        
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        
        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);
        
        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    void createStarDescriptorSetLayout() {
        std::array<VkDescriptorSetLayoutBinding, 2> bindings{};
        
        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[0].pImmutableSamplers = nullptr;
        
        bindings[1].binding = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        bindings[1].pImmutableSamplers = nullptr;
        
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();
        
        VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &starDescriptorSetLayout));
    }

    void createStarDescriptorPool(mx::VKWindow* vkWin) {
        uint32_t imageCount = vkWin->getSwapChainImageCount();
        
        
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[0].descriptorCount = imageCount * 2;
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[1].descriptorCount = imageCount * 2;
        
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = imageCount * 2;  
        
        VK_CHECK_RESULT(vkCreateDescriptorPool(device, &poolInfo, nullptr, &starDescriptorPool));
    }

    void createStarDescriptorSets(mx::VKWindow* vkWin) {
        uint32_t imageCount = vkWin->getSwapChainImageCount();
        std::vector<VkDescriptorSetLayout> layouts(imageCount * 2, starDescriptorSetLayout);
        
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = starDescriptorPool;
        allocInfo.descriptorSetCount = imageCount * 2;
        allocInfo.pSetLayouts = layouts.data();
        
        std::vector<VkDescriptorSet> allSets(imageCount * 2);
        VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, allSets.data()));
        
        starDescriptorSets.resize(imageCount);
        starDescriptorSets2.resize(imageCount);
        for (uint32_t i = 0; i < imageCount; i++) {
            starDescriptorSets[i] = allSets[i];
            starDescriptorSets2[i] = allSets[imageCount + i];
        }
        
        for (size_t i = 0; i < imageCount; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = vkWin->getUniformBuffer(i);
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(mx::UniformBufferObject);
            
            
            VkDescriptorImageInfo imageInfo1{};
            imageInfo1.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo1.imageView = starTextureView;
            imageInfo1.sampler = starSampler;
            
            std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
            
            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = starDescriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pImageInfo = &imageInfo1;
            
            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = starDescriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pBufferInfo = &bufferInfo;
            
            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()),
                                   descriptorWrites.data(), 0, nullptr);
            
            
            VkDescriptorImageInfo imageInfo2{};
            imageInfo2.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo2.imageView = starTextureView2;
            imageInfo2.sampler = starSampler;
            
            descriptorWrites[0].dstSet = starDescriptorSets2[i];
            descriptorWrites[0].pImageInfo = &imageInfo2;
            
            descriptorWrites[1].dstSet = starDescriptorSets2[i];
            
            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()),
                                   descriptorWrites.data(), 0, nullptr);
        }
    }

    void createStarVertexBuffer() {
        VkDeviceSize bufferSize = sizeof(StarVertex) * NUM_STARS;
        
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        
        VK_CHECK_RESULT(vkCreateBuffer(device, &bufferInfo, nullptr, &starVertexBuffer));
        
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, starVertexBuffer, &memRequirements);
        
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        
        VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &starVertexBufferMemory));
        vkBindBufferMemory(device, starVertexBuffer, starVertexBufferMemory, 0);
        
        vkMapMemory(device, starVertexBufferMemory, 0, bufferSize, 0, &starVertexBufferMapped);
        
        
        VkDeviceSize indexBufferSize = sizeof(uint32_t) * NUM_STARS;
        
        bufferInfo.size = indexBufferSize;
        bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        
        VK_CHECK_RESULT(vkCreateBuffer(device, &bufferInfo, nullptr, &starIndexBuffer));
        
        vkGetBufferMemoryRequirements(device, starIndexBuffer, &memRequirements);
        
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        
        VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &starIndexBufferMemory));
        vkBindBufferMemory(device, starIndexBuffer, starIndexBufferMemory, 0);
        
        vkMapMemory(device, starIndexBufferMemory, 0, indexBufferSize, 0, &starIndexBufferMapped);
    }

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
        
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        
        throw mx::Exception("Failed to find suitable memory type");
    }

    void createStarPipeline(mx::VKWindow* vkWin, const std::string& dataPath) {
        auto vertShaderCode = mx::readFile(dataPath + "/data/star_vert.spv");
        auto fragShaderCode = mx::readFile(dataPath + "/data/star_frag.spv");
        
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = vertShaderCode.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(vertShaderCode.data());
        
        VkShaderModule vertShaderModule;
        VK_CHECK_RESULT(vkCreateShaderModule(device, &createInfo, nullptr, &vertShaderModule));
        
        createInfo.codeSize = fragShaderCode.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(fragShaderCode.data());
        
        VkShaderModule fragShaderModule;
        VK_CHECK_RESULT(vkCreateShaderModule(device, &createInfo, nullptr, &fragShaderModule));
        
        VkPipelineShaderStageCreateInfo shaderStages[2] = {};
        shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStages[0].module = vertShaderModule;
        shaderStages[0].pName = "main";
        
        shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStages[1].module = fragShaderModule;
        shaderStages[1].pName = "main";
        
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(StarVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(StarVertex, pos);
        
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(StarVertex, size);
        
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(StarVertex, color);
        
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
        
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;
        
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)vkWin->getSwapChainExtent().width;
        viewport.height = (float)vkWin->getSwapChainExtent().height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        
        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = vkWin->getSwapChainExtent();
        
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();
        
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;
        
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        
        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_FALSE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;
        
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &starDescriptorSetLayout;
        
        VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &starPipelineLayout));
        
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = starPipelineLayout;
        pipelineInfo.renderPass = vkWin->getRenderPass();
        pipelineInfo.subpass = 0;
        
        VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &starPipeline));
        
        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }

    void update(float deltaTime) {
        if (!initialized) return;
        if (deltaTime > 0.1f) deltaTime = 0.1f;
        
        float time = SDL_GetTicks() * 0.001f;
        
        tex1Indices.clear();
        tex2Indices.clear();
        tex1Indices.reserve(NUM_STARS / 2);
        tex2Indices.reserve(NUM_STARS / 2);
        
        for (int i = 0; i < NUM_STARS; ++i) {
            auto& star = stars[i];
            
            
            float moveX = star.vx * deltaTime;
            float moveY = star.vy * deltaTime;
            float moveZ = star.vz * deltaTime;
            float moveLen = sqrt(moveX*moveX + moveY*moveY + moveZ*moveZ);
            
            int steps = 1;
            if (exclusionRadius > 0.0f && moveLen > 0.0f) {
                float maxStep = exclusionRadius * 0.5f;
                steps = (int)ceil(moveLen / maxStep);
                if (steps < 1) steps = 1;
                if (steps > 16) steps = 16;
            }
            
            bool respawned = false;
            for (int s = 0; s < steps && !respawned; ++s) {
                float stepDt = deltaTime / (float)steps;
                star.x += star.vx * stepDt;
                star.y += star.vy * stepDt;
                star.z += star.vz * stepDt;
                
                
                if (exclusionRadius > 0.0f) {
                    float dx = star.x - exclusionCenter.x;
                    float dy = star.y - exclusionCenter.y;
                    float dz = star.z - exclusionCenter.z;
                    float d2 = dx*dx + dy*dy + dz*dz;
                    
                    if (d2 <= exclusionRadius*exclusionRadius) {
                        
                        for (int attempts = 0; attempts < 16; ++attempts) {
                            float theta = generateRandomFloat(0.0f, 2.0f * M_PI);
                            star.z = generateRandomFloat(-400.0f, -150.0f);
                            float r = sqrt(generateRandomFloat(0.0f, 1.0f)) * starFieldRadius * 0.3f;
                            star.x = r * cos(theta);
                            star.y = r * sin(theta);
                            
                            dx = star.x - exclusionCenter.x;
                            dy = star.y - exclusionCenter.y;
                            dz = star.z - exclusionCenter.z;
                            d2 = dx*dx + dy*dy + dz*dz;
                            if (d2 > exclusionRadius*exclusionRadius) break;
                        }
                        respawned = true;
                    }
                }
            }
            
            if (respawned) continue;
            
            
            if (star.z > 10.0f) {
                for (int attempts = 0; attempts < 16; ++attempts) {
                    float theta = generateRandomFloat(0.0f, 2.0f * M_PI);
                    star.z = generateRandomFloat(-400.0f, -150.0f);
                    float r = sqrt(generateRandomFloat(0.0f, 1.0f)) * starFieldRadius * 0.3f;
                    star.x = r * cos(theta);
                    star.y = r * sin(theta);
                    
                    if (exclusionRadius <= 0.0f) break;
                    
                    float dx = star.x - exclusionCenter.x;
                    float dy = star.y - exclusionCenter.y;
                    float dz = star.z - exclusionCenter.z;
                    float d2 = dx*dx + dy*dy + dz*dz;
                    if (d2 > exclusionRadius*exclusionRadius) break;
                }
            }

            if (!star.active) continue;

            starVertices[i].pos[0] = star.x;
            starVertices[i].pos[1] = star.y;
            starVertices[i].pos[2] = star.z;
            
            float twinkleFactor = 1.0f;
            if (atmosphericTwinkle > 0.0f) {
                twinkleFactor = 0.7f + 0.3f * sin(time * star.twinkle) * atmosphericTwinkle;
            }
            
            float size = star.size * twinkleFactor;
            if (star.isConstellation) {
                size *= 1.2f;
            }
            float dx = star.x - cameraX;
            float dy = star.y - cameraY;
            float dz = star.z - cameraZ;
            float dist = glm::length(glm::vec3(dx, dy, dz));
            float perspectiveScale = glm::clamp(150.0f / glm::max(dist, 1.0f), 0.05f, 20.0f);
            size *= perspectiveScale;
            starVertices[i].size = size;
            
            glm::vec3 starColor = getStarColor(star.temperature);
            float alpha = magnitudeToAlpha(star.magnitude, lightPollution) * twinkleFactor;
            
            starVertices[i].color[0] = starColor.r;
            starVertices[i].color[1] = starColor.g;
            starVertices[i].color[2] = starColor.b;
            starVertices[i].color[3] = alpha;
            
            
            if (star.textureIndex == 0) {
                tex1Indices.push_back(static_cast<uint32_t>(i));
            } else {
                tex2Indices.push_back(static_cast<uint32_t>(i));
            }
        }
        
        if (starVertexBufferMapped) {
            memcpy(starVertexBufferMapped, starVertices, sizeof(starVertices));
        }
    }

    void draw(VkCommandBuffer commandBuffer, uint32_t imageIndex, int screenW, int screenH) {
        if (!initialized || !vkWindow) return;
        
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f;
        lastUpdateTime = currentTime;
        
        update(deltaTime);
        
        glm::vec3 front;
        front.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
        front.y = sin(glm::radians(cameraPitch));
        front.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
        front = glm::normalize(front);
        
        glm::vec3 cameraPos(cameraX, cameraY, cameraZ);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + front, glm::vec3(0.0f, 1.0f, 0.0f));
        
        glm::mat4 proj = glm::perspective(glm::radians(60.0f),
            (float)screenW / (float)screenH, 0.1f, starFieldDepth + 500.0f);
        proj[1][1] *= -1;
        
        mx::UniformBufferObject ubo{};
        ubo.model = glm::mat4(1.0f);
        ubo.view = view;
        ubo.proj = proj;
        ubo.color = glm::vec4(1.0f);
        ubo.params = glm::vec4(SDL_GetTicks() / 1000.0f, 0.0f, 0.0f, 0.0f);
        vkWindow->updateUniformBuffer(imageIndex, ubo);
        
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, starPipeline);
        
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(screenW);
        viewport.height = static_cast<float>(screenH);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        
        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = {static_cast<uint32_t>(screenW), static_cast<uint32_t>(screenH)};
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        
        VkBuffer vertexBuffers[] = {starVertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, starIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
        
        
        if (!tex1Indices.empty() && starIndexBufferMapped) {
            memcpy(starIndexBufferMapped, tex1Indices.data(), tex1Indices.size() * sizeof(uint32_t));
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                starPipelineLayout, 0, 1, &starDescriptorSets[imageIndex], 0, nullptr);
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(tex1Indices.size()), 1, 0, 0, 0);
        }
        
        
        if (!tex2Indices.empty() && starIndexBufferMapped) {
            memcpy(starIndexBufferMapped, tex2Indices.data(), tex2Indices.size() * sizeof(uint32_t));
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                starPipelineLayout, 0, 1, &starDescriptorSets2[imageIndex], 0, nullptr);
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(tex2Indices.size()), 1, 0, 0, 0);
        }
    }

    void drawText(int screenW, int screenH) {}

    void processInput(float deltaTime) {
        if (!keyboardState) return;
        
        glm::vec3 front;
        front.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
        front.y = sin(glm::radians(cameraPitch));
        front.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
        front = glm::normalize(front);
        
        //glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
        
        float velocity = cameraSpeed * deltaTime;
        
        if (keyboardState[SDL_SCANCODE_W]) {
            cameraX += front.x * velocity;
            cameraY += front.y * velocity;
            cameraZ += front.z * velocity;
        }
        if (keyboardState[SDL_SCANCODE_S]) {
            cameraX -= front.x * velocity;
            cameraY -= front.y * velocity;
            cameraZ -= front.z * velocity;
        }
        /*
        if (keyboardState[SDL_SCANCODE_A]) {
            cameraX -= right.x * velocity;
            cameraZ -= right.z * velocity;
        }
        if (keyboardState[SDL_SCANCODE_D]) {
            cameraX += right.x * velocity;
            cameraZ += right.z * velocity;
        }*/
        if (keyboardState[SDL_SCANCODE_Q]) {
            cameraY += velocity;
        }
        if (keyboardState[SDL_SCANCODE_E]) {
            cameraY -= velocity;
        }
        
        float rotSpeed = 90.0f; 
        if (keyboardState[SDL_SCANCODE_LEFT]) {
            cameraYaw -= rotSpeed * deltaTime;
        }
        if (keyboardState[SDL_SCANCODE_RIGHT]) {
            cameraYaw += rotSpeed * deltaTime;
        }
        if (keyboardState[SDL_SCANCODE_UP]) {
            cameraPitch += rotSpeed * deltaTime;
        }
        if (keyboardState[SDL_SCANCODE_DOWN]) {
            cameraPitch -= rotSpeed * deltaTime;
        }
        cameraPitch = glm::clamp(cameraPitch, -89.0f, 89.0f);
    }
};





class VK_ModelRenderer {
public:
    mx::VKWindow* vkWindow = nullptr;
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    
    
    std::vector<ModelVertex> vertices;
    std::vector<uint32_t> indices;
    
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
    
    
    VkImage modelTexture = VK_NULL_HANDLE;
    VkDeviceMemory modelTextureMemory = VK_NULL_HANDLE;
    VkImageView modelTextureView = VK_NULL_HANDLE;
    VkSampler modelSampler = VK_NULL_HANDLE;
    
    
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptorSets;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;
    
    bool initialized = false;
    float rotationX = 0.0f;
    float rotationY = 0.0f;
    float rotationZ = 0.0f;
    float modelScale = 10.0f;  
    glm::vec3 modelPosition = glm::vec3(0.0f, 0.0f, -30.0f);  
    
    void cleanup() {
        if (!initialized || device == VK_NULL_HANDLE) return;
        
        SDL_Log("VK_ModelRenderer::cleanup() - start");
        vkDeviceWaitIdle(device);
        
        if (pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, pipeline, nullptr);
            pipeline = VK_NULL_HANDLE;
        }
        if (pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
            pipelineLayout = VK_NULL_HANDLE;
        }
        if (descriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device, descriptorPool, nullptr);
            descriptorPool = VK_NULL_HANDLE;
        }
        descriptorSets.clear();
        if (descriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
            descriptorSetLayout = VK_NULL_HANDLE;
        }
        if (vertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, vertexBuffer, nullptr);
            vkFreeMemory(device, vertexBufferMemory, nullptr);
            vertexBuffer = VK_NULL_HANDLE;
        }
        if (indexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, indexBuffer, nullptr);
            vkFreeMemory(device, indexBufferMemory, nullptr);
            indexBuffer = VK_NULL_HANDLE;
        }
        if (modelSampler != VK_NULL_HANDLE) {
            vkDestroySampler(device, modelSampler, nullptr);
            modelSampler = VK_NULL_HANDLE;
        }
        if (modelTextureView != VK_NULL_HANDLE) {
            vkDestroyImageView(device, modelTextureView, nullptr);
            modelTextureView = VK_NULL_HANDLE;
        }
        if (modelTexture != VK_NULL_HANDLE) {
            vkDestroyImage(device, modelTexture, nullptr);
            vkFreeMemory(device, modelTextureMemory, nullptr);
            modelTexture = VK_NULL_HANDLE;
        }
        for (size_t i = 0; i < uniformBuffers.size(); ++i) {
            if (uniformBuffers[i] != VK_NULL_HANDLE) {
                vkDestroyBuffer(device, uniformBuffers[i], nullptr);
            }
            if (uniformBuffersMemory[i] != VK_NULL_HANDLE) {
                vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
            }
        }
        uniformBuffers.clear();
        uniformBuffersMemory.clear();
        uniformBuffersMapped.clear();
        
        vertices.clear();
        indices.clear();
        
        SDL_Log("VK_ModelRenderer::cleanup() - complete");
        initialized = false;
        device = VK_NULL_HANDLE;
    }
    
    void init(mx::VKWindow* vkWin, const std::string& dataPath, 
              const std::string& modelFile, const std::string& textureFile,
              float scale = 10.0f, const glm::vec3& position = glm::vec3(0.0f, 0.0f, -30.0f)) {
        if (initialized) return;
        
        vkWindow = vkWin;
        device = vkWin->getDevice();
        physicalDevice = vkWin->getPhysicalDevice();
        commandPool = vkWin->getCommandPool();
        graphicsQueue = vkWin->getGraphicsQueue();
        
        
        modelScale = scale;
        modelPosition = position;
        std::string modelPath = dataPath + "/data/" + modelFile;
        loadMXMOD(modelPath, vertices, indices, modelScale);
        
        
        std::string texPath = dataPath + "/data/" + textureFile;
        loadTexture(texPath);
        
        createDescriptorSetLayout();
        createPipeline(vkWin, dataPath);
        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffers(vkWin);
        createDescriptorPool(vkWin);
        createDescriptorSets(vkWin);
        
        initialized = true;
        SDL_Log("VK_ModelRenderer initialized with %zu vertices", vertices.size());
    }
    
    void loadTexture(const std::string& path) {
        SDL_Surface* img = png::LoadPNG(path.c_str());
        if (!img || !img->pixels) {
            throw mx::Exception("Failed to load model texture: " + path);
        }
        
        int width = img->w;
        int height = img->h;
        VkDeviceSize imageSize = width * height * 4;
        
        
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = imageSize;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        
        VK_CHECK_RESULT(vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer));
        
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, stagingBuffer, &memRequirements);
        
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        
        VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &stagingBufferMemory));
        vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);
        
        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, img->pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(device, stagingBufferMemory);
        
        SDL_FreeSurface(img);
        
        
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = static_cast<uint32_t>(width);
        imageInfo.extent.height = static_cast<uint32_t>(height);
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        
        VK_CHECK_RESULT(vkCreateImage(device, &imageInfo, nullptr, &modelTexture));
        
        vkGetImageMemoryRequirements(device, modelTexture, &memRequirements);
        
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        
        VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &modelTextureMemory));
        vkBindImageMemory(device, modelTexture, modelTextureMemory, 0);
        
        transitionImageLayout(modelTexture, VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copyBufferToImage(stagingBuffer, modelTexture, 
            static_cast<uint32_t>(width), static_cast<uint32_t>(height));
        transitionImageLayout(modelTexture, VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
        
        
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = modelTexture;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        
        VK_CHECK_RESULT(vkCreateImageView(device, &viewInfo, nullptr, &modelTextureView));
        
        
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        
        VK_CHECK_RESULT(vkCreateSampler(device, &samplerInfo, nullptr, &modelSampler));
    }
    
    void transitionImageLayout(VkImage image, VkFormat format,
        VkImageLayout oldLayout, VkImageLayout newLayout) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;
        
        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
        
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        
        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        
        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;
        
        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
            newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
                   newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
            throw mx::Exception("Unsupported layout transition");
        }
        
        vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage,
            0, 0, nullptr, 0, nullptr, 1, &barrier);
        
        vkEndCommandBuffer(commandBuffer);
        
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        
        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);
        
        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }
    
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;
        
        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
        
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        
        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {width, height, 1};
        
        vkCmdCopyBufferToImage(commandBuffer, buffer, image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        
        vkEndCommandBuffer(commandBuffer);
        
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        
        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);
        
        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }
    
    void createDescriptorSetLayout() {
        std::array<VkDescriptorSetLayoutBinding, 2> bindings{};
        
        
        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[0].pImmutableSamplers = nullptr;
        
        
        bindings[1].binding = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[1].pImmutableSamplers = nullptr;
        
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();
        
        VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout));
    }
    
    void createDescriptorPool(mx::VKWindow* vkWin) {
        uint32_t imageCount = vkWin->getSwapChainImageCount();
        
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[0].descriptorCount = imageCount;
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[1].descriptorCount = imageCount;
        
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = imageCount;
        
        VK_CHECK_RESULT(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool));
    }
    
    void createDescriptorSets(mx::VKWindow* vkWin) {
        uint32_t imageCount = vkWin->getSwapChainImageCount();
        std::vector<VkDescriptorSetLayout> layouts(imageCount, descriptorSetLayout);
        
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = imageCount;
        allocInfo.pSetLayouts = layouts.data();
        
        descriptorSets.resize(imageCount);
        VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()));
        
        for (size_t i = 0; i < imageCount; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(mx::UniformBufferObject);
            
            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = modelTextureView;
            imageInfo.sampler = modelSampler;
            
            std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
            
            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pImageInfo = &imageInfo;
            
            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pBufferInfo = &bufferInfo;
            
            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()),
                                   descriptorWrites.data(), 0, nullptr);
        }
    }
    
    void createVertexBuffer() {
        VkDeviceSize bufferSize = sizeof(ModelVertex) * vertices.size();
        
        
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingBufferMemory);
        
        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
        vkUnmapMemory(device, stagingBufferMemory);
        
        
        createBuffer(bufferSize, 
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            vertexBuffer, vertexBufferMemory);
        
        copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
        
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }
    
    void createIndexBuffer() {
        VkDeviceSize bufferSize = sizeof(uint32_t) * indices.size();
        
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingBufferMemory);
        
        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
        vkUnmapMemory(device, stagingBufferMemory);
        
        createBuffer(bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            indexBuffer, indexBufferMemory);
        
        copyBuffer(stagingBuffer, indexBuffer, bufferSize);
        
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    void createUniformBuffers(mx::VKWindow* vkWin) {
        uint32_t imageCount = vkWin->getSwapChainImageCount();
        VkDeviceSize bufferSize = sizeof(mx::UniformBufferObject);
        uniformBuffers.resize(imageCount);
        uniformBuffersMemory.resize(imageCount);
        uniformBuffersMapped.resize(imageCount);

        for (size_t i = 0; i < imageCount; ++i) {
            createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                uniformBuffers[i], uniformBuffersMemory[i]);
            vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
        }
    }

    void updateModelUniformBuffer(uint32_t imageIndex, const mx::UniformBufferObject& ubo) {
        if (uniformBuffersMapped.size() > imageIndex && uniformBuffersMapped[imageIndex] != nullptr) {
            memcpy(uniformBuffersMapped[imageIndex], &ubo, sizeof(ubo));
        }
    }
    
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, 
                      VkMemoryPropertyFlags properties,
                      VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        
        VK_CHECK_RESULT(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer));
        
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);
        
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
        
        VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory));
        vkBindBufferMemory(device, buffer, bufferMemory, 0);
    }
    
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;
        
        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
        
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        
        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        
        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
        
        vkEndCommandBuffer(commandBuffer);
        
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        
        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);
        
        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }
    
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
        
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        
        throw mx::Exception("Failed to find suitable memory type");
    }
    
    void createPipeline(mx::VKWindow* vkWin, const std::string& dataPath) {
        
        auto vertShaderCode = mx::readFile(dataPath + "/data/vert.spv");
        auto fragShaderCode = mx::readFile(dataPath + "/data/frag.spv");
        
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = vertShaderCode.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(vertShaderCode.data());
        
        VkShaderModule vertShaderModule;
        VK_CHECK_RESULT(vkCreateShaderModule(device, &createInfo, nullptr, &vertShaderModule));
        
        createInfo.codeSize = fragShaderCode.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(fragShaderCode.data());
        
        VkShaderModule fragShaderModule;
        VK_CHECK_RESULT(vkCreateShaderModule(device, &createInfo, nullptr, &fragShaderModule));
        
        VkPipelineShaderStageCreateInfo shaderStages[2] = {};
        shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStages[0].module = vertShaderModule;
        shaderStages[0].pName = "main";
        
        shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStages[1].module = fragShaderModule;
        shaderStages[1].pName = "main";
        
        
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(ModelVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
        
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(ModelVertex, pos);
        
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(ModelVertex, texCoord);
        
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(ModelVertex, normal);
        
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
        
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;
        
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)vkWin->getSwapChainExtent().width;
        viewport.height = (float)vkWin->getSwapChainExtent().height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        
        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = vkWin->getSwapChainExtent();
        
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();
        
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;
        
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE;  
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        
        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;
        
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
        
        VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout));
        
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = vkWin->getRenderPass();
        pipelineInfo.subpass = 0;
        
        VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));
        
        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }
    
    void update(float deltaTime) {
        rotationX += 10.0f * deltaTime;
        rotationY += 30.0f * deltaTime;
        rotationZ += 5.0f * deltaTime;
        if (rotationX >= 360.0f) rotationX -= 360.0f;
        if (rotationY >= 360.0f) rotationY -= 360.0f;
        if (rotationZ >= 360.0f) rotationZ -= 360.0f;
    }
    
    void draw(VkCommandBuffer commandBuffer, uint32_t imageIndex, int screenW, int screenH,
              const glm::vec3& cameraPos, float cameraYaw, float cameraPitch) {
        if (!initialized || !vkWindow) {
            SDL_Log("VK_ModelRenderer::draw() - not initialized or no window");
            return;
        }
    
        
        glm::vec3 front;
        front.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
        front.y = sin(glm::radians(cameraPitch));
        front.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
        front = glm::normalize(front);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + front, glm::vec3(0.0f, 1.0f, 0.0f));
        
        
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, modelPosition);
        model = glm::rotate(model, glm::radians(rotationX), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotationZ), glm::vec3(0.0f, 0.0f, 1.0f));
        
        
        glm::mat4 proj = glm::perspective(glm::radians(60.0f),
            (float)screenW / (float)screenH, 0.1f, 10000.0f);
        proj[1][1] *= -1; 
        
        mx::UniformBufferObject ubo{};
        ubo.model = model;
        ubo.view = view;
        ubo.proj = proj;
        ubo.color = glm::vec4(1.0f);
        ubo.params = glm::vec4(SDL_GetTicks() / 1000.0f, 0.0f, 0.0f, 0.0f);
        updateModelUniformBuffer(imageIndex, ubo);
        
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(screenW);
        viewport.height = static_cast<float>(screenH);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        
        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = {static_cast<uint32_t>(screenW), static_cast<uint32_t>(screenH)};
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        
        VkBuffer vertexBuffers[] = {vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout, 0, 1, &descriptorSets[imageIndex], 0, nullptr);
        
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    }
};

#endif 

class StarfieldApp : public cross::EventHandler {
public:
#ifdef WITH_GL
    std::unique_ptr<GL_Starfield> glStarfield;
    std::unique_ptr<GL_ModelRenderer> glModelRenderer;
#endif
#ifdef WITH_VK
    std::unique_ptr<VK_Starfield> vkStarfield;
    std::unique_ptr<VK_ModelRenderer> vkModelRenderer;
#endif
    
    bool mouseGrabbed = false;
    int lastMouseX = 0, lastMouseY = 0;
    Uint64 lastFrameTime = 0;
    bool showModel = true;  
    
    void load() override {
        SDL_Log("StarfieldApp::load()");
        lastFrameTime = SDL_GetPerformanceCounter();
        
        std::string path = window->getPath();
        
#ifdef WITH_VK
        auto* vkWin = window->as<cross::VK_Window>();
        if (vkWin) {
            SDL_Log("Using Vulkan renderer");
            vkStarfield = std::make_unique<VK_Starfield>();
            vkStarfield->init(vkWin, path);

            
            vkModelRenderer = std::make_unique<VK_ModelRenderer>();
            glm::vec3 vkModelPos = glm::vec3(0.0f, 0.0f, -6.0f);  
            float vkModelScale = 1.5f;  
            vkModelRenderer->init(vkWin, path, "geosphere.mxmod", "bg.png", vkModelScale, vkModelPos);

            
            vkStarfield->cameraX = 0.0f;
            vkStarfield->cameraY = 0.0f;
            vkStarfield->cameraZ = -3.0f;
            vkStarfield->cameraYaw = -90.0f;  
            vkStarfield->cameraPitch = 0.0f;  

            
            vkStarfield->exclusionCenter = vkModelPos;
            vkStarfield->exclusionRadius = 15.0f * vkModelScale;
            vkStarfield->initStars();
            return;
        }
#endif
#ifdef WITH_GL
        auto* glWin = window->as<cross::GL_Window>();
        if (glWin) {
            SDL_Log("Using OpenGL renderer");
            glStarfield = std::make_unique<GL_Starfield>();
            glStarfield->init(path);

            
            glModelRenderer = std::make_unique<GL_ModelRenderer>();
            glm::vec3 modelPos = glm::vec3(0.0f, 0.0f, -6.0f);  
            float modelScale = 1.5f;  
            glModelRenderer->init(path, "sphere.mxmod", "bg.png", modelScale, modelPos);    
            glStarfield->cameraX = 0.0f;
            glStarfield->cameraY = 0.0f;
            glStarfield->cameraZ = -3.0f;
            glStarfield->cameraYaw = -90.0f;  
            glStarfield->cameraPitch = 0.0f;  

            
            glStarfield->exclusionCenter = modelPos;
            glStarfield->exclusionRadius = 15.0f * modelScale;
            glStarfield->initStars();
            return;
        }
#endif
        SDL_Log("Warning: No renderer initialized!");
    }
    
    void draw() override {
        int w = window->getWidth();
        int h = window->getHeight();
        
#ifdef WITH_VK
        if (vkStarfield) {
            auto* vkWin = window->as<cross::VK_Window>();
            if (vkWin) {
                VkCommandBuffer cmd = vkWin->getCurrentCommandBuffer();
                uint32_t imageIndex = vkWin->getCurrentImageIndex();
                
                
                vkStarfield->draw(cmd, imageIndex, w, h);
                vkStarfield->drawText(w, h);
                
                if (showModel && vkModelRenderer) {
                    glm::vec3 camPos(vkStarfield->cameraX, vkStarfield->cameraY, vkStarfield->cameraZ);
                    vkModelRenderer->draw(cmd, imageIndex, w, h, camPos, vkStarfield->cameraYaw, vkStarfield->cameraPitch);
                    vkStarfield->modelPosToDisplay = vkModelRenderer->modelPosition;
                }
            }
            return;
        }
#endif
#ifdef WITH_GL
        if (glStarfield) {
            
            if (showModel && glModelRenderer) {
                glm::vec3 camPos(glStarfield->cameraX, glStarfield->cameraY, glStarfield->cameraZ);
                glModelRenderer->draw(w, h, camPos, glStarfield->cameraYaw, glStarfield->cameraPitch);
                glStarfield->modelPosToDisplay = glModelRenderer->modelPosition;
            }
            
            
            glStarfield->draw(w, h);
            
            return;
        }
#endif
    }
    
    void event(SDL_Event &e) override {
        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_ESCAPE:
                    window->quit();
                    break;
                case SDLK_g:
                    mouseGrabbed = !mouseGrabbed;
                    SDL_SetRelativeMouseMode(mouseGrabbed ? SDL_TRUE : SDL_FALSE);
                    break;
                case SDLK_m:  
                    showModel = !showModel;
                    SDL_Log("Model visibility: %s", showModel ? "ON" : "OFF");
                    break;
                case SDLK_UP:
#ifdef WITH_VK
                    if (vkStarfield) vkStarfield->atmosphericTwinkle = std::min(1.0f, vkStarfield->atmosphericTwinkle + 0.1f);
#endif
#ifdef WITH_GL
                    if (glStarfield) glStarfield->atmosphericTwinkle = std::min(1.0f, glStarfield->atmosphericTwinkle + 0.1f);
#endif
                    break;
                case SDLK_DOWN:
#ifdef WITH_VK
                    if (vkStarfield) vkStarfield->atmosphericTwinkle = std::max(0.0f, vkStarfield->atmosphericTwinkle - 0.1f);
#endif
#ifdef WITH_GL
                    if (glStarfield) glStarfield->atmosphericTwinkle = std::max(0.0f, glStarfield->atmosphericTwinkle - 0.1f);
#endif
                    break;
            }
        }
        
        if (e.type == SDL_MOUSEMOTION && mouseGrabbed) {
            float sensitivity = 0.1f;
            float xOffset = e.motion.xrel * sensitivity;
            float yOffset = -e.motion.yrel * sensitivity;
            
#ifdef WITH_VK
            if (vkStarfield) {
                vkStarfield->cameraYaw += xOffset;
                vkStarfield->cameraPitch += yOffset;
                if (vkStarfield->cameraPitch > 89.0f) vkStarfield->cameraPitch = 89.0f;
                if (vkStarfield->cameraPitch < -89.0f) vkStarfield->cameraPitch = -89.0f;
            }
#endif
#ifdef WITH_GL
            if (glStarfield) {
                glStarfield->cameraYaw += xOffset;
                glStarfield->cameraPitch += yOffset;
                if (glStarfield->cameraPitch > 89.0f) glStarfield->cameraPitch = 89.0f;
                if (glStarfield->cameraPitch < -89.0f) glStarfield->cameraPitch = -89.0f;
            }
#endif
        }
    }
    
    void update(float deltaTime) override {
#ifdef WITH_VK
        if (vkStarfield) {
            vkStarfield->processInput(deltaTime);
        }
        if (vkModelRenderer) {
            vkModelRenderer->update(deltaTime);
        }
#endif
#ifdef WITH_GL
        if (glStarfield) {
            glStarfield->processInput(deltaTime);
        }
        if (glModelRenderer) {
            glModelRenderer->update(deltaTime);
        }
#endif
    }
    
    void cleanup() override {
        SDL_Log("StarfieldApp::cleanup()");
#ifdef WITH_VK
        if (vkModelRenderer) {
            vkModelRenderer->cleanup();
            vkModelRenderer.reset();
        }
        if (vkStarfield) {
            vkStarfield->cleanup();
            vkStarfield.reset();
        }
#endif
#ifdef WITH_GL
        if (glModelRenderer) {
            glModelRenderer->cleanup();
            glModelRenderer.reset();
        }
        if (glStarfield) {
            glStarfield->cleanup();
            glStarfield.reset();
        }
#endif
    }
};

int main(int argc, char **argv) {
    Arguments args = proc_args(argc, argv);
    
    try {
        auto window = cross::createWindow();
        StarfieldApp app;
        
        window->setHandler(&app);
        window->init("Cross-Platform Starfield 3D with Sphere Model", args.path, args.width, args.height, args.fullscreen);
        window->loop();
        window->cleanup();
        
    } catch (mx::Exception &e) {
        SDL_Log("mx: Exception: %s", e.text().c_str());
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
