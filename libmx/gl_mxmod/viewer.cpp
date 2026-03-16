#include"mx.hpp"
#include"argz.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <GLES3/gl3.h>
#endif

#include"gl.hpp"
#include"loadpng.hpp"
#include"model.hpp"
#include<tuple>
#include<limits>
#include<cmath>
#include<filesystem>

#define CHECK_GL_ERROR() \
    { GLenum err = glGetError(); \
      if (err != GL_NO_ERROR) \
        printf("OpenGL Error: %d at %s:%d\n", err, __FILE__, __LINE__); }


static std::string findShaderFile(const std::string &filename) {
    namespace fs = std::filesystem;
    
    std::vector<std::string> searchPaths = {
        filename,  
        "data/" + filename,
        "./data/" + filename,
        "../data/" + filename,
        "../../libmx/gl_mxmod/data/" + filename  
    };
    
    for (const auto &path : searchPaths) {
        if (fs::exists(path)) {
            mx::system_out << "Found shader: " << path << "\n";
            return path;
        }
    }
    
    throw mx::Exception("Could not find shader file: " + filename);
}

class ModelViewer : public gl::GLObject {
public:
    gl::ShaderProgram shaderProgram;
    GLuint texture;
    mx::Font font;
    mx::Model obj_model;
    std::string filename;
    std::string text, texture_path;
    std::tuple<float, bool> rot_x;
    std::tuple<float, bool> rot_y;
    std::tuple<float, bool> rot_z;
    std::tuple<float, float, float> light;    

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
    float modelRenderScale = 1.0f;
    glm::vec3 modelCenterOffset = glm::vec3(0.0f);
    float cameraDistance = 5.0f;
    bool compress = true;
    float twistAngle = 1.0f;
    float twistSpeed = 6.0f;
    float twistDirection = 1.0f;
    const float minTwistAngle = 0.0f;
    const float maxTwistAngle = 360.0f;
    const float minTwistSpeed = 0.1f;
    const float maxTwistSpeed = 60.0f;
    bool twistX = false, twistY = false, twistZ = false;
    bool morphT = false;
    bool showControls = true;
    bool mouseDown = false;
    int lastMouseX = 0, lastMouseY = 0;
    float mouseRotX = 0.0f, mouseRotY = 0.0f;
#ifndef __EMSCRIPTEN__
    bool poly = false;
#endif

    ModelViewer(const std::string &filename, const std::string &text, std::string text_path, bool compress_) : rot_x{1.0f, true}, rot_y{1.0f, true}, rot_z{0.0f, false}, light(0.0, 10.0, 5.0) {
        this->filename = filename;
        this->text = text;
        this->texture_path = text_path;    
        this->compress = compress_;
    }
    virtual ~ModelViewer() override {
        glDeleteTextures(1, &texture);
    }

