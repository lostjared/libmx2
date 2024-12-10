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
    GLuint positionVBO, normalVBO, texCoordVBO;
    GLuint VAO;
    mx::Model model;
    std::string filename;
    std::string text;
    std::tuple<float, bool> rot_x;
    std::tuple<float, bool> rot_y;
    std::tuple<float, bool> rot_z;
    

    ModelViewer(const std::string &filename, const std::string &text) : rot_x{1.0f, true}, rot_y{1.0f, true}, rot_z{0.0f, false} {
        this->filename = filename;
        this->text = text;
    
    }
    virtual ~ModelViewer() override {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &positionVBO);
        glDeleteBuffers(1, &normalVBO);
        glDeleteBuffers(1, &texCoordVBO);
        glDeleteTextures(1, &texture);
    }

    float zoom = 45.0f;

    virtual void load(gl::GLWindow *win) override {
        if(!model.openModel(filename)) {
            mx::system_err << "Error loading model..\n";
            mx::system_err.flush();
            exit(EXIT_FAILURE);
        }
        model.generateBuffers(VAO, positionVBO, normalVBO, texCoordVBO);
        if (!shaderProgram.loadProgram(win->util.getFilePath("data/tri.vert"), win->util.getFilePath("data/tri.frag"))) {
            throw mx::Exception("Failed to load shader program");
        }
        shaderProgram.useProgram();
        CHECK_GL_ERROR();

        glm::mat4 model = glm::mat4(1.0f); // Identity matrix
        glm::mat4 view = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 5.0f),  // Camera position
            glm::vec3(0.0f, 0.0f, 0.0f),  // Look at origin
            glm::vec3(0.0f, 1.0f, 0.0f)   // Up vector
        );

        glm::mat4 projection = glm::perspective(glm::radians(zoom), (float)win->w / win->h, 0.1f, 100.0f);
        shaderProgram.setUniform("model", model);
        shaderProgram.setUniform("view", view);
        shaderProgram.setUniform("projection", projection);
        texture = gl::loadTexture(text);
    }

    virtual void draw(gl::GLWindow *win) override {
        glm::vec3 cameraPos(0.0f, 0.0f, 5.0f);
        glm::vec3 cameraTarget(0.0f, 0.0f, 0.0f);
        glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);
        float aspectRatio = (float)win->w / (float)win->h;
        static float rotationAngle = 0.0f; // Rotation angle in degrees
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
        rotationAngle += deltaTime * 50.0f; 
        shaderProgram.useProgram();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        shaderProgram.setUniform("texture1", 0);
        glBindVertexArray(VAO);
        glm::mat4 modelMatrix = glm::mat4(1.0f);

        if(std::get<0>(rot_x) == 0.0f && std::get<0>(rot_y) == 0.0f && std::get<0>(rot_z) == 0.0f) {
            std::get<0>(rot_x) = 1.0f;
            std::get<1>(rot_x) = true;
        }

        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationAngle), glm::vec3(std::get<0>(rot_x), std::get<0>(rot_y), std::get<0>(rot_z))); // Rotate around X-axis
        glm::mat4 viewMatrix = glm::lookAt(cameraPos, cameraTarget, cameraUp);
        glm::mat4 projectionMatrix = glm::perspective(glm::radians(zoom), aspectRatio, 0.1f, 100.0f);
        glm::vec3 lightPos(2.0f, 4.0f, 1.0f);
        glm::vec3 viewPos = cameraPos;
        glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
        shaderProgram.setUniform("model", modelMatrix);
        shaderProgram.setUniform("view", viewMatrix);
        shaderProgram.setUniform("projection", projectionMatrix);
        shaderProgram.setUniform("lightPos", lightPos);
        shaderProgram.setUniform("viewPos", viewPos);
        shaderProgram.setUniform("lightColor", lightColor);
        model.drawArrays();
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

    virtual void event(gl::GLWindow *window, SDL_Event &e) override {
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
                    break;
                    case SDLK_s:
                       zoom += 1.0f;
                        if (zoom > 90.0f) zoom = 90.0f; 
            
                    break;
                }
            }
            break;
        }
    }
private:
    
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(const std::string &path, const std::string &filename, const std::string &text, int tw, int th) : gl::GLWindow("OpenGL Example", tw, th) {
        setPath(path);
        setObject(new ModelViewer(filename, text));
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
#ifdef __EMSCRIPTEN__
    MainWindow main_window("/", 960, 720);
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
          .addOptionDoubleValue('T', "texture", "texture file");
    Argument<std::string> arg;
    std::string path;
    int value = 0;
    int tw = 960, th = 720;
    std::string model_file;
    std::string text_file;
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
                case 't':
                case 'T':
                    text_file = arg.arg_value;
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
        MainWindow main_window(path, model_file, text_file, tw, th);
        main_window.loop();
    } catch(const mx::Exception &e) {
        mx::system_err << "mx: Exception: " << e.text() << "\n";
        mx::system_err.flush();
        exit(EXIT_FAILURE);
    } 
#endif
    return 0;
}