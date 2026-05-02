#include "argz.hpp"
#include "gl.hpp"
#include "gl_cv.hpp"
#include "loadpng.hpp"
#include "model.hpp"
#include "mx.hpp"
#include <array>
#include <cstdlib>
#include <ctime>

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
        static constexpr int HISTORY_SIZE = 32;

    gl::ShaderProgram computeProgram;
    gl::ShaderProgram renderProgram;
    mx::MXCapture capture;
    std::array<GLuint, HISTORY_SIZE> historyTextures;
    std::array<GLuint, 2> workTextures;
    GLuint outputTexture;
    GLuint quadVAO;
    GLuint quadVBO;
    int frameCount;
    int cameraIndex;
    int historyIndex;
    int historyCount;
    int texWidth = 1920;
    int texHeight = 1080;

    GLuint createFloatTexture(const void *data = nullptr) {
        GLuint texture = 0;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        return texture;
    }

    void uploadFrame(GLuint texture, const cv::Mat &rgba) {
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texWidth, texHeight, GL_RGBA, GL_UNSIGNED_BYTE, rgba.ptr());
    }

    void setTextureUniforms() {
        computeProgram.useProgram();
        computeProgram.setUniform("sourceTex", 0);
        for (int i = 0; i < HISTORY_SIZE; ++i) {
            computeProgram.setUniform("historyTex[" + std::to_string(i) + "]", i + 1);
        }
    }

    void bindHistoryTextures() {
        for (int i = 0; i < HISTORY_SIZE; ++i) {
            glActiveTexture(GL_TEXTURE1 + i);
            glBindTexture(GL_TEXTURE_2D, historyTextures[i]);
        }
    }

    void dispatchCompute(GLuint sourceTexture, GLuint destTexture, int mode) {
        computeProgram.useProgram();
        computeProgram.setUniform("mode", mode);
        computeProgram.setUniform("historyCount", historyCount);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sourceTexture);
        bindHistoryTextures();
        glBindImageTexture(0, destTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
        GLuint numGroupsX = (texWidth + 15) / 16;
        GLuint numGroupsY = (texHeight + 15) / 16;
        computeProgram.dispatchCompute(numGroupsX, numGroupsY, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    }

    static cv::Mat toDisplayFrame(const cv::Mat &frame) {
        cv::Mat rgba;
        if (frame.channels() == 4) {
            cv::cvtColor(frame, rgba, cv::COLOR_BGRA2RGBA);
        } else if (frame.channels() == 3) {
            cv::cvtColor(frame, rgba, cv::COLOR_BGR2RGBA);
        } else if (frame.channels() == 1) {
            cv::cvtColor(frame, rgba, cv::COLOR_GRAY2RGBA);
        } else {
            throw mx::Exception("Unsupported camera frame format");
        }
        return rgba;
    }

  public:
    explicit Compute(int cameraIndex)
        : historyTextures{}, workTextures{}, outputTexture(0), quadVAO(0), quadVBO(0), frameCount(0), cameraIndex(cameraIndex), historyIndex(0), historyCount(0) {
        static bool seeded = false;
        if (!seeded) {
            std::srand(static_cast<unsigned>(std::time(nullptr)));
            seeded = true;
        }
    }

    void load(gl::GLWindow *win) {
        /*
        const char *computeSource = R"glsl(
            #version 430 core
            layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
            layout (rgba16f, binding = 0) uniform image2D destTex;
            uniform sampler2D sourceTex;
            uniform sampler2D historyTex[8];
            uniform int mode;
            uniform int historyCount;

            float medianValue(float values[9]) {
                for (int i = 0; i < 8; ++i) {
                    for (int j = i + 1; j < 9; ++j) {
                        if (values[j] < values[i]) {
                            float temp = values[i];
                            values[i] = values[j];
                            values[j] = temp;
                        }
                    }
                }
                return values[4];
            }

            vec3 median3x3(ivec2 pos, ivec2 imgSize) {
                float red[9];
                float green[9];
                float blue[9];
                int index = 0;
                for (int y = -1; y <= 1; ++y) {
                    for (int x = -1; x <= 1; ++x) {
                        ivec2 samplePos = clamp(pos + ivec2(x, y), ivec2(0), imgSize - ivec2(1));
                        vec3 color = texelFetch(sourceTex, samplePos, 0).rgb;
                        red[index] = color.r;
                        green[index] = color.g;
                        blue[index] = color.b;
                        ++index;
                    }
                }
                return vec3(medianValue(red), medianValue(green), medianValue(blue));
            }

            uvec3 toByteColor(vec3 color) {
                return uvec3(round(clamp(color, 0.0, 1.0) * 255.0));
            }

            void main() {
                ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
                ivec2 imgSize = imageSize(destTex);
                if(pos.x >= imgSize.x || pos.y >= imgSize.y) return;

                if (mode == 0) {
                    vec3 blurred = median3x3(pos, imgSize);
                    imageStore(destTex, pos, vec4(blurred, 1.0));
                    return;
                }

                uvec3 currentPixel = toByteColor(texelFetch(sourceTex, pos, 0).rgb);
                uvec3 value = uvec3(0u);
                for (int j = 0; j < historyCount; ++j) {
                    value += toByteColor(texelFetch(historyTex[j], pos, 0).rgb);
                }
                uvec3 xorValue = (value + uvec3(1u)) & uvec3(255u);
                uvec3 result = currentPixel ^ xorValue;
                imageStore(destTex, pos, vec4(vec3(result) / 255.0, 1.0));
            }
        )glsl";*/

        const char *computeSource = R"glsl(
        #version 430 core
        layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

        layout (rgba16f, binding = 0) uniform image2D destTex;
        uniform sampler2D sourceTex;
        uniform sampler2D historyTex[32];

        uniform int mode;
        uniform int historyCount;
        uniform int square_size;
        uniform int start_history_index;
        uniform int history_dir;

        vec3 blur3x3(ivec2 pos, ivec2 imgSize) {
            vec3 sum = vec3(0.0);
            int count = 0;
            for (int y = -1; y <= 1; ++y) {
                for (int x = -1; x <= 1; ++x) {
                    ivec2 samplePos = clamp(pos + ivec2(x, y), ivec2(0), imgSize - ivec2(1));
                    sum += texelFetch(sourceTex, samplePos, 0).rgb;
                    ++count;
                }
            }
            return sum / float(count);
        }

        int wrapPingPongIndex(int idx, int maxIndex) {
            if (maxIndex <= 0) {
                return 0;
            }
            int cycle = maxIndex * 2;
            int wrapped = idx % cycle;
            if (wrapped < 0) {
                wrapped += cycle;
            }
            if (wrapped > maxIndex) {
                wrapped = cycle - wrapped;
            }
            return wrapped;
        }

        void main() {
            ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
            ivec2 imgSize = imageSize(destTex);
            if (pos.x >= imgSize.x || pos.y >= imgSize.y) {
                return;
            }

            if (mode == 0) {
                vec3 blurred = blur3x3(pos, imgSize);
                imageStore(destTex, pos, vec4(blurred, 1.0));
                return;
            }

            vec3 currentPixel = texelFetch(sourceTex, pos, 0).rgb;
            if (historyCount <= 0) {
                imageStore(destTex, pos, vec4(currentPixel, 1.0));
                return;
            }

            int safeSquare = max(square_size, 1);
            ivec2 blockPos = (pos / safeSquare) * safeSquare;
            int blockRow = blockPos.y / safeSquare;

            int maxHistoryIndex = min(historyCount, 32) - 1;
            int offset = blockRow * history_dir;
            int rawIndex = start_history_index + offset;
            int historyIndex = wrapPingPongIndex(rawIndex, maxHistoryIndex);

            vec3 historyPixel = texelFetch(historyTex[historyIndex], pos, 0).rgb;
            vec3 blended = mix(currentPixel, historyPixel, 0.5);
            imageStore(destTex, pos, vec4(blended, 1.0));
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
                vec3 h_color = texture(screenTex, vUV).rgb;
                vec3 mapped = h_color;
                //vec3 mapped = h_color / (h_color + vec3(1.0));
                FragColor = vec4(mapped, 1.0);
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

        if (!capture.open(cameraIndex)) {
            throw mx::Exception("Failed to open camera index: " + std::to_string(cameraIndex));
        }
        
        capture.set(cv::CAP_PROP_FRAME_WIDTH, 1920);
        capture.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);
        capture.set(cv::CAP_PROP_FPS, 60.0);
        
        cv::Mat frame;
        if (!capture.read(frame) || frame.empty()) {
            throw mx::Exception("Failed to read initial camera frame");
        }

        cv::Mat rgba = toDisplayFrame(frame);
        texWidth = rgba.cols;
        texHeight = rgba.rows;

        workTextures[0] = createFloatTexture(rgba.ptr());
        workTextures[1] = createFloatTexture();
        outputTexture = createFloatTexture();
        for (GLuint &texture : historyTextures) {
            texture = createFloatTexture();
        }
        setTextureUniforms();

        float quadVertices[] = {
            -1.0f, -1.0f, 0.0f, 1.0f, // Bottom-Left position -> Top-Left UV
            1.0f, -1.0f, 1.0f, 1.0f, // Bottom-Right position -> Top-Right UV
            -1.0f,  1.0f, 0.0f, 0.0f, // Top-Left position -> Bottom-Left UV
            1.0f,  1.0f, 1.0f, 0.0f  // Top-Right position -> Bottom-Right UV
        };
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

    int current_square_size = 4, square_dir = 1;
    int current_index = 0;
    int current_dir = 1;

    void draw(gl::GLWindow *win) {
        cv::Mat frame;
        if (capture.read(frame) && !frame.empty()) {
            cv::Mat rgba = toDisplayFrame(frame);
            if (rgba.cols == texWidth && rgba.rows == texHeight) {
                uploadFrame(workTextures[0], rgba);
            }
        }

        computeProgram.useProgram();
        computeProgram.setUniform("square_size", current_square_size);
        computeProgram.setUniform("start_history_index", current_index);
        computeProgram.setUniform("history_dir", current_dir);

        if(current_dir == 1) {
            ++current_index;
            if(current_index > HISTORY_SIZE - 1) {
                current_index = HISTORY_SIZE - 1;
                current_dir = -1;
            }
        } else {
            --current_index;
            if(current_index <= 0) {
                current_index = 0;
                current_dir = 1;
            }
        }
        if(square_dir == 1) {
            current_square_size += 2;
            if(current_square_size >= 64) {
                current_square_size = 64;
                square_dir = 0;
            }
        } else {
            current_square_size -= 2;
            if(current_square_size <= 2) {
                current_square_size = 2;
                square_dir = 1;
            }
        }

        int sourceIndex = 0;
        int destIndex = 1;
        const int passes = 3 + (std::rand() % 7);
        for (int i = 0; i < passes; ++i) {
            dispatchCompute(workTextures[sourceIndex], workTextures[destIndex], 0);
            std::swap(sourceIndex, destIndex);
        }

        glCopyImageSubData(workTextures[sourceIndex], GL_TEXTURE_2D, 0, 0, 0, 0,
                           historyTextures[historyIndex], GL_TEXTURE_2D, 0, 0, 0, 0,
                           texWidth, texHeight, 1);
        if (historyCount < HISTORY_SIZE) {
            ++historyCount;
        }
        historyIndex = (historyIndex + 1) % HISTORY_SIZE;
        dispatchCompute(workTextures[sourceIndex], outputTexture, 1);
        renderProgram.useProgram();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, outputTexture);
        renderProgram.setUniform("screenTex", 0);
        glBindVertexArray(quadVAO);
        glDisable(GL_DEPTH_TEST);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
        ++frameCount;
    }

    void event(gl::GLWindow *win, SDL_Event &e) {}

    ~Compute() {
        capture.close();
        for (GLuint &texture : historyTextures) {
            if (texture)
                glDeleteTextures(1, &texture);
        }
        for (GLuint &texture : workTextures) {
            if (texture)
                glDeleteTextures(1, &texture);
        }
        if (outputTexture)
            glDeleteTextures(1, &outputTexture);
        if (quadVBO)
            glDeleteBuffers(1, &quadVBO);
        if (quadVAO)
            glDeleteVertexArrays(1, &quadVAO);
    }
};

