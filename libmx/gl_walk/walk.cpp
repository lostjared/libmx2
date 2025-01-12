// Trying to learn this stuff
// A lot of this demo was Ai generated.
// I am getting the hang of it.

#include "mx.hpp"
#include "argz.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <GLES3/gl3.h>
#endif
#include "gl.hpp"
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

#define CHECK_GL_ERROR() \
{ \
    GLenum err = glGetError(); \
    if (err != GL_NO_ERROR) { \
        printf("OpenGL Error: %d at %s:%d\n", err, __FILE__, __LINE__); \
    } \
}

const float floorVertices[] = {
    0.0f, 0.0f, 0.0f, 500.0f, 0.0f, 0.0f, 500.0f, 0.0f, 500.0f, 0.0f, 0.0f, 500.0f
};

const unsigned int floorIndices[] = { 0, 1, 2, 0, 2, 3 };

class SceneProgram : public gl::GLObject {
public:
    gl::ShaderProgram shaderProgram;
    GLuint floorVAO, floorVBO, floorEBO;
    GLuint pillarVAO, pillarVBO, pillarEBO;
    glm::vec3 playerPos = glm::vec3(15.0f, 1.0f, 15.0f);
    glm::vec3 playerFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 playerUp = glm::vec3(0.0f, 1.0f, 0.0f);
    float yaw = -90.0f, pitch = 0.0f, playerSpeed = 3.5f, mouseSens = 0.05f;
    float deltaTime = 0.0f, lastFrameTime = 0.0f;
    std::vector<float> pillarVertices;
    std::vector<unsigned int> pillarIndices;

    SceneProgram() = default;

    ~SceneProgram() override {
        glDeleteBuffers(1, &floorEBO);
        glDeleteBuffers(1, &floorVBO);
        glDeleteVertexArrays(1, &floorVAO);
        glDeleteBuffers(1, &pillarEBO);
        glDeleteBuffers(1, &pillarVBO);
        glDeleteVertexArrays(1, &pillarVAO);
    }

    void load(gl::GLWindow *win) override {
        if (!shaderProgram.loadProgram(win->util.getFilePath("data/tri.vert"), win->util.getFilePath("data/tri.frag")))
            throw mx::Exception("Failed to load shader program");
        shaderProgram.useProgram();
        setupFloor();
        setupPillars();
        CHECK_GL_ERROR();
        SDL_SetRelativeMouseMode(SDL_TRUE);
    }

