// This Demo was made with the help of AI
#include"mx.hpp"
#include"argz.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <GLES3/gl3.h>
#endif

#include"gl.hpp"
#include"loadpng.hpp"
#include <vector>

#define CHECK_GL_ERROR() \
{ GLenum err = glGetError(); \
if (err != GL_NO_ERROR) \
printf("OpenGL Error: %d at %s:%d\n", err, __FILE__, __LINE__); }

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct Point {
    float x, y;
    Point(float _x = 0.0f, float _y = 0.0f) : x(_x), y(_y) {}
};

class Game : public gl::GLObject {
public:
    GLuint highlightVAO = 0, highlightVBO = 0;
    
    Game() = default;
    virtual ~Game() override {
        glDeleteBuffers(1, &curveVBO);
        glDeleteVertexArrays(1, &curveVAO);
        glDeleteBuffers(1, &controlPointsVBO);
        glDeleteVertexArrays(1, &controlPointsVAO);
        glDeleteBuffers(1, &highlightVBO);       
        glDeleteVertexArrays(1, &highlightVAO);  
    }

    void load(gl::GLWindow *win) override {
        font.loadFont(win->util.getFilePath("data/font.ttf"), 20);
        controlPoints = {
            {100.0f, 400.0f},  
            {400.0f, 140.0f},  
            {700.0f, 400.0f}   
        };

        if (!shaderProgram.loadProgram(win->util.getFilePath("data/curve.vert"), 
                                      win->util.getFilePath("data/curve.frag"))) {
            throw mx::Exception("Failed to load shader program");
        }

        glGenVertexArrays(1, &curveVAO);
        glGenBuffers(1, &curveVBO);
        glGenVertexArrays(1, &controlPointsVAO);
        glGenBuffers(1, &controlPointsVBO);
        glGenVertexArrays(1, &highlightVAO);
        glGenBuffers(1, &highlightVBO);
        calculateCurvePoints();
        projectionMatrix = glm::ortho(0.0f, static_cast<float>(win->w), 
                                      static_cast<float>(win->h), 0.0f, -1.0f, 1.0f);
    }

    void draw(gl::GLWindow *win) override {
        glDisable(GL_DEPTH_TEST);
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; 
        lastUpdateTime = currentTime;
        
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        shaderProgram.useProgram();
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram.id(), "projection"), 
                         1, GL_FALSE, glm::value_ptr(projectionMatrix));
        
        glUniform4f(glGetUniformLocation(shaderProgram.id(), "color"), 0.0f, 1.0f, 0.0f, 1.0f);
        glBindVertexArray(curveVAO);
        glLineWidth(3.0f);
        glDrawArrays(GL_LINE_STRIP, 0, curvePoints.size());
        glBindVertexArray(controlPointsVAO);
        glUniform4f(glGetUniformLocation(shaderProgram.id(), "color"), 0.7f, 0.7f, 0.7f, 1.0f);
        glLineWidth(1.0f);
        glDrawArrays(GL_LINE_STRIP, 0, 3);
        glUniform4f(glGetUniformLocation(shaderProgram.id(), "color"), 1.0f, 0.0f, 0.0f, 1.0f);
        glPointSize(10.0f);
        glDrawArrays(GL_POINTS, 0, 3);
        glBindVertexArray(0);
        win->text.setColor({255, 255, 255, 255});
        win->text.printText_Solid(font, 20.0f, 20.0f, "Quadratic Bezier Curve Demo");
        win->text.printText_Solid(font, 20.0f, 50.0f, "Formula: B(t) = (1-t)^2*P0 + 2*(1-t)*t*P1 + t^2*P2");
        win->text.printText_Solid(font, 20.0f, 80.0f, "Click and drag the red control points to modify the curve");
        
        for (int i = 0; i < 3; i++) {
            std::string pointLabel = "P" + std::to_string(i) + " (" + 
                                   std::to_string(int(controlPoints[i].x)) + ", " + 
                                   std::to_string(int(controlPoints[i].y)) + ")";
            win->text.printText_Solid(font, controlPoints[i].x + 10, controlPoints[i].y - 10, pointLabel);
        }
        
        if (isAnimating) {
            tParam += deltaTime * 0.5f; 
            if (tParam > 1.0f) {
                tParam = 0.0f;
            }
            highlightPoint = calculateBezierPoint(tParam);
            std::string tValue = "t = " + std::to_string(tParam).substr(0, 4);
            win->text.printText_Solid(font, 20.0f, 110.0f, tValue);
            shaderProgram.useProgram();
            glUniform4f(glGetUniformLocation(shaderProgram.id(), "color"), 1.0f, 1.0f, 0.0f, 1.0f);
            glBindVertexArray(highlightVAO);
            glBindBuffer(GL_ARRAY_BUFFER, highlightVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(Point), &highlightPoint, GL_DYNAMIC_DRAW);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)0);
            glEnableVertexAttribArray(0);    
            glPointSize(8.0f);
            glDrawArrays(GL_POINTS, 0, 1);
            glBindVertexArray(0);
        }
    }
    
    void event(gl::GLWindow *win, SDL_Event &e) override {
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
            int mouseX = e.button.x;
            int mouseY = e.button.y;
            
            for (int i = 0; i < static_cast<int>(controlPoints.size()); i++) {
                float dx = mouseX - controlPoints[i].x;
                float dy = mouseY - controlPoints[i].y;
                float distSquared = dx*dx + dy*dy;
                
                if (distSquared < 100.0f) {  
                    selectedPointIndex = i;
                    isDragging = true;
                    break;
                }
            }
        }
        else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
            isDragging = false;
            selectedPointIndex = -1;
        }
        else if (e.type == SDL_MOUSEMOTION && isDragging && selectedPointIndex >= 0) {
            controlPoints[selectedPointIndex].x = e.motion.x;
            controlPoints[selectedPointIndex].y = e.motion.y;
            calculateCurvePoints();
        }
        else if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.sym == SDLK_SPACE) {
                isAnimating = !isAnimating;
                tParam = 0.0f;
            }
        }
    }

