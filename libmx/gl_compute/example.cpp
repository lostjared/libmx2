#include "argz.hpp"
#include "gl.hpp"
#include "loadpng.hpp"
#include "model.hpp"
#include "mx.hpp"

#ifndef MX2_COMPUTE
#error "gl_compute requires MX2_COMPUTE (build with -DCOMPUTE=ON)"
#endif

#define CHECK_GL_ERROR()                                                    \
    {                                                                       \
        GLenum err = glGetError();                                          \
        if (err != GL_NO_ERROR)                                             \
            printf("OpenGL Error: %d at %s:%d\n", err, __FILE__, __LINE__); \
    }

class Compute : public gl::GLObject {
  private:
    gl::ShaderProgram computeProgram;
    gl::ShaderProgram renderProgram;
    GLuint textureID;
    GLuint quadVAO;
    GLuint quadVBO;
    int frameCount;
    int texWidth = 640;
    int texHeight = 480;

  public:
    Compute() : textureID(0), quadVAO(0), quadVBO(0), frameCount(0) {}

    void load(gl::GLWindow *win) {
        const char *computeSource = R"glsl(
            #version 430 core
            layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
            layout (rgba8, binding = 0) uniform image2D destTex;

            void main() {
                ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
                ivec2 imgSize = imageSize(destTex);
                if(pos.x >= imgSize.x || pos.y >= imgSize.y) return;

                vec4 currentPixel = imageLoad(destTex, pos);
                ivec2 nR = ivec2((pos.x + 1 + imgSize.x) % imgSize.x, pos.y);
                ivec2 nG = ivec2(pos.x, (pos.y + 1 + imgSize.y) % imgSize.y);
                ivec2 nB = ivec2((pos.x + 1 + imgSize.x) % imgSize.x, (pos.y + 1 + imgSize.y) % imgSize.y);
                vec4 evolvedPixel = vec4(
                    mix(imageLoad(destTex, nR).r, currentPixel.r, 0.5),
                    mix(imageLoad(destTex, nG).g, currentPixel.g, 0.6),
                    mix(imageLoad(destTex, nB).b, currentPixel.b, 0.7),
                    1.0
                );
                imageStore(destTex, pos, evolvedPixel);
            }
        )glsl";

        const char *vertexSource = R"glsl(
            #version 430 core
            layout (location = 0) in vec2 aPos;
            layout (location = 1) in vec2 aUV;
            out vec2 vUV;

            void main() {
                vUV = aUV;
                gl_Position = vec4(aPos, 0.0, 1.0);
            }
        )glsl";

        const char *fragmentSource = R"glsl(
            #version 430 core
            in vec2 vUV;
            out vec4 FragColor;
            uniform sampler2D screenTex;

            void main() {
                FragColor = texture(screenTex, vUV);
            }
        )glsl";

        computeProgram.setName("compute");
        if (!computeProgram.loadComputeFromText(computeSource)) {
            throw mx::Exception("Failed to load compute shader");
        }

        renderProgram.setName("render");
        if (!renderProgram.loadProgramFromText(vertexSource, fragmentSource)) {
            throw mx::Exception("Failed to load render shader");
        }

        textureID = gl::loadTexture(win->util.getFilePath("data/bg.png"), texWidth, texHeight);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glBindImageTexture(0, textureID, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);

        float quadVertices[] = {
            -1.0f, -1.0f, 0.0f, 0.0f,
            1.0f, -1.0f, 1.0f, 0.0f,
            -1.0f, 1.0f, 0.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 1.0f};

        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        CHECK_GL_ERROR();
    }

    void draw(gl::GLWindow *win) {
        GLuint numGroupsX = (texWidth + 15) / 16;
        GLuint numGroupsY = (texHeight + 15) / 16;
        computeProgram.dispatchCompute(numGroupsX, numGroupsY, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
        renderProgram.useProgram();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        renderProgram.setUniform("screenTex", 0);
        glBindVertexArray(quadVAO);
        glDisable(GL_DEPTH_TEST);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
        ++frameCount;
    }

    void event(gl::GLWindow *win, SDL_Event &e) {}

    ~Compute() {
        if (textureID)
            glDeleteTextures(1, &textureID);
        if (quadVBO)
            glDeleteBuffers(1, &quadVBO);
        if (quadVAO)
            glDeleteVertexArrays(1, &quadVAO);
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
        .addOptionSingleValue('r', "Resolution WidthxHeight")
        .addOptionDoubleValue('R', "resolution", "Resolution WidthxHeight");
    Argument<std::string> arg;
    std::string path;
    int value = 0;
    int tw = 1280, th = 720;
    try {
        while ((value = parser.proc(arg)) != -1) {
            switch (value) {
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
                if (pos == std::string::npos) {
                    mx::system_err << "Error invalid resolution use WidthxHeight\n";
                    mx::system_err.flush();
                    exit(EXIT_FAILURE);
                }
                std::string left, right;
                left = arg.arg_value.substr(0, pos);
                right = arg.arg_value.substr(pos + 1);
                tw = atoi(left.c_str());
                th = atoi(right.c_str());
            } break;
            }
        }
    } catch (const ArgException<std::string> &e) {
        mx::system_err << e.text() << "\n";
    }
    if (path.empty()) {
        mx::system_out << "mx: No path provided trying default current directory.\n";
        path = ".";
    }
    try {
        MainWindow main_window(path, tw, th);
        main_window.loop();
    } catch (const mx::Exception &e) {
        mx::system_err << "mx: Exception: " << e.text() << "\n";
        mx::system_err.flush();
        exit(EXIT_FAILURE);
    }
    return 0;
}