    float zoom = 75.0f;
    virtual void load(gl::GLWindow *win) override {
        font.loadFont(win->util.getFilePath("data/font.ttf"), 18);
        if(texture_path.empty()) {
            texture_path = win->util.getFilePath("data");
            std::cout << "Viewer: Texture default: " << texture_path << "\n";
        }
        mx::system_out << "Viewer: Loading files in path: " << win->util.getFilePath("data") << "\n";
        if(!obj_model.openModel(filename, compress)) { 
            throw mx::Exception("Error loading model...");
        }
        mx::system_out << "mx: Loaded Meshes:  " << obj_model.meshes.size() << "\n";
        
        for (const auto &mesh : obj_model.meshes) {
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
        
        modelCenterOffset = -modelCenter;
        const float maxExtent = std::max(extent.x, std::max(extent.y, extent.z));
        const float targetSize = 2.5f;
        if (maxExtent > 1e-6f)
            modelRenderScale = targetSize / maxExtent;
        
        baseCameraDistance = modelRadius * 2.5f;
        cameraDistance = 5.0f;
        outsideCameraPos = glm::vec3(0.0f, 0.0f, cameraDistance);
        
        zoom = 45.0f;
        outsideFOV = zoom;
        
        mx::system_out << "mx: Model bounds: (" << modelMin.x << ", " << modelMin.y << ", " << modelMin.z << ") to ("
                       << modelMax.x << ", " << modelMax.y << ", " << modelMax.z << ")\n";
        mx::system_out << "mx: Model center: (" << modelCenter.x << ", " << modelCenter.y << ", " << modelCenter.z << ")\n";
        mx::system_out << "mx: Model radius: " << modelRadius << ", Camera distance: " << baseCameraDistance << "\n";

        if (!shaderProgram.loadProgram(findShaderFile("tri.vert"), findShaderFile("tri.frag"))) {
            throw mx::Exception("Failed to load shader program");
        }
        shaderProgram.useProgram();
        CHECK_GL_ERROR();

        obj_model.setShaderProgram(&shaderProgram, "texture1");

        glm::mat4 model = glm::mat4(1.0f); 
        glm::mat4 view = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 5.0f),  
            glm::vec3(0.0f, 0.0f, 0.0f),  
            glm::vec3(0.0f, 1.0f, 0.0f)   
        );

        glm::mat4 projection = glm::perspective(glm::radians(zoom), (float)win->w / win->h, 0.1f, 10000.0f);
        shaderProgram.setUniform("model", model);
        shaderProgram.setUniform("view", view);
        shaderProgram.setUniform("projection", projection);

