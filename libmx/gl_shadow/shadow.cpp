#include"mx.hpp"
#include"argz.hpp"
#include <iostream> 

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <GLES3/gl3.h>
#endif

#include"gl.hpp"
#include"loadpng.hpp"
#include"model.hpp"

#define CHECK_GL_ERROR() \
{ GLenum err = glGetError(); \
if (err != GL_NO_ERROR) \
std::cerr << "OpenGL Error: " << err << " at " << __FILE__ << ":" << __LINE__ << std::endl; }

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#if defined(__EMSCRIPTEN__) || defined(__ANDROID__)

const char *main_vSource = R"(#version 300 es
precision highp float;
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform vec3 objectColor;

out vec3 vertexColor;
out vec2 TexCoords;
out vec4 FragPosLightSpace;
out vec3 FragPos;
out vec3 Normal;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;
    gl_Position = projection * view * worldPos;
    Normal = normalize(mat3(transpose(inverse(model))) * aNormal);
    
    // Calculate position in light space for shadow calculation
    FragPosLightSpace = lightSpaceMatrix * worldPos;
    
    // Calculate basic lighting (we'll add shadows in fragment shader)
    vec3 norm = Normal;
    vec3 lightDir = normalize(lightPos - FragPos);
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    float specularStrength = 1.0;
    float shininess = 64.0;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;
    vec3 finalColor = (ambient + diffuse + specular) * objectColor;
    vertexColor = finalColor;
    TexCoords = aTexCoords;
}
)";

const char *main_fSource = R"(#version 300 es
precision highp float;
in vec3 vertexColor;
in vec2 TexCoords;
in vec4 FragPosLightSpace;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D texture1;
uniform sampler2D shadowMap;
uniform vec3 lightPos;

out vec4 FragColor;

float ShadowCalculation(vec4 fragPosLightSpace)
{
    // Perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    
    // Manual border handling since OpenGL ES doesn't support GL_CLAMP_TO_BORDER
    if(projCoords.x < 0.0 || projCoords.x > 1.0 || 
       projCoords.y < 0.0 || projCoords.y > 1.0 ||
       projCoords.z < 0.0 || projCoords.z > 1.0) {
        return 0.0; // Not in shadow when outside the light's frustum
    }
    
    // Get closest depth value from light's perspective
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    
    // Get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    
    // Check if fragment is in shadow
    vec3 normal = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    
    // PCF (percentage-closer filtering)
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    return shadow;
}

void main()
{
    // Calculate shadow
    float shadow = ShadowCalculation(FragPosLightSpace);
    
    // Final color with shadow
    vec3 lighting = vertexColor * (1.0 - shadow * 0.6); // Reduce shadow intensity by 40%
    
    FragColor = vec4(lighting, 1.0) * texture(texture1, TexCoords);
}
)";


const char *shadow_vSource = R"(#version 300 es
precision highp float;
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 lightSpaceMatrix;
uniform mat4 model;

void main()
{
    gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
}
)";

const char *shadow_fSource = R"(#version 300 es
precision highp float;
layout(location = 0) out vec4 FragColor;
void main()
{
    // We need to output a color even though we're only using the depth
    FragColor = vec4(1.0, 1.0, 1.0, 1.0); 
}
)";
#else
const char *main_vSource = R"(#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform vec3 objectColor;

out vec3 vertexColor;
out vec2 TexCoords;
out vec4 FragPosLightSpace;
out vec3 FragPos;
out vec3 Normal;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;
    gl_Position = projection * view * worldPos;
    Normal = normalize(mat3(transpose(inverse(model))) * aNormal);
    
    // Calculate position in light space for shadow calculation
    FragPosLightSpace = lightSpaceMatrix * worldPos;
    
    // Calculate basic lighting (we'll add shadows in fragment shader)
    vec3 norm = Normal;
    vec3 lightDir = normalize(lightPos - FragPos);
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    float specularStrength = 1.0;
    float shininess = 64.0;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;
    vec3 finalColor = (ambient + diffuse + specular) * objectColor;
    vertexColor = finalColor;
    TexCoords = aTexCoords;
}
)";

const char *main_fSource = R"(#version 330 core
in vec3 vertexColor;
in vec2 TexCoords;
in vec4 FragPosLightSpace;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D texture1;
uniform sampler2D shadowMap;
uniform vec3 lightPos;

out vec4 FragColor;

float ShadowCalculation(vec4 fragPosLightSpace)
{
    // Perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    
    // Get closest depth value from light's perspective
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    
    // Get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    
    // Check if fragment is in shadow
    vec3 normal = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    
    // PCF (percentage-closer filtering)
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    // Keep shadow at 0.0 when outside the light's frustum
    if(projCoords.z > 1.0)
        shadow = 0.0;
        
    return shadow;
}

