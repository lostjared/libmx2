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


GLfloat positions[] = {
    // Base (square)
    -1.0f, 0.0f,  1.0f,
     1.0f, 0.0f,  1.0f,
     1.0f, 0.0f, -1.0f,

    -1.0f, 0.0f,  1.0f,
     1.0f, 0.0f, -1.0f,
    -1.0f, 0.0f, -1.0f,

    // Front face
    -1.0f, 0.0f,  1.0f,
     1.0f, 0.0f,  1.0f,
     0.0f, 1.0f,  0.0f,

    // Right face
     1.0f, 0.0f,  1.0f,
     1.0f, 0.0f, -1.0f,
     0.0f, 1.0f,  0.0f,

    // Back face
     1.0f, 0.0f, -1.0f,
    -1.0f, 0.0f, -1.0f,
     0.0f, 1.0f,  0.0f,

    // Left face
    -1.0f, 0.0f, -1.0f,
    -1.0f, 0.0f,  1.0f,
     0.0f, 1.0f,  0.0f,
};

GLfloat texCoords[] = {
    // Base (square)
    0.0f, 0.0f,
    1.0f, 0.0f,
    1.0f, 1.0f,

    0.0f, 0.0f,
    1.0f, 1.0f,
    0.0f, 1.0f,

    // Front face
    0.0f, 0.0f,
    1.0f, 0.0f,
    0.5f, 1.0f,

    // Right face
    0.0f, 0.0f,
    1.0f, 0.0f,
    0.5f, 1.0f,

    // Back face
    0.0f, 0.0f,
    1.0f, 0.0f,
    0.5f, 1.0f,

    // Left face
    0.0f, 0.0f,
    1.0f, 0.0f,
    0.5f, 1.0f,
};

GLfloat normals[] = {
    // Base (square)
    0.0f, -1.0f,  0.0f,
    0.0f, -1.0f,  0.0f,
    0.0f, -1.0f,  0.0f,

    0.0f, -1.0f,  0.0f,
    0.0f, -1.0f,  0.0f,
    0.0f, -1.0f,  0.0f,

    // Front face
    0.0f,  0.707f,  0.707f,
    0.0f,  0.707f,  0.707f,
    0.0f,  0.707f,  0.707f,

    // Right face
    0.707f,  0.707f,  0.0f,
    0.707f,  0.707f,  0.0f,
    0.707f,  0.707f,  0.0f,

    // Back face
    0.0f,  0.707f, -0.707f,
    0.0f,  0.707f, -0.707f,
    0.0f,  0.707f, -0.707f,

    // Left face
    -0.707f,  0.707f,  0.0f,
    -0.707f,  0.707f,  0.0f,
    -0.707f,  0.707f,  0.0f,
};

class Pyramid : public gl::GLObject {
public:
    gl::ShaderProgram shaderProgram;
    GLuint texture;
    GLuint positionVBO, normalVBO, texCoordVBO;
    GLuint VAO;

    Pyramid() = default;
    virtual ~Pyramid() override {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &positionVBO);
        glDeleteBuffers(1, &normalVBO);
        glDeleteBuffers(1, &texCoordVBO);
        glDeleteTextures(1, &texture);
    }

    std::vector<GLfloat> cubeData;

    virtual void load(gl::GLWindow *win) override {
        
        glGenVertexArrays(1, &VAO);
        CHECK_GL_ERROR();
        glBindVertexArray(VAO);
        CHECK_GL_ERROR();

        glGenBuffers(1, &positionVBO);
        CHECK_GL_ERROR();
        glBindBuffer(GL_ARRAY_BUFFER, positionVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);
        CHECK_GL_ERROR();
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0); // Location = 0
        glEnableVertexAttribArray(0);
        CHECK_GL_ERROR();

        glGenBuffers(1, &normalVBO);
        CHECK_GL_ERROR();
        glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(normals), normals, GL_STATIC_DRAW);
        CHECK_GL_ERROR();
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0); // Location = 1
        glEnableVertexAttribArray(1);
        CHECK_GL_ERROR();

        glGenBuffers(1, &texCoordVBO);
        CHECK_GL_ERROR();
        glBindBuffer(GL_ARRAY_BUFFER, texCoordVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(texCoords), texCoords, GL_STATIC_DRAW);
        CHECK_GL_ERROR();
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0); // Location = 2
        glEnableVertexAttribArray(2);
        CHECK_GL_ERROR();

        glBindVertexArray(0);
        CHECK_GL_ERROR();

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
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)win->w / win->h, 0.1f, 100.0f);
        shaderProgram.setUniform("model", model);
        shaderProgram.setUniform("view", view);
        shaderProgram.setUniform("projection", projection);
        CHECK_GL_ERROR();

        glGenTextures(1, &texture);
        CHECK_GL_ERROR();
        glBindTexture(GL_TEXTURE_2D, texture);
        CHECK_GL_ERROR();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        CHECK_GL_ERROR();

        SDL_Surface *surface = png::LoadPNG(win->util.getFilePath("data/bg.png").c_str());
        if (!surface) {
            mx::system_err << "Error: loading PNG.\n";
            exit(EXIT_FAILURE);
        }
        SDL_Surface *converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
        SDL_FreeSurface(surface);
        surface = converted;
        surface = mx::Texture::flipSurface(surface);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
        CHECK_GL_ERROR();
        SDL_FreeSurface(surface);
        glBindTexture(GL_TEXTURE_2D, 0);
        CHECK_GL_ERROR();
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
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationAngle), glm::vec3(1.0f, 1.0f, 0.0f)); // Rotate around X-axis
        glm::mat4 viewMatrix = glm::lookAt(cameraPos, cameraTarget, cameraUp);
        glm::mat4 projectionMatrix = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);

        glm::vec3 lightPos(2.0f, 4.0f, 1.0f);
        glm::vec3 viewPos = cameraPos; // Viewer position is the camera position
        glm::vec3 lightColor(1.0f, 1.0f, 1.0f);

        shaderProgram.setUniform("model", modelMatrix);
        shaderProgram.setUniform("view", viewMatrix);
        shaderProgram.setUniform("projection", projectionMatrix);
        shaderProgram.setUniform("lightPos", lightPos);
        shaderProgram.setUniform("viewPos", viewPos);
        shaderProgram.setUniform("lightColor", lightColor);

        int vertexCount = sizeof(positions) / (sizeof(GLfloat) * 3);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        glBindVertexArray(0);
    }
    virtual void event(gl::GLWindow *window, SDL_Event &e) override {
    }
private:
    
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("OpenGL Example", tw, th) {
        setPath(path);
        setObject(new Pyramid());
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
    int tw = 960, th = 720;
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