class MainWindow : public gl::GLWindow {
  public:
        MainWindow(std::string path, int tw, int th, int cameraIndex) : gl::GLWindow("Compute Shader", tw, th, true, gl::GLMode::DESKTOP, 4, 3) {
        setPath(path);
                setObject(new Compute(cameraIndex));
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
        .addOptionSingleValue('c', "Camera index")
        .addOptionDoubleValue('C', "camera", "Camera index")
        .addOptionSingleValue('p', "assets path")
        .addOptionDoubleValue('P', "path", "assets path")
        .addOptionSingleValue('r', "Resolution WidthxHeight")
        .addOptionDoubleValue('R', "resolution", "Resolution WidthxHeight");
    Argument<std::string> arg;
    std::string path;
    int value = 0;
    int cameraIndex = 0;
    int tw = 1920, th = 1080;
    try {
        while ((value = parser.proc(arg)) != -1) {
            switch (value) {
            case 'h':
            case 'v':
                parser.help(std::cout);
                exit(EXIT_SUCCESS);
                break;
            case 'c':
            case 'C':
                cameraIndex = atoi(arg.arg_value.c_str());
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
        MainWindow main_window(path, tw, th, cameraIndex);
        main_window.loop();
    } catch (const mx::Exception &e) {
        mx::system_err << "mx: Exception: " << e.text() << "\n";
        mx::system_err.flush();
        exit(EXIT_FAILURE);
    }
    return 0;
}