void main()
{
    // Calculate shadow
    float shadow = ShadowCalculation(FragPosLightSpace);
    
    // Final color with shadow
    vec3 lighting = vertexColor * (1.0 - shadow * 0.6); // Reduce shadow intensity by 40%
    
    FragColor = vec4(lighting, 1.0) * texture(texture1, TexCoords);
}
)";

const char *shadow_vSource = R"(#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 lightSpaceMatrix; // Light's view-projection matrix
uniform mat4 model;

void main()
{
    gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
}
)";

const char *shadow_fSource = R"(#version 330 core
void main()
{
    // No need to write anything - depth is automatically written
    // FragDepth = gl_FragCoord.z;
}
)";
#endif


class Game : public gl::GLObject {
public:
    Game() = default;
    virtual ~Game() override {
        if (shadowMapFBO != 0) glDeleteFramebuffers(1, &shadowMapFBO);
        if (shadowMap != 0) glDeleteTextures(1, &shadowMap);
    #ifdef __EMSCRIPTEN__
        if (colorAttachment != 0) glDeleteTextures(1, &colorAttachment);
    #endif
    }

    void load(gl::GLWindow *win) override {
        font.loadFont(win->util.getFilePath("data/font.ttf"),24);
        if(object_model.openModel(win->util.getFilePath("data/star.mxmod.z")) == false) {
            throw mx::Exception("Failed to load model");
        }   
        if(shader.loadProgramFromText(main_vSource, main_fSource) == false) {
            throw mx::Exception("Failed to load shader");
        }   
        if(shadow_shader.loadProgramFromText(shadow_vSource, shadow_fSource) == false) {
            throw mx::Exception("Failed to load shadow shader");
        }       
        object_model.setTextures(win, win->util.getFilePath("data/star.tex"), win->util.getFilePath("data"));
        object_model.setShaderProgram(&shader);
        
        createShadowMap(SHADOW_WIDTH, SHADOW_HEIGHT);
    }
    
    void createShadowMap(unsigned int width, unsigned int height) {
        glGenFramebuffers(1, &shadowMapFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
        
    #ifdef __EMSCRIPTEN__
        glGenTextures(1, &colorAttachment);
        glBindTexture(GL_TEXTURE_2D, colorAttachment);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorAttachment, 0);
        glGenTextures(1, &shadowMap);
        glBindTexture(GL_TEXTURE_2D, shadowMap);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);
        
        const GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
        glDrawBuffers(1, drawBuffers);
    #else
        glGenTextures(1, &shadowMap);
        glBindTexture(GL_TEXTURE_2D, shadowMap);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
        
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    #endif

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Framebuffer not complete! Status code: " << status << std::endl;
            switch (status) {
                case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                    std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT" << std::endl;
                    break;
                case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                    std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT" << std::endl;
                    break;
                case GL_FRAMEBUFFER_UNSUPPORTED:
                    std::cerr << "GL_FRAMEBUFFER_UNSUPPORTED" << std::endl;
                    break;
                default:
                    std::cerr << "Unknown framebuffer issue" << std::endl;
                    break;
            }
        }    
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        CHECK_GL_ERROR();
    }

    void draw(gl::GLWindow *win) override {
        glDisable(GL_DEPTH_TEST);
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; 
        lastUpdateTime = currentTime;
        glEnable(GL_DEPTH_TEST);
        static float rotationAngle = 0.0f;
        rotationAngle += deltaTime * 30.0f; 
        
        cameraPosition = glm::vec3(0.0f, 2.0f, 5.0f);
        glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);

        modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::scale(modelMatrix, glm::vec3(2.5f, 2.5f, 2.5f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationAngle), glm::vec3(1.0f, 1.0f, 0.0f));
        
        viewMatrix = glm::lookAt(cameraPosition, cameraTarget, upVector);
        
        float aspectRatio = static_cast<float>(win->w) / static_cast<float>(win->h);
        projectionMatrix = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);

        lightPos = glm::vec3(3.0f, 5.0f, -3.0f); 
        glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        float orthoSize = 10.0f;
        glm::mat4 lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, 0.1f, 20.0f);
        glm::mat4 lightSpaceMatrix = lightProjection * lightView;
        
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
        
        
        GLenum shadowStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (shadowStatus != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Shadow framebuffer not complete! Status: " << shadowStatus << std::endl;
        }
        
#ifdef __EMSCRIPTEN__
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#else
        glClear(GL_DEPTH_BUFFER_BIT);
