#include"mx.hpp"
#include"argz.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <GLES3/gl3.h>
#endif

#include"gl.hpp"
#include"loadpng.hpp"

#define CHECK_GL_ERROR() \
    { GLenum err = glGetError(); \
      if (err != GL_NO_ERROR) \
        printf("OpenGL Error: %d at %s:%d\n", err, __FILE__, __LINE__); }


GLfloat vertices[] = { -1.0f, -1.0f, 1.0f, // front face
    1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    
    -1.0f, -1.0f, 1.0f,
    1.0f, -1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    
    -1.0f, -1.0f, -1.0f, // left side
    -1.0f, -1.0f, 1.0f,
    -1.0f, 1.0f, -1.0f,
    
    -1.0f, 1.0f, -1.0f,
    -1.0f, -1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    
    -1.0f, 1.0f, -1.0f, // top
    -1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, -1.0f,
    -1.0f, 1.0f, -1.0f,
    
    -1.0f, -1.0f, -1.0f, // bottom
    -1.0f, -1.0f, 1.0f,
    1.0f, -1.0f, 1.0f,
    
    1.0f, -1.0f, 1.0f,
    1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
    
    1.0f, -1.0f, -1.0f, // right
    1.0f, -1.0f, 1.0f,
    1.0f, 1.0f, -1.0f,
    
    1.0f, 1.0f, -1.0f,
    1.0f, -1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    
    -1.0f, -1.0f, -1.0f, // back face
    1.0f, 1.0f, -1.0f,
    -1.0f, 1.0f, -1.0f,
    
    -1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,
    1.0f, 1.0f, -1.0f,
    
};

GLfloat texCoords[] = {
    0, 1, // Front face
    1, 0,
    0, 0,
    
    0, 1,
    1, 1,
    1, 0,
    
    0, 1, // Left face
    1, 1,
    0, 0,
    
    0, 0,
    1, 1,
    1, 0,
    
    0, 1, // Top face
    0, 0,
    1, 0,
    
    1, 0,
    1, 1,
    0, 1,
    
    0, 1, // Bottom face
    0, 0,
    1, 0,
    
    1, 0,
    1, 1,
    0, 1,
    
    0, 1, // Right face
    1, 1,
    0, 0,
    
    0, 0,
    1, 1,
    1, 0,
    
    0, 1, // Back face
    1, 0,
    0, 0,
    
    0, 1,
    1, 1,
    1, 0
};


class Cube : public gl::GLObject {
public:
    gl::ShaderProgram shaderProgram;
    GLuint VAO, VBO;
    GLuint texture;
        
    Cube() = default;
    virtual ~Cube() override {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteTextures(1, &texture);
    }

    std::vector<GLfloat> cubeData;

    virtual void load(gl::GLWindow *win) override {     
        size_t vertexCount = sizeof(vertices) / (3 * sizeof(GLfloat)); 
        size_t texCoordCount = sizeof(texCoords) / (2 * sizeof(GLfloat)); 
   
        if (vertexCount != texCoordCount) {
            throw std::runtime_error("Vertices and texture coordinates size mismatch");
        }
     
        cubeData.resize(vertexCount * 5);   

        for (size_t i = 0; i < vertexCount; ++i) {
            cubeData[i * 5] = vertices[i * 3];         // Position X
            cubeData[i * 5 + 1] = vertices[i * 3 + 1]; // Position Y
            cubeData[i * 5 + 2] = vertices[i * 3 + 2]; // Position Z
            cubeData[i * 5 + 3] = texCoords[i * 2];    // Texture U
            cubeData[i * 5 + 4] = texCoords[i * 2 + 1]; // Texture V
        }

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, cubeData.size() * sizeof(GLfloat), cubeData.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);

        if (!shaderProgram.loadProgram(win->util.getFilePath("data/tri.vert"), win->util.getFilePath("data/tri.frag"))) {
            throw mx::Exception("Failed to load shader program");
        }
        
        

        

        shaderProgram.useProgram();

        glm::mat4 model = glm::mat4(1.0f); // Identity matrix
       // glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -5.0f)); // Camera backward
        glm::mat4 view = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 5.0f),  // Camera position
            glm::vec3(0.0f, 0.0f, 0.0f),  // Look at origin
            glm::vec3(0.0f, 1.0f, 0.0f)   // Up vector
        );
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)win->w / win->h, 0.1f, 100.0f);

        
    
        shaderProgram.setUniform("model", model);
        shaderProgram.setUniform("view", view);
        shaderProgram.setUniform("projection", projection);
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        SDL_Surface *surface = png::LoadPNG(win->util.getFilePath("data/bg.png").c_str());
        if (!surface) {
            mx::system_err << "Error: loading PNG.\n";
            exit(EXIT_FAILURE);
        }

        GLenum format = (surface->format->BytesPerPixel == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, surface->w, surface->h, 0, format, GL_UNSIGNED_BYTE, surface->pixels);
        SDL_FreeSurface(surface);
    }

    float alpha = 1.0;
    int dir = 1;


    virtual void draw(gl::GLWindow *win) override {
        shaderProgram.useProgram();
        CHECK_GL_ERROR();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindVertexArray(VAO);
        CHECK_GL_ERROR();
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
        static float rotationAngle = 0.0f;
        rotationAngle += deltaTime * 50.0f; 
        glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        shaderProgram.setUniform("model", model);
        if(dir == 1) {
            alpha += 0.1f;
            if(alpha > 10.0f)
                dir = 0;
        } else {
            alpha -= 0.1f;
            if(alpha <= 1.0)
                dir = 1;
        }
        shaderProgram.setUniform("alpha", alpha);
        CHECK_GL_ERROR();
        size_t vertexCount = cubeData.size() / 5; 
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        CHECK_GL_ERROR();
        glBindVertexArray(0);
        CHECK_GL_ERROR();
    }
  

    virtual void event(gl::GLWindow *window, SDL_Event &e) override {
    }
private:
    
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("OpenGL Example", tw, th) {
        setPath(path);
        setObject(new Cube());
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
          .addOptionDoubleValue('R',"resolution", "Resolution WidthxHeight");
    Argument<std::string> arg;
    std::string path;
    int value = 0;
    int tw = 1920, th = 1080;
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
        MainWindow main_window(path, tw, th);
        main_window.loop();
    } catch(const mx::Exception &e) {
        mx::system_err << "mx: Exception: " << e.text() << "\n";
        mx::system_err.flush();
        exit(EXIT_FAILURE);
    } 
#endif
    return 0;
}