#include"mx.hpp"
#include"argz.hpp"
#include <emscripten/emscripten.h>
#include <emscripten/bind.h>
#include <GLES3/gl3.h>
#include"gl.hpp"
#include"loadpng.hpp"
#include"model.hpp"
#include<iostream>
#include<sstream>


class JSLogBuffer : public std::streambuf {
public:
    JSLogBuffer() {}
protected:
    virtual int overflow(int c) override {
        if (c != traits_type::eof()) {
            buffer += static_cast<char>(c);
            if (c == '\n') {
                flush_to_js();
            }
        }
        return c;
    }
    
    virtual int sync() override {
        if (!buffer.empty()) {
            flush_to_js();
        }
        return 0;
    }
private:
    std::string buffer;
    void flush_to_js() {
        if (!buffer.empty()) {
            EM_ASM({
                if (typeof window.addLogMessage === 'function') {
                    window.addLogMessage(UTF8ToString($0));
                } else {
                    console.log(UTF8ToString($0));
                }
            }, buffer.c_str());
            buffer.clear();
        }
    }
};

static JSLogBuffer jsLogBuffer;
static std::ostream jsLogStream(&jsLogBuffer);

#define CHECK_GL_ERROR() \
{ GLenum err = glGetError(); \
if (err != GL_NO_ERROR) \
printf("OpenGL Error: %d at %s:%d\n", err, __FILE__, __LINE__); }

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const char *szFragment = R"(#version 300 es
precision highp float;
in vec3 fragPos;       
in vec3 fragNormal;    
in vec2 fragTexCoord;  

out vec4 fragColor;

uniform sampler2D texture1; 
uniform vec3 lightPos;
uniform vec3 lightColor;

void main() {
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * lightColor;
    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(lightPos-fragPos);  
    vec3 viewDir = normalize(-fragPos); 
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    float specularStrength = 0.2;
    vec3 reflectDir = reflect(-lightDir, norm); 
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0); 
    vec3 specular = specularStrength * spec * lightColor;
    vec3 textureColor = texture(texture1, fragTexCoord).rgb; 
    vec3 result = (ambient + diffuse + specular) * textureColor;
    fragColor = vec4(result, 1.0); 
}
)";

const char *szVertex = R"(#version 300 es
precision highp float;

layout(location = 0) in vec3 position;   
layout(location = 1) in vec3 normal;     
layout(location = 2) in vec2 texCoord;   

out vec3 fragPos;       
out vec3 fragNormal;    
out vec2 fragTexCoord;  

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    vec4 viewPos4 = view * model * vec4(position, 1.0);
    fragPos = viewPos4.xyz;
    fragNormal = mat3(transpose(inverse(view * model))) * normal;
    
    fragTexCoord = texCoord;                                 
    gl_Position = projection * view * vec4(fragPos, 1.0);    
})";

class ModelViewer : public gl::GLObject {
public:

    float zoom = 45.0f;
    bool compressModel = true;

    ModelViewer() = default;
    virtual ~ModelViewer() override {}

    void load(gl::GLWindow *win) override {
        if(!shader.loadProgramFromText(szVertex, szFragment)) {
            throw mx::Exception("Error could not load shader.");
        }
        loadObject(win, win->util.getFilePath("data/skybox_star.mxmod"), win->util.getFilePath("data/metal.tex"), win->util.getFilePath("data"));
    }