#endif
        
        shadow_shader.useProgram();
        glUniformMatrix4fv(glGetUniformLocation(shadow_shader.id(), "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shadow_shader.id(), "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
        glCullFace(GL_FRONT);  
        object_model.setShaderProgram(&shadow_shader);
        for(auto &m : object_model.meshes) {
            m.draw();
        }
        glCullFace(GL_BACK);   
        
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        
        GLenum defaultStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (defaultStatus != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Default framebuffer not complete! Status: " << defaultStatus << std::endl;
        }
        
        
        CHECK_GL_ERROR(); 
        
        glViewport(0, 0, win->w, win->h);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        object_model.setShaderProgram(&shader, "texture1");
        shader.useProgram();
        CHECK_GL_ERROR(); 
    
        GLint modelLoc = glGetUniformLocation(shader.id(), "model");
        GLint viewLoc = glGetUniformLocation(shader.id(), "view");
        GLint projLoc = glGetUniformLocation(shader.id(), "projection");
        GLint lightSpaceMatrixLoc = glGetUniformLocation(shader.id(), "lightSpaceMatrix");
        GLint lightPosLoc = glGetUniformLocation(shader.id(), "lightPos");
        GLint viewPosLoc = glGetUniformLocation(shader.id(), "viewPos");
        GLint lightColorLoc = glGetUniformLocation(shader.id(), "lightColor");
        GLint objectColorLoc = glGetUniformLocation(shader.id(), "objectColor");
        GLint shadowMapLoc = glGetUniformLocation(shader.id(), "shadowMap");
        
        if (modelLoc == -1 || viewLoc == -1 || projLoc == -1 || 
            lightSpaceMatrixLoc == -1 || shadowMapLoc == -1) {
            std::cerr << "Warning: Some required uniforms not found in shader" << std::endl;
        }
        
        if (modelLoc != -1)
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
        if (viewLoc != -1)
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
        if (projLoc != -1)
            glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
        if (lightSpaceMatrixLoc != -1)
            glUniformMatrix4fv(lightSpaceMatrixLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
        
        if (lightPosLoc != -1)
            glUniform3f(lightPosLoc, lightPos.x, lightPos.y, lightPos.z);
        if (viewPosLoc != -1)
            glUniform3f(viewPosLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);
        if (lightColorLoc != -1)
            glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);
        if (objectColorLoc != -1)
            glUniform3f(objectColorLoc, 1.0f, 1.0f, 1.0f);
        
        CHECK_GL_ERROR(); 
    
        if (shadowMap != 0 && shadowMapLoc != -1) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, shadowMap);
            glUniform1i(shadowMapLoc, 1);
        }
        
        object_model.drawArrays();
        CHECK_GL_ERROR();
        
        lightPos.x = lightRadius * cos(lightAngle);
        lightPos.z = lightRadius * sin(lightAngle);
        lightPos.y = lightHeight;
        win->text.setColor({255, 0, 0, 255});
        win->text.printText_Solid(font, 25.0f, 25.0f, "Shadow Mapping Demo: " + std::to_string(currentTime));
        win->text.printText_Solid(font, 25.0f, 60.0f, "Light: X=" + std::to_string(lightPos.x).substr(0,4) + 
                                                  " Y=" + std::to_string(lightPos.y).substr(0,4) + 
                                                  " Z=" + std::to_string(lightPos.z).substr(0,4));
        win->text.printText_Solid(font, 25.0f, 95.0f, "Use arrow keys to move light, W/S to adjust radius");

        update(deltaTime);
    }
    
    void event(gl::GLWindow *win, SDL_Event &e) override {
        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_LEFT:
                    lightAngle += 0.1f;
                    break;
                case SDLK_RIGHT:
                    lightAngle -= 0.1f;
                    break;
                case SDLK_UP:
                    lightHeight += 0.5f;
                    break;
                case SDLK_DOWN:
                    lightHeight = std::max(1.0f, lightHeight - 0.5f);
                    break;
                case SDLK_w:
                    lightRadius += 0.5f;
                    break;
                case SDLK_s:
                    lightRadius = std::max(1.0f, lightRadius - 0.5f);
                    break;
            }
        }
    }
    void update(float deltaTime) {}
    
private:
    mx::Font font;
    mx::Model object_model;
    gl::ShaderProgram shader;
    gl::ShaderProgram shadow_shader;
    Uint32 lastUpdateTime = SDL_GetTicks();
    glm::mat4 modelMatrix{1.0f};
    glm::mat4 viewMatrix{1.0f};
    glm::mat4 projectionMatrix{1.0f};
    glm::vec3 cameraPosition{0.0f, 2.0f, 5.0f};
    glm::vec3 lightPos{0.0f, 5.0f, 0.0f};
    GLuint shadowMapFBO = 0;
    GLuint shadowMap = 0;
    const unsigned int SHADOW_WIDTH = 1024;
    const unsigned int SHADOW_HEIGHT = 1024;
    float lightRadius = 3.0f;
    float lightAngle = 0.0f;
    float lightHeight = 5.0f;
#ifdef __EMSCRIPTEN__
    GLuint colorAttachment = 0;
#endif
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("Skeleton", tw, th) {
        setPath(path);
        setObject(new Game());
        object->load(this);
    }
    
    ~MainWindow() override {}
    
    virtual void event(SDL_Event &e) override {}
    
    virtual void draw() override {
        glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
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
    MainWindow main_window("/", 1920, 1080);
    main_w =&main_window;
    emscripten_set_main_loop(eventProc, 0, 1);
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
