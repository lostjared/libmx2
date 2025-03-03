#include"mx.hpp"
#include"argz.hpp"

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
        printf("OpenGL Error: %d at %s:%d\n", err, __FILE__, __LINE__); }
#if defined(__EMSCRIPTEN__) || defined(__ANDOIRD__)
    const char *g_vSource = R"(#version 300 es
        precision highp float;
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec2 aTexCoords;
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        uniform vec3 lightPos;
        uniform vec3 viewPos;
        uniform vec3 lightColor;
        uniform vec3 objectColor;

        out vec3 vertexColor;
        out vec2 TexCoords;

        void main()
        {
            vec4 worldPos = model * vec4(aPos, 1.0);
            gl_Position = projection * view * worldPos;
            vec3 norm = normalize(mat3(transpose(inverse(model))) * aNormal);
            vec3 lightDir = normalize(lightPos - vec3(worldPos));
            float ambientStrength = 0.1;
            vec3 ambient = ambientStrength * lightColor;
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * lightColor;
            float specularStrength = 1.0;    // Increase from 0.5 to 1.0 for more shine
            float shininess = 64.0;          // Increase this value for a tighter, shinier highlight
            vec3 viewDir = normalize(viewPos - vec3(worldPos));
            vec3 reflectDir = reflect(-lightDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
            vec3 specular = specularStrength * spec * lightColor;
            vec3 finalColor = (ambient + diffuse + specular) * objectColor;
            vertexColor = finalColor;
            TexCoords = aTexCoords;
        }
    )";

    const char *g_fSource = R"(#version 300 es
            precision highp float;
            in vec3 vertexColor;
            in vec2 TexCoords;
            uniform sampler2D texture1;  
            out vec4 FragColor;

            void main()
            {
                FragColor = vec4(vertexColor, 1.0) * texture(texture1, TexCoords);
                FragColor.a =  0.5;
            }
    )";
    #else
    const char *g_vSource = R"(#version 330 core
        layout (location = 0) in vec3 aPos;

        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec2 aTexCoords;
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        uniform vec3 lightPos;
        uniform vec3 viewPos;
        uniform vec3 lightColor;
        uniform vec3 objectColor;

        out vec3 vertexColor;
        out vec2 TexCoords;

        void main()
        {
            vec4 worldPos = model * vec4(aPos, 1.0);
            gl_Position = projection * view * worldPos;
            vec3 norm = normalize(mat3(transpose(inverse(model))) * aNormal);
            vec3 lightDir = normalize(lightPos - vec3(worldPos));
            float ambientStrength = 0.1;
            vec3 ambient = ambientStrength * lightColor;
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * lightColor;
            float specularStrength = 1.0;    // Increase from 0.5 to 1.0 for more shine
            float shininess = 64.0;          // Increase this value for a tighter, shinier highlight
            vec3 viewDir = normalize(viewPos - vec3(worldPos));
            vec3 reflectDir = reflect(-lightDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
            vec3 specular = specularStrength * spec * lightColor;
            vec3 finalColor = (ambient + diffuse + specular) * objectColor;
            vertexColor = finalColor;
            TexCoords = aTexCoords;
        }

    )";
    const char *g_fSource = R"(#version 330 core
            in vec3 vertexColor;
            in vec2 TexCoords;
            uniform sampler2D texture1;  
            out vec4 FragColor;

            void main()
            {
                FragColor = vec4(vertexColor, 1.0) * texture(texture1, TexCoords);
                FragColor.a = 0.5;
            }
    )";
#endif

class Cube : public gl::GLObject {
public:
    gl::ShaderProgram shaderProgram;
    GLuint VAO, VBO;
    GLuint texture;
    mx::Model cube;        
    mx::Font font;
    Cube() = default;
    virtual ~Cube() override {
    }

    std::vector<GLfloat> cubeData;

    virtual void load(gl::GLWindow *win) override {     
        font.loadFont(win->util.getFilePath("data/font.ttf"), 18);
        if(cube.openModel(win->util.getFilePath("data/cube.mxmod.z")) == false) {
            throw mx::Exception("Error loading model");
        }
        if (!shaderProgram.loadProgramFromText(g_vSource, g_fSource)) {
            throw mx::Exception("Failed to load shader program");
        }
        shaderProgram.useProgram();
        cube.setTextures(win, win->util.getFilePath("data/cube.tex"), win->util.getFilePath("data"));
    }

    virtual void draw(gl::GLWindow *win) override {
        shaderProgram.useProgram();
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
        cube.setShaderProgram(&shaderProgram, "texture1");
        shaderProgram.useProgram();
        cameraPosition = glm::vec3(0.0f, 2.0f, 5.0f);
        glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);
        modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::scale(modelMatrix, glm::vec3(2.0f, 2.0f, 2.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationAngle), glm::vec3(rot_x, 1.0f, 0.0f));
        viewMatrix = glm::lookAt(cameraPosition, cameraTarget, upVector);
        float aspectRatio = static_cast<float>(win->w) / static_cast<float>(win->h);
        projectionMatrix = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);      
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram.id(), "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram.id(), "view"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram.id(), "projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
        glUniform3f(glGetUniformLocation(shaderProgram.id(), "lightPos"), lightPos.x, lightPos.y, lightPos.z);
        glUniform3f(glGetUniformLocation(shaderProgram.id(), "viewPos"), cameraPosition.x, cameraPosition.y, cameraPosition.z);
        glUniform3f(glGetUniformLocation(shaderProgram.id(), "lightColor"), 1.0f, 1.0f, 1.0f);
        glUniform3f(glGetUniformLocation(shaderProgram.id(), "objectColor"), 1.0f, 1.0f, 1.0f); 
        CHECK_GL_ERROR();
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        cube.drawArrays();
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        win->text.setColor({255, 255, 255, 255});
        win->text.printText_Solid(font, 25.0f, 25.0f, "Press Arrow keys / Page Up Down to move light source Enter to reset");
        win->text.printText_Solid(font, 25.0f, 60.0f, "Press Space to rotate cube");
    }
  
    void event(gl::GLWindow *win, SDL_Event &e) override {
        if (e.type == SDL_KEYDOWN) {
            float moveSpeed = 0.05f; 
            switch (e.key.keysym.sym) {
                case SDLK_LEFT:
                    lightPos.x -= moveSpeed;
                    break;
                case SDLK_RIGHT:
                    lightPos.x += moveSpeed;
                    break;
                case SDLK_UP:
                    lightPos.z -= moveSpeed;
                    break;
                case SDLK_DOWN:
                    lightPos.z += moveSpeed;
                    break;
                case SDLK_PAGEUP:
                    lightPos.y += moveSpeed;
                    break;
                case SDLK_PAGEDOWN:
                    lightPos.y -= moveSpeed;
                    break;
                case SDLK_RETURN:
                    lightPos = glm::vec3(0.0f, 5.0f, 0.0f);
                    break;
                case SDLK_SPACE:
                    if(rot_x >  0) rot_x = 0; else rot_x = 1.0f;
                    break;
            }
            std::cout << "Light Position: " << lightPos.x << ", " << lightPos.y << ", " << lightPos.z << "\n";
        }
    }
private:
    glm::mat4 modelMatrix{1.0f};
    glm::mat4 viewMatrix{1.0f};
    glm::mat4 projectionMatrix{1.0f};
    glm::vec3 cameraPosition{0.0f, 2.0f, 5.0f};
    glm::vec3 lightPos{0.0f, 5.0f, 0.0f};
    float rot_x =  1.0f;
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("Transparent Rotating Cube", tw, th) {
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