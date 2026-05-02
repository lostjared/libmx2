#include"mx.hpp"
#include"argz.hpp"
#include"gl.hpp"
#include"loadpng.hpp"
#include"model.hpp"


#define CHECK_GL_ERROR() \
    { GLenum err = glGetError(); \
      if (err != GL_NO_ERROR) \
        printf("OpenGL Error: %d at %s:%d\n", err, __FILE__, __LINE__); }

class Compute : public gl::GLObject {
private:
    GLuint computeProgram;
    GLuint renderProgram;
    GLuint textureID;
    GLuint quadVAO;
    GLuint quadVBO;
    int frameCount;
    bool computeReady;
    int texWidth = 512;
    int texHeight = 512;

    void checkCompileErrors(GLuint shader, std::string type) {
        GLint success;
        GLchar infoLog[1024];
        if (type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                mx::system_err << "libmx2 ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n";
            }
            else {
                mx::system_out << "libmx2 program compilation succesfull.\n";
            }
        } else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                mx::system_err << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n";
            } else {

                mx::system_out << "libmx2: compute shader linked.\n";
            }
        }
    }

public:
    Compute() : computeProgram(0), renderProgram(0), textureID(0), quadVAO(0), quadVBO(0), frameCount(0), computeReady(false) {}

    void load(gl::GLWindow *win) {
        const char* computeSource = R"glsl(
            #version 430 core
            layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
            layout (rgba8, binding = 0) uniform image2D destTex;
            uniform int uFrame;

            void main() {
                ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
                ivec2 imgSize = imageSize(destTex);
                if(pos.x >= imgSize.x || pos.y >= imgSize.y) return;

                if (uFrame == 0) {
                    float r = float(pos.x ^ pos.y) / 255.0;
                    float g = float(pos.x % 128) / 128.0;
                    float b = float(pos.y % 128) / 128.0;
                    imageStore(destTex, pos, vec4(r, g, b, 1.0));
                    return;
                }

                vec4 currentPixel = imageLoad(destTex, pos);
                ivec2 neighborPos = ivec2(pos.x, (pos.y + 1) % imgSize.y);
                vec4 neighborPixel = imageLoad(destTex, neighborPos);
                vec4 evolvedPixel = vec4(
                    (currentPixel.g + neighborPixel.r) * 0.505, // Slight boost to prevent fading to black
                    (currentPixel.b + neighborPixel.g) * 0.495, // Slight decay
                    (currentPixel.r + neighborPixel.b) * 0.5,
                    1.0
                );
                 imageStore(destTex, pos, evolvedPixel);
            }
        )glsl";

        const char* vertexSource = R"glsl(
            #version 430 core
            layout (location = 0) in vec2 aPos;
            layout (location = 1) in vec2 aUV;
            out vec2 vUV;

            void main() {
                vUV = aUV;
                gl_Position = vec4(aPos, 0.0, 1.0);
            }
        )glsl";

        const char* fragmentSource = R"glsl(
            #version 430 core
            in vec2 vUV;
            out vec4 FragColor;
            uniform sampler2D screenTex;

            void main() {
                FragColor = texture(screenTex, vUV);
            }
        )glsl";

        // 2. Compile the Compute Shader
        GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
        glShaderSource(computeShader, 1, &computeSource, NULL);
        glCompileShader(computeShader);
        checkCompileErrors(computeShader, "COMPUTE");

        // 3. Link into a Program
        computeProgram = glCreateProgram();
        glAttachShader(computeProgram, computeShader);
        glLinkProgram(computeProgram);
        checkCompileErrors(computeProgram, "PROGRAM");

        // Cleanup the shader object as it is now linked into the program
        glDeleteShader(computeShader);

        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexSource, NULL);
        glCompileShader(vertexShader);
        checkCompileErrors(vertexShader, "VERTEX");

        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
        glCompileShader(fragmentShader);
        checkCompileErrors(fragmentShader, "FRAGMENT");

        renderProgram = glCreateProgram();
        glAttachShader(renderProgram, vertexShader);
        glAttachShader(renderProgram, fragmentShader);
        glLinkProgram(renderProgram);
        checkCompileErrors(renderProgram, "PROGRAM");

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        // 4. Create a Texture to store the compute shader output
        glGenTextures(1, &textureID);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        
        // Define texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        
        // Allocate immutable storage for the texture (required for Image Load/Store)
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, texWidth, texHeight);
        
        // Bind the texture to Image Unit 0 so the compute shader can write to it
        glBindImageTexture(0, textureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

        float quadVertices[] = {
            -1.0f, -1.0f, 0.0f, 0.0f,
             1.0f, -1.0f, 1.0f, 0.0f,
            -1.0f,  1.0f, 0.0f, 1.0f,
             1.0f,  1.0f, 1.0f, 1.0f
        };

        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        
        CHECK_GL_ERROR();
    }

    void draw(gl::GLWindow *win) {
        if (!computeReady) {
            return;
        }
        glUseProgram(computeProgram);
        glUniform1i(glGetUniformLocation(computeProgram, "uFrame"), frameCount);
        GLuint numGroupsX = (texWidth + 15) / 16;
        GLuint numGroupsY = (texHeight + 15) / 16;
        glDispatchCompute(numGroupsX, numGroupsY, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
        glUseProgram(renderProgram);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glUniform1i(glGetUniformLocation(renderProgram, "screenTex"), 0);        
        glBindVertexArray(quadVAO);
        glDisable(GL_DEPTH_TEST);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
        ++frameCount;
    }

    void event(gl::GLWindow *win, SDL_Event &e) {
        // Handle input events here
    }

    ~Compute() {
        if (computeProgram) glDeleteProgram(computeProgram);
        if (renderProgram) glDeleteProgram(renderProgram);
        if (textureID) glDeleteTextures(1, &textureID);
        if (quadVBO) glDeleteBuffers(1, &quadVBO);
        if (quadVAO) glDeleteVertexArrays(1, &quadVAO);
    }
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("Compute Shader", tw, th, false, gl::GLMode::DESKTOP, 4, 3) {
        setPath(path);
        setObject(new Compute());
		object->load(this);
    }
    
    ~MainWindow() override {}
    
    virtual void event(SDL_Event &e) override {}

    virtual void draw() override {
        glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glViewport(0, 0, w, h);
        object->draw(this);
        swap();
    }
private:
};


int main(int argc, char **argv) {
    Argz<std::string> parser(argc, argv);    
    parser.addOptionSingle('h', "Display help message")
          .addOptionSingleValue('p', "assets path")
          .addOptionDoubleValue('P', "path", "assets path")
          .addOptionSingleValue('r',"Resolution WidthxHeight")
          .addOptionDoubleValue('R',"resolution", "Resolution WidthxHeight");
    Argument<std::string> arg;
    std::string path;
    int value = 0;
    int tw = 1280, th = 720;
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
    return 0;
}