private:
    mx::Font font;
    Uint32 lastUpdateTime = SDL_GetTicks();
    gl::ShaderProgram shaderProgram;
    
    GLuint curveVAO = 0, curveVBO = 0;
    GLuint controlPointsVAO = 0, controlPointsVBO = 0;
    
    std::vector<Point> controlPoints;
    std::vector<Point> curvePoints;
    glm::mat4 projectionMatrix;
    
    bool isDragging = false;
    int selectedPointIndex = -1;
    
    bool isAnimating = false;
    float tParam = 0.0f;
    Point highlightPoint;
    Point calculateBezierPoint(float t) {
        float u = 1.0f - t;
        float tt = t * t;
        float uu = u * u;
        Point p;
        p.x = uu * controlPoints[0].x + 2.0f * u * t * controlPoints[1].x + tt * controlPoints[2].x;
        p.y = uu * controlPoints[0].y + 2.0f * u * t * controlPoints[1].y + tt * controlPoints[2].y;
        return p;
    }
    
    void calculateCurvePoints() {
        const int numPoints = 100;  
        curvePoints.clear();
        curvePoints.reserve(numPoints);
        
        for (int i = 0; i < numPoints; i++) {
            float t = static_cast<float>(i) / (numPoints - 1);
            curvePoints.push_back(calculateBezierPoint(t));
        }
        
        glBindVertexArray(curveVAO);
        glBindBuffer(GL_ARRAY_BUFFER, curveVBO);
        glBufferData(GL_ARRAY_BUFFER, curvePoints.size() * sizeof(Point), curvePoints.data(), GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)0);
        glEnableVertexAttribArray(0);
        
        glBindVertexArray(controlPointsVAO);
        glBindBuffer(GL_ARRAY_BUFFER, controlPointsVBO);
        glBufferData(GL_ARRAY_BUFFER, controlPoints.size() * sizeof(Point), controlPoints.data(), GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)0);
        glEnableVertexAttribArray(0);
        
        glBindVertexArray(0);
    }
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("Quadratic Bezier Curve", tw, th, false) {
        setPath(path);
        setObject(new Game());
        object->load(this);
    }
    
    ~MainWindow() override {}
    
    virtual void event(SDL_Event &e) override {
        if (object) {
            object->event(this, e);
        }
    }
    
    virtual void draw() override {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
    MainWindow main_window("/", 800, 600);
    main_w = &main_window;
    emscripten_set_main_loop(eventProc, 0, 1);
#else
    Arguments args = proc_args(argc, argv);
    try {
        MainWindow main_window(args.path, args.width, args.height);
        main_window.loop();
    } catch(const mx::Exception &e) {
        mx::system_err << "mx: Exception: " << e.text() << "\n";
        mx::system_err.flush();
        exit(EXIT_FAILURE);
    }
#endif
    return 0;
}