    void loadObject(gl::GLWindow *win, const std::string &filename, const std::string &texture_file, const std::string &texture_path) {
        obj.reset(new mx::Model());
        if(!obj->openModel(filename, compressModel)) {
            throw mx::Exception("Could not load model file: " + filename);
        }
        
        modelMin = glm::vec3(std::numeric_limits<float>::max());
        modelMax = glm::vec3(std::numeric_limits<float>::lowest());
        
        mx::system_out << "mx: Loaded Meshes:  " << obj->meshes.size() << "\n";     
        for (const auto &mesh : obj->meshes) {
            for (size_t i = 0; i < mesh.vert.size(); i += 3) {
                float x = mesh.vert[i];
                float y = mesh.vert[i + 1];
                float z = mesh.vert[i + 2];
                modelMin.x = std::min(modelMin.x, x);
                modelMin.y = std::min(modelMin.y, y);
                modelMin.z = std::min(modelMin.z, z);
                modelMax.x = std::max(modelMax.x, x);
                modelMax.y = std::max(modelMax.y, y);
                modelMax.z = std::max(modelMax.z, z);
            }
        }
        modelCenter = (modelMin + modelMax) * 0.5f;
        glm::vec3 extent = modelMax - modelMin;
        modelRadius = glm::length(extent) * 0.5f;
        
        if (modelRadius < 0.001f) {
            modelRadius = 1.0f;
        }
        
        float fovRadians = glm::radians(45.0f);
        baseCameraDistance = (modelRadius / std::tan(fovRadians / 2.0f)) * 1.5f;
        
        
        outsideCameraPos = glm::vec3(modelCenter.x, modelCenter.y, modelCenter.z + baseCameraDistance);
        zoom = 45.0f;
        outsideFOV = zoom;
        
        
        std::get<0>(rot_x) = 0.0f;
        std::get<0>(rot_y) = 0.0f;
        std::get<0>(rot_z) = 0.0f;
        
        mx::system_out << "mx: Model bounds: (" << modelMin.x << ", " << modelMin.y << ", " << modelMin.z << ") to ("
                       << modelMax.x << ", " << modelMax.y << ", " << modelMax.z << ")\n";
        mx::system_out << "mx: Model center: (" << modelCenter.x << ", " << modelCenter.y << ", " << modelCenter.z << ")\n";
        mx::system_out << "mx: Model radius: " << modelRadius << ", Camera distance: " << baseCameraDistance << "\n";
        shader.useProgram();
        CHECK_GL_ERROR();
        obj->setShaderProgram(&shader, "texture1");
        
        float nearPlane = modelRadius * 0.01f;
        float farPlane = baseCameraDistance * 10.0f;
        
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = glm::lookAt(
            outsideCameraPos,  
            modelCenter,  
            glm::vec3(0.0f, 1.0f, 0.0f)   
        );

        glm::mat4 projection = glm::perspective(glm::radians(zoom), (float)win->w / win->h, nearPlane, farPlane);
        shader.setUniform("model", model);
        shader.setUniform("view", view);
        shader.setUniform("projection", projection);

        if(texture_file.find(".png") != std::string::npos) {
            texture = gl::loadTexture(texture_file);
            std::vector<GLuint> textures {static_cast<unsigned int>(texture)};
            obj->setTextures(textures);
        } else {
            mx::system_out << "mx: Texture file: " << texture_file <<  " : " << texture_path << "\n";
            obj->setTextures(win, texture_file, texture_path);
        }
    }

