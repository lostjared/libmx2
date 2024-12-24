#include"mx.hpp"
#include"argz.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <GLES3/gl3.h>
#endif

#include"gl.hpp"
#include"loadpng.hpp"
#include<random>

#define CHECK_GL_ERROR() \
{ GLenum err = glGetError(); \
if (err != GL_NO_ERROR) \
printf("OpenGL Error: %d at %s:%d\n", err, __FILE__, __LINE__); }

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const char* vertSource = R"(#version 330 core

layout (location = 0) in vec2 inPosition; 
layout (location = 1) in float inSize;    
layout (location = 2) in vec4 inColor;    

out vec4 fragColor; 

void main() {
    gl_Position = vec4(inPosition, 0.0, 1.0); 
    gl_PointSize = inSize;                   
    fragColor = inColor;                     
}
)";

const char* fragSource = R"(#version 330 core
in vec4 fragColor;           
in vec2 texCoord;            
out vec4 FragColor;          
uniform sampler2D spriteTexture; 
void main() {
    float dist = length(gl_PointCoord - vec2(0.5)); 
    if (dist > 0.5) {
        discard; 
    }
    vec4 texColor = texture(spriteTexture, gl_PointCoord); 
    FragColor = texColor * fragColor;
}
)";

float generateRandomFloat(float min, float max) {
    std::random_device rd; 
    std::default_random_engine eng(rd()); 
    std::uniform_real_distribution<float> dist(min, max);
    return dist(eng);
}



class Game : public gl::GLObject {
public:

    struct Particle {
        float x, y, vx, vy, life, angle;
    };

    static constexpr int NUM_PARTICLES = 1000;
    gl::ShaderProgram program;
    std::vector<Particle> particles;
    GLuint VAO, VBO[3];
    GLuint texture;
    Game() : particles(NUM_PARTICLES) {}

    ~Game() override {
         glDeleteVertexArrays(1, &VAO);
         glDeleteBuffers(3, VBO);
         glDeleteTextures(1, &texture);
    }

    void load(gl::GLWindow *win) override {
        font.loadFont(win->util.getFilePath("data/font.ttf"), 36);
        if(!program.loadProgramFromText(vertSource, fragSource)) {
            throw mx::Exception("Error loading shader");
        }
        for (auto& p : particles) {
            p.x = (rand() % 200 - 100) / 100.0f; 
            p.y = (rand() % 200 - 100) / 100.0f;
            p.vx = 0.0;
            p.vy = generateRandomFloat(-0.01f, -0.2f);
            p.life = 1.0f;
            p.angle = generateRandomFloat(0.0f, 1.0f);
        }

        glGenVertexArrays(1, &VAO);
        glGenBuffers(3, VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
        glBufferData(GL_ARRAY_BUFFER, NUM_PARTICLES * 2 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
        glBufferData(GL_ARRAY_BUFFER, NUM_PARTICLES * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
        glBufferData(GL_ARRAY_BUFFER, NUM_PARTICLES * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
        texture = gl::loadTexture(win->util.getFilePath("data/snowball.png"));
    }

    void draw(gl::GLWindow *win) override {  
        glEnable(GL_PROGRAM_POINT_SIZE);
        glDisable(GL_DEPTH_TEST);
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; // Convert to seconds
        lastUpdateTime = currentTime;
        update(deltaTime);
        CHECK_GL_ERROR();
        program.useProgram();
        program.setUniform("spriteTexture", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindVertexArray(VAO);
        glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);
        CHECK_GL_ERROR();
        win->text.setColor({255, 0, 0, 255});
        win->text.printText_Solid(font, 25.0f, 25.0f, "Paritcle Count: " + std::to_string(NUM_PARTICLES)); 
      
    }   
    
    void event(gl::GLWindow *win, SDL_Event &e) override {}
    
    void update(float deltaTime) {
        if(deltaTime > 0.1f)
            deltaTime = 0.1f;
        std::vector<float> positions, sizes, colors;

        float circularSpeed = 0.1f; 
        float descentSpeed = -0.5f; 
        float radius = 0.1f;        

        for (auto& p : particles) {
            p.angle += circularSpeed * deltaTime;
            p.x += radius * cos(p.angle);
            p.y += descentSpeed * deltaTime;

            p.life -= 0.05f;
            if(p.life <= 0) {
                p.life = 1.0f;
            }
            if(p.x < -1.0f || p.x > -1.0f) {
                p.x  = generateRandomFloat(-1.0f, 1.0f);
                p.angle = generateRandomFloat(0.0f, 1.0f);
            }
            if (p.y < -1.0f) {
                p.y = 1.0f; 
                p.x = generateRandomFloat(-1.0f, 1.0f);  
            }
            positions.push_back(p.x);
            positions.push_back(p.y);

            float size = 50.0f * p.life;
            sizes.push_back(size);

            float particleAlpha = p.life;
            colors.push_back(generateRandomFloat(0.0f, 1.0f));
            colors.push_back(generateRandomFloat(0.0f, 1.0f));
            colors.push_back(generateRandomFloat(0.0f, 1.0f));
            colors.push_back(particleAlpha);
        }


        glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, positions.size() * sizeof(float), positions.data());

        glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizes.size() * sizeof(float), sizes.data());

        glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, colors.size() * sizeof(float), colors.data());
        CHECK_GL_ERROR();
    }

private:
    mx::Font font;
    Uint32 lastUpdateTime = SDL_GetTicks();
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("-[Particle Effect]-", tw, th) {
        setPath(path);
        setObject(new Game());
        object->load(this);
    }
    
    ~MainWindow() override {}
    
    virtual void event(SDL_Event &e) override {
        object->event(this, e);
    }
    
    virtual void draw() override {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glViewport(0, 0, w, h);
        object->draw(this);
        swap();
        delay();
    }
};

MainWindow *main_w = nullptr;

void eventProc() {
    main_w->proc();
}

int main(int argc, char **argv) {
#ifdef __EMSCRIPTEN__
    MainWindow main_window("/", 1280, 720);
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
#endif
    return 0;
}