    void draw(gl::GLWindow *win) override {
        glEnable(GL_DEPTH_TEST);
        float currentTime = SDL_GetTicks() * 0.001f;
        deltaTime = currentTime - lastFrameTime;
        lastFrameTime = currentTime;
        updateCameraVectors();
        playerPos.y = 1.0f;

        glm::mat4 view = glm::lookAt(playerPos, playerPos + playerFront, playerUp);
        glm::mat4 projection = glm::perspective(glm::radians(60.0f), (float)win->w / win->h, 0.1f, 500.0f);
        glm::mat4 model = glm::mat4(1.0f);
        shaderProgram.useProgram();
        shaderProgram.setUniform("view", view);
        shaderProgram.setUniform("projection", projection);
        shaderProgram.setUniform("model", model);

        glBindVertexArray(floorVAO);
        shaderProgram.setUniform("uColor", glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glBindVertexArray(pillarVAO);
        shaderProgram.setUniform("uColor", glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
        glDrawElements(GL_TRIANGLES, (GLsizei)pillarIndices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        CHECK_GL_ERROR();
    }

    void event(gl::GLWindow *win, SDL_Event &e) override {
        const Uint8 *state = SDL_GetKeyboardState(NULL);
        float vel = playerSpeed * deltaTime;
        glm::vec3 frontXZ = glm::normalize(glm::vec3(playerFront.x, 0.0f, playerFront.z));
        if (state[SDL_SCANCODE_W]) playerPos += frontXZ * vel;
        if (state[SDL_SCANCODE_S]) playerPos -= frontXZ * vel;
        if (state[SDL_SCANCODE_A]) playerPos += glm::normalize(glm::cross(playerUp, frontXZ)) * vel;
        if (state[SDL_SCANCODE_D]) playerPos += glm::normalize(glm::cross(frontXZ, playerUp)) * vel;
        if (e.type == SDL_MOUSEMOTION) {
            float xoffset = (float)e.motion.xrel, yoffset = (float)e.motion.yrel;
            yaw += xoffset * mouseSens;
            pitch -= yoffset * mouseSens;
            if (pitch > 89.0f) pitch = 89.0f;
            if (pitch < -89.0f) pitch = -89.0f;
        }
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
            SDL_SetRelativeMouseMode(SDL_FALSE);
            win->quit();
        }
    }

private:
    void updateCameraVectors() {
        float ryaw = glm::radians(yaw), rpitch = glm::radians(pitch);
        glm::vec3 f;
        f.x = cos(ryaw) * cos(rpitch);
        f.y = sin(rpitch);
        f.z = sin(ryaw) * cos(rpitch);
        playerFront = glm::normalize(f);
    }

    void setupFloor() {
        glGenVertexArrays(1, &floorVAO);
        glGenBuffers(1, &floorVBO);
        glGenBuffers(1, &floorEBO);
        glBindVertexArray(floorVAO);
        glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), floorVertices, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, floorEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(floorIndices), floorIndices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }

    void setupPillars() {
        createScenePillars();
        glGenVertexArrays(1, &pillarVAO);
        glGenBuffers(1, &pillarVBO);
        glGenBuffers(1, &pillarEBO);
        glBindVertexArray(pillarVAO);
        glBindBuffer(GL_ARRAY_BUFFER, pillarVBO);
        glBufferData(GL_ARRAY_BUFFER, pillarVertices.size() * sizeof(float), pillarVertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pillarEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, pillarIndices.size() * sizeof(unsigned int), pillarIndices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }

    void createScenePillars() {
        addCylinder(10.0f, 10.0f, 0.5f, 4.0f, 16);
        addCylinder(20.0f, 25.0f, 0.5f, 6.0f, 16);
        addCylinder(30.0f, 15.0f, 0.4f, 5.0f, 16);
        addCylinder(40.0f, 40.0f, 0.7f, 5.0f, 16);
        addCylinder(5.0f, 45.0f, 0.3f, 3.0f, 16);
    }

    void addCylinder(float cx, float cz, float radius, float height, int segments) {
        unsigned baseIndex = (unsigned)(pillarVertices.size() / 3);
        pillarVertices.push_back(cx);
        pillarVertices.push_back(height);
        pillarVertices.push_back(cz);
        pillarVertices.push_back(cx);
        pillarVertices.push_back(0.0f);
        pillarVertices.push_back(cz);
        for (int i = 0; i < segments; i++) {
            float theta = 2.0f * 3.14159f * (float)i / (float)segments;
            float xoff = radius * cos(theta);
            float zoff = radius * sin(theta);
            pillarVertices.push_back(cx + xoff);
            pillarVertices.push_back(height);
            pillarVertices.push_back(cz + zoff);
        }
        for (int i = 0; i < segments; i++) {
            float theta = 2.0f * 3.14159f * (float)i / (float)segments;
            float xoff = radius * cos(theta);
            float zoff = radius * sin(theta);
            pillarVertices.push_back(cx + xoff);
            pillarVertices.push_back(0.0f);
            pillarVertices.push_back(cz + zoff);
        }
        unsigned topCenter = baseIndex, bottomCenter = baseIndex + 1;
        unsigned topRingStart = baseIndex + 2, botRingStart = baseIndex + 2 + segments;
        for (int i = 0; i < segments; i++) {
            unsigned v0 = topCenter, v1 = topRingStart + i, v2 = topRingStart + ((i + 1) % segments);
            pillarIndices.push_back(v0);
            pillarIndices.push_back(v1);
            pillarIndices.push_back(v2);
        }
        for (int i = 0; i < segments; i++) {
            unsigned v0 = bottomCenter, v1 = botRingStart + ((i + 1) % segments), v2 = botRingStart + i;
            pillarIndices.push_back(v0);
            pillarIndices.push_back(v1);
            pillarIndices.push_back(v2);
        }
        for (int i = 0; i < segments; i++) {
            unsigned t0 = topRingStart + i, t1 = topRingStart + ((i + 1) % segments);
            unsigned b0 = botRingStart + i, b1 = botRingStart + ((i + 1) % segments);
            pillarIndices.push_back(t0);
            pillarIndices.push_back(b0);
            pillarIndices.push_back(b1);
            pillarIndices.push_back(t0);
            pillarIndices.push_back(b1);
            pillarIndices.push_back(t1);
        }
    }
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("FPS Walk with Floor & Pillars", tw, th) {
        setPath(path);
        setObject(new SceneProgram());
        object->load(this);
    }

    ~MainWindow() override {}

    void event(SDL_Event &e) override {
        if (e.type == SDL_QUIT) quit();
    }

    void draw() override {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, w, h);
        object->draw(this);
        swap();
    }
};

MainWindow *main_w = nullptr;
void eventProc() { main_w->proc(); }

int main(int argc, char **argv) {
#ifdef __EMSCRIPTEN__
    MainWindow main_window("/", 960, 720);
    main_w = &main_window;
    emscripten_set_main_loop(eventProc, 0, 1);
#else
    Argz<std::string> parser(argc, argv);
    parser.addOptionSingle('h', "Display help message")
          .addOptionSingleValue('p', "assets path")
          .addOptionDoubleValue('P', "path", "assets path")
          .addOptionSingleValue('r', "Resolution WidthxHeight")
          .addOptionDoubleValue('R', "resolution", "Resolution WidthxHeight");

    Argument<std::string> arg;
    std::string path;
    int value = 0;
    int tw = 960, th = 720;

    try {
        while ((value = parser.proc(arg)) != -1) {
            switch (value) {
            case 'h':
            case 'v':
                parser.help(std::cout);
                exit(EXIT_SUCCESS);
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
                std::string left = arg.arg_value.substr(0, pos);
                std::string right = arg.arg_value.substr(pos + 1);
                tw = atoi(left.c_str());
                th = atoi(right.c_str());
            }
                break;
            }
        }
    } catch (const ArgException<std::string> &e) {
        mx::system_err << e.text() << "\n";
    }

    if (path.empty()) {
        mx::system_out << "No path provided, using current directory.\n";
        path = ".";
    }

    try {
        MainWindow main_window(path, tw, th);
        main_w = &main_window;
        main_window.loop();
    } catch (const mx::Exception &e) {
        mx::system_err << "Exception: " << e.text() << "\n";
        mx::system_err.flush();
        exit(EXIT_FAILURE);
    }
#endif
    return 0;
}