    void draw(gl::GLWindow *win) override {
        glDisable(GL_CULL_FACE);
        glm::vec3 cameraPos;
        float fovDegrees;
        if (insideCube) {
            lookDirection.x = cos(glm::radians(cameraPitch)) * cos(glm::radians(cameraYaw));
            lookDirection.y = sin(glm::radians(cameraPitch));
            lookDirection.z = cos(glm::radians(cameraPitch)) * sin(glm::radians(cameraYaw));
            lookDirection = glm::normalize(lookDirection);
            cameraPos = glm::vec3(0.0f, 0.0f, 0.0f);
            fovDegrees = 200.0f;  
        } else {
            cameraPos = outsideCameraPos;
            fovDegrees = outsideFOV;
        }
        glm::vec3 cameraTarget = insideCube ? cameraPos + lookDirection : glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::mat4 viewMatrix = glm::lookAt(cameraPos, cameraTarget, cameraUp);
        
        float nearPlane = insideCube ? 0.01f : std::max(0.001f, modelRadius * 0.01f);
        float farPlane = baseCameraDistance * 20.0f;
        
        glm::mat4 projectionMatrix = glm::perspective(
            glm::radians(fovDegrees),
            static_cast<float>(win->w) / static_cast<float>(win->h),
            nearPlane,
            farPlane
        );
        if (insideCube) {
            glFrontFace(GL_CW);
        } else {
            glFrontFace(GL_CCW);
        }
        static Uint32 lastTime = emscripten_get_now();
        float currentTime = emscripten_get_now();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;
        shader.useProgram();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        shader.setUniform("texture1", 0);
     
        if (std::get<1>(rot_x)) {
            std::get<0>(rot_x) += 50.0f * deltaTime;
            if (std::get<0>(rot_x) >= 360.0f) {
                std::get<0>(rot_x) -= 360.0f;
            }
        }if (std::get<1>(rot_y)) {
            std::get<0>(rot_y) += 50.0f * deltaTime;
            if (std::get<0>(rot_y) >= 360.0f) {
                std::get<0>(rot_y) -= 360.0f;
            }
        }if (std::get<1>(rot_z)) {
            std::get<0>(rot_z) += 50.0f * deltaTime;
            if (std::get<0>(rot_z) >= 360.0f) {
                std::get<0>(rot_z) -= 360.0f;
            }
        }
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, -modelCenter);
        modelMatrix = glm::rotate(modelMatrix, glm::radians(std::get<0>(rot_x)), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(std::get<0>(rot_y)), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(std::get<0>(rot_z)), glm::vec3(0.0f, 0.0f, 1.0f));
        glm::vec3 lightPos(std::get<0>(light), std::get<1>(light), std::get<2>(light));
        glm::vec4 lightPosView = viewMatrix * glm::vec4(lightPos, 1.0f);
        shader.setUniform("lightPos", glm::vec3(lightPosView));
        glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
        shader.setUniform("model", modelMatrix);
        shader.setUniform("view", viewMatrix);
        shader.setUniform("projection", projectionMatrix);
        shader.setUniform("lightColor", lightColor);
        obj->drawArrays();
        glBindVertexArray(0);
    }
    
    void event(gl::GLWindow *win, SDL_Event &e) override {}

    void update(float deltaTime) {}
    
    void setRotX(bool enabled) {
        std::get<1>(rot_x) = enabled;
        if (!enabled) std::get<0>(rot_x) = 0.0f;
    }
    void setRotY(bool enabled) {
        std::get<1>(rot_y) = enabled;
        if (!enabled) std::get<0>(rot_y) = 0.0f;
    }
    void setRotZ(bool enabled) {
        std::get<1>(rot_z) = enabled;
        if (!enabled) std::get<0>(rot_z) = 0.0f;
    }
    
    void setZoom(float zoomFactor) {
        float clampedZoom = glm::max(0.1f, glm::min(5.0f, zoomFactor));
        outsideCameraPos.z = modelCenter.z + baseCameraDistance * clampedZoom;
    }
    
    void setLightX(float x) {
        std::get<0>(light) = modelCenter.x + x;
    }
    
    void setLightY(float y) {
        std::get<1>(light) = modelCenter.y + y;
    }
    
    void setLightZ(float z) {
        std::get<2>(light) = modelCenter.z + z;
    }
    
    void setCameraX(float x) {
        outsideCameraPos.x = modelCenter.x + x;
    }
    
    void setCameraY(float y) {
        outsideCameraPos.y = modelCenter.y + y;
    }
    
