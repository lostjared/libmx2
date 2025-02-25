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
        cubeData.resize(vertexCount * 3);   

        for (size_t i = 0; i < vertexCount; ++i) {
            cubeData[i * 3] = vertices[i * 3];         // Position X
            cubeData[i * 3 + 1] = vertices[i * 3 + 1]; // Position Y
            cubeData[i * 3 + 2] = vertices[i * 3 + 2]; // Position Z
        }

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, cubeData.size() * sizeof(GLfloat), cubeData.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
        if (!shaderProgram.loadProgram(win->util.getFilePath("data/cube.vert"), win->util.getFilePath("data/cube.frag"))) {
            throw mx::Exception("Failed to load shader program");
        }
        shaderProgram.useProgram();
        shaderProgram.setUniform("cubemapTexture", 0);
        glUniform2f(glGetUniformLocation(shaderProgram.id(), "iResolution"), win->w, win->h);
        glm::mat4 model = glm::mat4(1.0f); 
        glm::mat4 view = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 5.0f),  
            glm::vec3(0.0f, 0.0f, 0.0f),  
            glm::vec3(0.0f, 1.0f, 0.0f)   
        );
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)win->w / win->h, 0.1f, 100.0f);
        shaderProgram.setUniform("model", model);
        shaderProgram.setUniform("view", view);
        shaderProgram.setUniform("projection", projection);
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
        std::vector<std::string> faces = {
            win->util.getFilePath("data/cm_right.png"),
            win->util.getFilePath("data/cm_left.png"),
            win->util.getFilePath("data/cm_top.png"),
            win->util.getFilePath("data/cm_bottom.png"),
            win->util.getFilePath("data/cm_back.png"),
            win->util.getFilePath("data/cm_front.png")
        };

        for (unsigned int i = 0; i < faces.size(); i++) {
            SDL_Surface *surface = png::LoadPNG(faces[i].c_str());
            if (!surface) {
                mx::system_err << "Error: loading PNG " << faces[i] << "\n";
                exit(EXIT_FAILURE);
            }
            
            std::cout << "Loading texture " << i << ": " << faces[i] << "\n";
            std::cout << "  Size: " << surface->w << "x" << surface->h << "\n";
            std::cout << "  BytesPerPixel: " << (int)surface->format->BytesPerPixel << "\n";
            
            GLenum format = (surface->format->BytesPerPixel == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, 
                         surface->w, surface->h, 0, format, 
                         GL_UNSIGNED_BYTE, surface->pixels);
            CHECK_GL_ERROR();
            SDL_FreeSurface(surface);
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        CHECK_GL_ERROR();
    }

    virtual void draw(gl::GLWindow *win) override {
        
        glEnable(GL_DEPTH_TEST);
        
        shaderProgram.useProgram();
        glm::mat4 view = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 5.0f),  
            glm::vec3(0.0f, 0.0f, 0.0f),  
            glm::vec3(0.0f, 1.0f, 0.0f)   
        );
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 
                                              (float)win->w / win->h, 
                                              0.1f, 100.0f);
        static float rotationAngle = 0.0f;
        static Uint32 lastTime = SDL_GetTicks(); 
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;
        rotationAngle += deltaTime * 50.0f; 
        static float time_f = 0.0f;
        time_f += deltaTime;

        glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(rotationAngle), glm::vec3(1.0f, 1.0f, 0.0f));
    
        shaderProgram.setUniform("model", model);
        shaderProgram.setUniform("view", view);
        shaderProgram.setUniform("projection", projection);
        shaderProgram.setUniform("time_f", time_f);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
        CHECK_GL_ERROR();
        
        glBindVertexArray(VAO);
        size_t vertexCount = cubeData.size() / 3;
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        CHECK_GL_ERROR();
        
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
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
        glClearColor(0.0f, 0.0f,0.0f, 1.0f);  // Change background color to see if rendering is happening
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
    Arguments args = proc_args(argc, argv);
    try {
        MainWindow main_window(args.path, args.width, args.height);
        if(args.fullscreen)
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