        if(text.find(".png") != std::string::npos) {
            try {
                texture = gl::loadTexture(text);
                std::vector<GLuint> textures {texture};
                obj_model.setTextures(textures);
            } catch (const mx::Exception &e) {
                mx::system_err << "Warning: Failed to load PNG from '" << text << "': " << e.text() << "\n";
                mx::system_err << "Attempting to load as color name or default texture...\n";
                obj_model.setTextures(win, text, texture_path);
            }
        } else {
            obj_model.setTextures(win, text, texture_path);
        }
        obj_model.saveOriginal();
    }

    virtual void draw(gl::GLWindow *win) override {
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
            cameraPos = glm::vec3(0.0f, 0.0f, cameraDistance);
            fovDegrees = outsideFOV;
        }
        glm::vec3 cameraTarget = insideCube ? cameraPos + lookDirection : glm::vec3(0.0f);
        glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

        glm::mat4 viewMatrix = glm::lookAt(cameraPos, cameraTarget, cameraUp);
        glm::mat4 projectionMatrix = glm::perspective(
            glm::radians(fovDegrees),
            static_cast<float>(win->w) / static_cast<float>(win->h),
            insideCube ? 0.01f : 0.01f,  
            10000.0f
        );
        if (insideCube) {
            glFrontFace(GL_CW);
        } else {
            glFrontFace(GL_CCW);
        }

    #ifdef __EMSCRIPTEN__
        static Uint32 lastTime = emscripten_get_now();
        float currentTime = emscripten_get_now();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;
    #else
        static Uint32 lastTime = SDL_GetTicks();
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;
    #endif
        shaderProgram.useProgram();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        shaderProgram.setUniform("texture1", 0);
        twistAngle += twistDirection * twistSpeed * deltaTime;
        if (twistAngle > maxTwistAngle) {
            twistAngle = maxTwistAngle - (twistAngle - maxTwistAngle);
            twistDirection = -1.0f;
        } else if (twistAngle < minTwistAngle) {
            twistAngle = minTwistAngle + (minTwistAngle - twistAngle);
            twistDirection = 1.0f;
        }
        obj_model.resetToOriginal();

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
        modelMatrix = glm::scale(modelMatrix, glm::vec3(modelRenderScale));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(mouseRotX), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(mouseRotY), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(std::get<0>(rot_x)), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(std::get<0>(rot_y)), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(std::get<0>(rot_z)), glm::vec3(0.0f, 0.0f, 1.0f));
        modelMatrix = glm::translate(modelMatrix, modelCenterOffset);
        glm::vec3 lightPos(std::get<0>(light), std::get<1>(light), std::get<2>(light));
        glm::vec4 lightPosView = viewMatrix * glm::vec4(lightPos, 1.0f);
        shaderProgram.setUniform("lightPos", glm::vec3(lightPosView));
        glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
        shaderProgram.setUniform("model", modelMatrix);
        shaderProgram.setUniform("view", viewMatrix);
        shaderProgram.setUniform("projection", projectionMatrix);
        shaderProgram.setUniform("lightColor", lightColor);

               
        const float safeRadius = (modelRadius > 0.001f) ? modelRadius : 0.001f;
        const float deformFactor = glm::radians(twistAngle) / safeRadius;

        if(twistY) obj_model.twist(mx::DeformAxis::Y, deformFactor, modelCenter.y);
        if(twistZ) obj_model.twist(mx::DeformAxis::Z, deformFactor, modelCenter.z);
        if(twistX) obj_model.twist(mx::DeformAxis::X, deformFactor, modelCenter.x);
        if(morphT) obj_model.bend(mx::DeformAxis::Y, deformFactor, modelCenter.y, 0.0f);

        obj_model.updateBuffers();
        obj_model.recalculateNormals();
        obj_model.drawArrays();

        if (showControls) {
            win->text.setColor({255, 255, 0, 255});
            win->text.printText_Solid(font, 20.0f, 20.0f, "Controls (H: show/hide)");
            win->text.setColor({255, 255, 255, 255});
            win->text.printText_Solid(font, 20.0f, 48.0f, "Mouse Drag   - Rotate model      Scroll Wheel - Zoom");
            win->text.printText_Solid(font, 20.0f, 76.0f, "Arrow Keys   - Toggle auto-rotation axes");
            win->text.printText_Solid(font, 20.0f, 104.0f, "A/S or +/-   - Zoom in/out        Home - Reset view");
            win->text.printText_Solid(font, 20.0f, 132.0f, "W            - Toggle wireframe    R - Reverse twist");
            win->text.printText_Solid(font, 20.0f, 160.0f, "X/Y/Z - Toggle twist axis   T - Toggle bend");
            win->text.printText_Solid(font, 20.0f, 188.0f, "K/L/I/O - Move light   Enter - Inside/Outside");
            win->text.printText_Solid(font, 20.0f, 216.0f, "Space - Toggle rotation lock   Escape - Quit");
            win->text.printText_Solid(font, 20.0f, 244.0f,
                "twistSpeed=" + std::to_string(twistSpeed) + " deg/sec  direction=" +
                std::string(twistDirection > 0.0f ? "forward" : "reverse"));
        }

        glBindVertexArray(0);
    }

    void toggle(std::tuple<float, bool> &rot) {
        if(std::get<1>(rot) == true) {
            std::get<1>(rot) = false;
            std::get<0>(rot) = 0.0f;
        } else {
            std::get<1>(rot) = true;
            std::get<0>(rot) = 1.0f;
        }
    }

    virtual void event(gl::GLWindow *win, SDL_Event &e) override {
        switch(e.type) {
            case SDL_MOUSEBUTTONDOWN:
                if (e.button.button == SDL_BUTTON_LEFT) {
                    mouseDown = true;
                    lastMouseX = e.button.x;
                    lastMouseY = e.button.y;
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if (e.button.button == SDL_BUTTON_LEFT) {
                    mouseDown = false;
                }
                break;
            case SDL_MOUSEMOTION:
                if (mouseDown) {
                    int dx = e.motion.x - lastMouseX;
                    int dy = e.motion.y - lastMouseY;
                    mouseRotY += dx * 0.5f;
                    mouseRotX += dy * 0.5f;
                    lastMouseX = e.motion.x;
                    lastMouseY = e.motion.y;
                }
                break;
            case SDL_MOUSEWHEEL:
                cameraDistance -= e.wheel.y * 0.5f;
                if (cameraDistance < 0.1f) cameraDistance = 0.1f;
                if (cameraDistance > 100.0f) cameraDistance = 100.0f;
                break;
            case SDL_KEYUP:
                switch(e.key.keysym.sym) {
                    case SDLK_RETURN:
                        insideCube = !insideCube;
                        viewRotationActive = insideCube ? false : true;
                        break;
                case SDLK_x:
                    twistX = !twistX;
                    break;
                case SDLK_y:
                    twistY = !twistY;
                    break;
                case SDLK_z:
                    twistZ = !twistZ;
                    break;
                case SDLK_t:
                    morphT = !morphT;
                    break;
                }
                break;
        }
        float stepSize = 1.0;
           switch(e.type) {
            case SDL_KEYDOWN: {
                switch(e.key.keysym.sym) {
                    case SDLK_LEFT:
                        toggle(rot_x);
                    break;
                    case SDLK_RIGHT:
                        toggle(rot_x);
                    break;
                    case SDLK_DOWN:
                        toggle(rot_z);
                    break;
                    case SDLK_UP:
                        toggle(rot_y);
                    break;
                    case SDLK_a: 
                       cameraDistance -= 0.5f;
                       if (cameraDistance < 0.1f) cameraDistance = 0.1f;
                    break;
                    case SDLK_s: 
                       cameraDistance += 0.5f;
                       if (cameraDistance > 100.0f) cameraDistance = 100.0f;
                    break;
                    case SDLK_k:
                        std::get<0>(light) += stepSize; 
                        break;
                    case SDLK_l:
                        std::get<0>(light) -= stepSize;
                        break;
                    case SDLK_i:
                        std::get<1>(light) += stepSize;
                        break;
                    case SDLK_o:
                        std::get<1>(light) -= stepSize; 
                        break;
                    case SDLK_p:
                    case SDLK_w: {
#ifndef __EMSCRIPTEN__
                        if(poly == false) {
                            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                            poly = true;
                        } else {
                            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                            poly = false;                            
                        }
#endif
                    }
                    break;
                    case SDLK_SPACE:
                    case SDLK_h:
#ifndef __EMSCRIPTEN__
                        if(poly == false) {
#endif
                            showControls = !showControls;
                            mx::system_out << "mx: controls overlay " << (showControls ? "shown" : "hidden") << "\n";
#ifndef __EMSCRIPTEN__
                        }
#endif
                    break;
                    case SDLK_HOME:
                        cameraDistance = 5.0f;
                        mouseRotX = 0.0f;
                        mouseRotY = 0.0f;
                        rot_x = {1.0f, true};
                        rot_y = {1.0f, true};
                        rot_z = {0.0f, false};
                    break;
                    case SDLK_ESCAPE:
                        win->quit();
                    break;
                    case SDLK_MINUS:
                    case SDLK_KP_MINUS:
                        twistSpeed -= 1.0f;
                        if (twistSpeed < minTwistSpeed) twistSpeed = minTwistSpeed;
                        mx::system_out << "mx: twistSpeed = " << twistSpeed << " deg/sec\n";
                    break;
                    case SDLK_EQUALS:
                    case SDLK_KP_PLUS:
                        twistSpeed += 1.0f;
                        if (twistSpeed > maxTwistSpeed) twistSpeed = maxTwistSpeed;
                        mx::system_out << "mx: twistSpeed = " << twistSpeed << " deg/sec\n";
                    break;
                    case SDLK_0:
                    case SDLK_KP_0:
                        twistSpeed = 6.0f;
                        mx::system_out << "mx: twistSpeed reset = " << twistSpeed << " deg/sec\n";
                    break;
                    case SDLK_r:
                        twistDirection *= -1.0f;
                        mx::system_out << "mx: twistDirection = " << (twistDirection > 0.0f ? "forward" : "reverse") << "\n";
                    break;
                }
            }
            break;
        }
    }
private:
    bool insideCube = false;
    glm::vec3 outsideCameraPos = glm::vec3(0.0f, 0.0f, 5.0f);
    float outsideFOV = 45.0f;
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(const std::string &path, const std::string &filename, const std::string &text, const std::string &tpath, int tw, int th, bool compress) : gl::GLWindow("Model Viewer - [ OpenGL ]", tw, th) {
        setPath(path);
        setObject(new ModelViewer(filename, text, tpath, compress));
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
    }
private:
};