private:
    Uint32 lastUpdateTime = SDL_GetTicks();
    std::unique_ptr<mx::Model> obj;
    gl::ShaderProgram shader;
    std::tuple<float, bool> rot_x = {0.0f, true};
    std::tuple<float, bool> rot_y = {0.0f, true};
    std::tuple<float, bool> rot_z = {0.0f, false};
    std::tuple<float, float, float> light = {0.0f, 50.0f, 50.0f};    
    glm::vec3 lookDirection = glm::vec3(0.0f, 0.0f, -1.0f);
    bool viewRotationActive = false;
    float cameraYaw = 0.0f;
    float cameraPitch = 0.0f;
    const float cameraRotationSpeed = 5.0f;
    glm::vec3 modelMin = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 modelMax = glm::vec3(std::numeric_limits<float>::lowest());
    glm::vec3 modelCenter = glm::vec3(0.0f);
    float modelRadius = 1.0f;
    float baseCameraDistance = 50.0f;
    bool insideCube = false;
    glm::vec3 outsideCameraPos = glm::vec3(0.0f, 0.0f, 50.0f);
    float outsideFOV = 45.0f;
    int texture = 0;
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(bool full, std::string path, int tw, int th) : gl::GLWindow("MX Model Viewer", tw, th) {
        setPath(path);
	if(full) setFullScreen(true);
        setObject(new ModelViewer());
        object->load(this);
    }
    
    ~MainWindow() override {}
    
    virtual void event(SDL_Event &e) override {
        object->event(this, e);
    }
    
    virtual void draw() override {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glViewport(0, 0, w, h);
        object->draw(this);
        swap();
        delay();
    }
    
    void loadNewObject(const std::string &modelPath, const std::string &texturePath, const std::string &textureDir, bool compress = true) {
        ModelViewer *viewer = dynamic_cast<ModelViewer*>(object.get());
        if (viewer) {
            viewer->compressModel = compress;
            viewer->loadObject(this, modelPath, texturePath, textureDir);
        }
    }
    
    void setCompressModel(bool compress) {
        ModelViewer *viewer = dynamic_cast<ModelViewer*>(object.get());
        if (viewer) {
            viewer->compressModel = compress;
        }
    }
    
    void setRotationX(bool enabled) {
        ModelViewer *viewer = dynamic_cast<ModelViewer*>(object.get());
        if (viewer) {
            viewer->setRotX(enabled);
        }
    }
    
    void setRotationY(bool enabled) {
        ModelViewer *viewer = dynamic_cast<ModelViewer*>(object.get());
        if (viewer) {
            viewer->setRotY(enabled);
        }
    }
    
    void setRotationZ(bool enabled) {
        ModelViewer *viewer = dynamic_cast<ModelViewer*>(object.get());
        if (viewer) {
            viewer->setRotZ(enabled);
        }
    }
    
    void setZoom(float zoomFactor) {
        ModelViewer *viewer = dynamic_cast<ModelViewer*>(object.get());
        if (viewer) {
            viewer->setZoom(zoomFactor);
        }
    }
    
    void setLightX(float x) {
        ModelViewer *viewer = dynamic_cast<ModelViewer*>(object.get());
        if (viewer) {
            viewer->setLightX(x);
        }
    }
    
    void setLightY(float y) {
        ModelViewer *viewer = dynamic_cast<ModelViewer*>(object.get());
        if (viewer) {
            viewer->setLightY(y);
        }
    }
    
    void setLightZ(float z) {
        ModelViewer *viewer = dynamic_cast<ModelViewer*>(object.get());
        if (viewer) {
            viewer->setLightZ(z);
        }
    }
    
    void setCameraX(float x) {
        ModelViewer *viewer = dynamic_cast<ModelViewer*>(object.get());
        if (viewer) {
            viewer->setCameraX(x);
        }
    }
    
    void setCameraY(float y) {
        ModelViewer *viewer = dynamic_cast<ModelViewer*>(object.get());
        if (viewer) {
            viewer->setCameraY(y);
        }
    }
    
    virtual void resize(int width, int height) override {
        w = width;
        h = height;
    }
};

MainWindow *main_w = nullptr;

void eventProc() {
    main_w->proc();
}

