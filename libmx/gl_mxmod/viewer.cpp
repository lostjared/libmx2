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

#define CHECK_GL_ERROR() \
    { GLenum err = glGetError(); \
      if (err != GL_NO_ERROR) \
        printf("OpenGL Error: %d at %s:%d\n", err, __FILE__, __LINE__); }

class ModelViewer : public gl::GLObject {
public:
    gl::ShaderProgram shaderProgram;
    GLuint texture;
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

    ModelViewer(const std::string &filename, const std::string &text, std::string text_path) : rot_x{1.0f, true}, rot_y{1.0f, true}, rot_z{0.0f, false}, light(0.0, 10.0, 5.0) {
        this->filename = filename;
        this->text = text;
        this->texture_path = text_path;    
    }
    virtual ~ModelViewer() override {
        glDeleteTextures(1, &texture);
    }

    float zoom = 45.0f;

    virtual void load(gl::GLWindow *win) override {
        if(texture_path.empty()) {
            texture_path = win->util.getFilePath("data");
            std::cout << "Viewer: Texture default: " << texture_path << "\n";
        }
        mx::system_out << "Viewer: Loading files in path: " << win->util.getFilePath("data") << "\n";
        if(!obj_model.openModel(filename, true)) { 
            throw mx::Exception("Error loading model...");
        }
        mx::system_out << "mx: Loaded Meshes:  " << obj_model.meshes.size() << "\n";


        if (!shaderProgram.loadProgram(win->util.getFilePath("data/tri.vert"), win->util.getFilePath("data/tri.frag"))) {
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

        glm::mat4 projection = glm::perspective(glm::radians(zoom), (float)win->w / win->h, 0.1f, 100.0f);
        shaderProgram.setUniform("model", model);
        shaderProgram.setUniform("view", view);
        shaderProgram.setUniform("projection", projection);

        if(text.find(".png") != std::string::npos) {
            texture = gl::loadTexture(text);
            std::vector<GLuint> textures {texture};
            obj_model.setTextures(textures);
        } else {
            obj_model.setTextures(win, text, texture_path);
        }
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
            fovDegrees = 120.0f;  
        } else {
            lookDirection = glm::vec3(0.0f, 0.0f, -1.0f);  
            cameraPos = outsideCameraPos;
            fovDegrees = outsideFOV;
        }
        glm::vec3 cameraTarget = cameraPos + lookDirection;
        glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

        glm::mat4 viewMatrix = glm::lookAt(cameraPos, cameraTarget, cameraUp);
        glm::mat4 projectionMatrix = glm::perspective(
            glm::radians(fovDegrees),
            static_cast<float>(win->w) / static_cast<float>(win->h),
            insideCube ? 0.01f : 0.1f,  
            10.0f
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
        modelMatrix = glm::rotate(modelMatrix, glm::radians(std::get<0>(rot_x)), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(std::get<0>(rot_y)), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(std::get<0>(rot_z)), glm::vec3(0.0f, 0.0f, 1.0f));
        glm::vec3 lightPos(std::get<0>(light), std::get<1>(light), std::get<2>(light));
        glm::vec4 lightPosView = viewMatrix * glm::vec4(lightPos, 1.0f);
        shaderProgram.setUniform("lightPos", glm::vec3(lightPosView));
        glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
        shaderProgram.setUniform("model", modelMatrix);
        shaderProgram.setUniform("view", viewMatrix);
        shaderProgram.setUniform("projection", projectionMatrix);
        shaderProgram.setUniform("lightColor", lightColor);
        obj_model.drawArrays();
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
            case SDL_KEYUP:
                switch(e.key.keysym.sym) {
                    case SDLK_RETURN:
                        insideCube = !insideCube;
                        viewRotationActive = insideCube ? false : true;
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
                       zoom -= 1.0f;
                       if (zoom < 10.0f) zoom = 10.0f; 
                       outsideFOV = zoom;
                    break;
                    case SDLK_s:
                       zoom += 1.0f;
                        if (zoom > 90.0f) zoom = 90.0f; 
                        outsideFOV = zoom;
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
                    case SDLK_p: {
#ifndef __EMSCRIPTEN__
                        static bool poly = false;
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
                        mx::system_out << "X: " << std::get<0>(rot_x) << " Y: " << std::get<0>(rot_y) << "\n";
                    break;
                }
            }
            break;
        }
    }
private:
    bool insideCube = false;
    glm::vec3 outsideCameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
    float outsideFOV = zoom;
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(const std::string &path, const std::string &filename, const std::string &text, const std::string &tpath, int tw, int th) : gl::GLWindow("Model Viewer", tw, th) {
        setPath(path);
        setObject(new ModelViewer(filename, text, tpath));
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
          .addOptionSingle('a', "Aboolute Path");
    Argument<std::string> arg;
    std::string path;
    int value = 0;
    int tw = 1440, th = 1080;
    std::string model_file;
    std::string text_file, texture_path;
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

            }
        }
    } catch (const ArgException<std::string>& e) {
        mx::system_err << e.text() << "\n";
    }
    if(path.empty()) {
        mx::system_out << "mx: No path provided trying default current directory.\n";
        path = ".";
    }
    try {
        MainWindow main_window(path, model_file, text_file,texture_path,  tw, th);
        main_window.loop();
    } catch(const mx::Exception &e) {
        mx::system_err << "mx: Exception: " << e.text() << "\n";
        mx::system_err.flush();
        exit(EXIT_FAILURE);
    } 
#endif
    return 0;
}