MainWindow *main_w = nullptr;

void eventProc() {
    main_w->proc();
}

int main(int argc, char **argv) {
    fflush(stdout);
    fflush(stdin);
#ifdef __EMSCRIPTEN__
    MainWindow main_window("/", "globe.mxmod", "metal.tex", 1440, 1080);
    main_w =&main_window;
    emscripten_set_main_loop(eventProc, 0, 1);
#else
    Argz<std::string> parser(argc, argv);    
    parser.addOptionSingle('h', "Display help message")
          .addOptionSingleValue('p', "assets path")
          .addOptionDoubleValue('P', "path", "assets path")
          .addOptionSingleValue('r',"Resolution WidthxHeight")
          .addOptionDoubleValue('R',"resolution", "Resolution WidthxHeight")
          .addOptionSingleValue('m',"Model file")
          .addOptionDoubleValue('M', "model", "model file to load")
          .addOptionSingleValue('t', "texture file")
          .addOptionDoubleValue(123, "texture", "texture file")
          .addOptionSingleValue('T', "texture path")
          .addOptionSingle('a', "Aboolute Path")
          .addOptionDoubleValue(256, "compress", "compress true, compress false");

    Argument<std::string> arg;
    std::string path;
    int value = 0;
    int tw = 1440, th = 1080;
    std::string model_file;
    std::string text_file, texture_path;
    bool compress = true;
    try {
        while((value = parser.proc(arg)) != -1) {
            switch(value) {
                case 'h':
                case 'v':
                    parser.help(std::cout);
                    exit(EXIT_SUCCESS);
                    break;
                case 'p':
                case 'P':
                    path = arg.arg_value;
                break;
                case 'r':
                case 'R': {
                    auto pos = arg.arg_value.find("x");
                    if(pos == std::string::npos)  {
                        mx::system_err << "Error invalid resolution use WidthxHeight\n";
                        mx::system_err.flush();
                        exit(EXIT_FAILURE);
                    }
                    std::string left, right;
                    left = arg.arg_value.substr(0, pos);
                    right = arg.arg_value.substr(pos+1);
                    tw = atoi(left.c_str());
                    th = atoi(right.c_str());
                }
                break;
                case 'm':
                case 'M':
                    model_file = arg.arg_value;
                break;
                case 123:
                case 't':
                    text_file = arg.arg_value;
                break;
                case 'T':
                    texture_path = arg.arg_value;
                    break;
                case 256:
                    if(arg.arg_value == "true")
                        compress = true;
                    else
                        compress = false;
                    break;

            }
        }
    } catch (const ArgException<std::string>& e) {
        mx::system_err << e.text() << "\n";
    }

    if(parser.count() == 0) {
        parser.help(mx::system_out);
        exit(EXIT_SUCCESS);
    }

    if(path.empty()) {
        mx::system_out << "mx: No path provided trying default current directory.\n";
        path = ".";
    }
    try {
        MainWindow main_window(path, model_file, text_file,texture_path,  tw, th, compress);
        main_window.loop();
    } catch(const mx::Exception &e) {
        mx::system_err << "mx: Exception: " << e.text() << "\n";
        mx::system_err.flush();
        exit(EXIT_FAILURE);
    } 
#endif
    return 0;
}