extern "C" {
    EMSCRIPTEN_KEEPALIVE
    void loadModelFromJS(const char* modelPath, const char* texturePath, const char* textureDir, bool compressModel = true) {
        if (main_w) {
            try {
                jsLogStream << "[INFO] Loading model: " << modelPath << "\n";
                jsLogStream << "[INFO] Texture: " << texturePath << "\n";
                jsLogStream << "[INFO] Compress model: " << (compressModel ? "true" : "false") << "\n";
                main_w->loadNewObject(modelPath, texturePath, textureDir, compressModel);
                jsLogStream << "[SUCCESS] Model loaded successfully!\n";
            } catch (const mx::Exception &e) {
                jsLogStream << "[ERROR] " << e.text() << "\n";
            } catch (const std::exception &e) {
                jsLogStream << "[ERROR] " << e.what() << "\n";
            }
        }
    }
    
    EMSCRIPTEN_KEEPALIVE
    void setCompressModelFromJS(bool compressModel) {
        if (main_w) {
            main_w->setCompressModel(compressModel);
            jsLogStream << "[INFO] Compress model set to: " << (compressModel ? "true" : "false") << "\n";
        }
    }
    
    EMSCRIPTEN_KEEPALIVE
    void setRotationX(bool enabled) {
        if (main_w) {
            main_w->setRotationX(enabled);
            jsLogStream << "[INFO] X-axis rotation: " << (enabled ? "ON" : "OFF") << "\n";
        }
    }
    
    EMSCRIPTEN_KEEPALIVE
    void setRotationY(bool enabled) {
        if (main_w) {
            main_w->setRotationY(enabled);
            jsLogStream << "[INFO] Y-axis rotation: " << (enabled ? "ON" : "OFF") << "\n";
        }
    }
    
    EMSCRIPTEN_KEEPALIVE
    void setRotationZ(bool enabled) {
        if (main_w) {
            main_w->setRotationZ(enabled);
            jsLogStream << "[INFO] Z-axis rotation: " << (enabled ? "ON" : "OFF") << "\n";
        }
    }
    
    EMSCRIPTEN_KEEPALIVE  
    void logMessage(const char* msg) {
        jsLogStream << msg << "\n";
    }
    
    EMSCRIPTEN_KEEPALIVE
    void setZoom(float zoomFactor) {
        if (main_w) {
            main_w->setZoom(zoomFactor);
        }
    }
    
    EMSCRIPTEN_KEEPALIVE
    void setLightX(float x) {
        if (main_w) {
            main_w->setLightX(x);
        }
    }
    
    EMSCRIPTEN_KEEPALIVE
    void setLightY(float y) {
        if (main_w) {
            main_w->setLightY(y);
        }
    }
    
    EMSCRIPTEN_KEEPALIVE
    void setLightZ(float z) {
        if (main_w) {
            main_w->setLightZ(z);
        }
    }
    
    EMSCRIPTEN_KEEPALIVE
    void setCameraX(float x) {
        if (main_w) {
            main_w->setCameraX(x);
        }
    }
    
    EMSCRIPTEN_KEEPALIVE
    void setCameraY(float y) {
        if (main_w) {
            main_w->setCameraY(y);
        }
    }
    
    EMSCRIPTEN_KEEPALIVE
    void resizeCanvas(int width, int height) {
        if (main_w) {
            main_w->resize(width, height);
        }
    }
}

int main(int argc, char **argv) {
    std::cout.rdbuf(&jsLogBuffer);
    try {
        jsLogStream << "[INFO] MX Model Viewer initializing...\n";
        MainWindow main_window(false, "/", 960, 720);
        main_w = &main_window;
        jsLogStream << "[INFO] WebGL context created successfully\n";
        jsLogStream << "[INFO] Default model loaded\n";
        jsLogStream << "[INFO] Use the control panel to adjust view settings\n";
        emscripten_set_main_loop(eventProc, 0, 1);
    } catch (mx::Exception &e) {
        jsLogStream << "[FATAL] " << e.text() << "\n";
        std::cerr << e.text() << "\n";
        return EXIT_FAILURE;
    } catch (std::exception &e) {
        jsLogStream << "[FATAL] " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return 0;
}
