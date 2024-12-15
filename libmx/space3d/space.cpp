#include"mx.hpp"
#include"argz.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <GLES3/gl3.h>
#endif

#include<ctime>
#include<cstdlib>
#include<tuple>

#include"gl.hpp"
#include"loadpng.hpp"
#include"model.hpp"

#define CHECK_GL_ERROR() \
{ GLenum err = glGetError(); \
if (err != GL_NO_ERROR) \
printf("OpenGL Error: %d at %s:%d\n", err, __FILE__, __LINE__); }

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


class SpaceGame : public gl::GLObject {
public:
    gl::ShaderProgram shaderProgram, textShader, starsShader;
    mx::Font font;
    mx::Input stick;
    int score;
    GLuint starsVBO, starsVAO;
    const int numStars = 1000;
    glm::vec3 stars[1000];
    mx::Model ship;
    glm::vec3 ship_pos;
    float shipRotation = 0.0f;        
    float rotationSpeed = 90.0f;      
    float maxTiltAngle = 10.0f;       
    std::tuple<float, float, float, float> screenx;
    float barrelRollAngle = 0.0f;      
    bool isBarrelRolling = false;      
    float barrelRollSpeed = 360.0f; 

    SpaceGame(gl::GLWindow *win) : score{0}, ship_pos(0.0f, -20.0f, -60.0f) {

    }
    
    virtual ~SpaceGame() {
        glDeleteBuffers(1, &starsVBO);
        glDeleteVertexArrays(1, &starsVAO);
    }

    void load(gl::GLWindow *win) override {
        font.loadFont(win->util.getFilePath("data/font.ttf"), 24);
        if(!shaderProgram.loadProgram(win->util.getFilePath("data/tri.vert"), win->util.getFilePath("data/tri.frag"))) {
            throw mx::Exception("Could not load shader program tri");
        }        
        if(!textShader.loadProgram(win->util.getFilePath("data/text.vert"), win->util.getFilePath("data/text.frag"))) {
            throw mx::Exception("Could not load shader program text");
        }
        
        if(!starsShader.loadProgram(win->util.getFilePath("data/stars.vert"), win->util.getFilePath("data/stars.frag"))) {
            throw mx::Exception("Could not load shader program text");
        }

        if(!ship.openModel(win->util.getFilePath("data/objects/bird.mxmod"))) {
            throw mx::Exception("Could not open model bird.mxmod");
        }

        shaderProgram.useProgram();
        float aspectRatio = (float)win->w / (float)win->h;
        float fov = glm::radians(45.0f); 
        float z = -60.0f; 
        float height = 2.0f * glm::tan(fov / 2.0f) * std::abs(z);
        float width = height * aspectRatio;
        std::get<0>(screenx) = -width / 2.0f;
        std::get<1>(screenx) =  width / 2.0f;
        std::get<2>(screenx) = -height / 2.0f;
        std::get<3>(screenx) = height / 2.0f;
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);
        shaderProgram.setUniform("projection", projection);
        glm::mat4 view = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 5.0f),  
            glm::vec3(0.0f, 0.0f, 0.0f),  
            glm::vec3(0.0f, 1.0f, 0.0f)   
        );
        shaderProgram.setUniform("view", view);   
        ship.setShaderProgram(&shaderProgram, "texture1");
        ship.setTextures(win, win->util.getFilePath("data/objects/bird.tex"), win->util.getFilePath("data/objects"));
        
        starsShader.useProgram();
        starsShader.setUniform("starColor", glm::vec3(1.0f, 1.0f, 1.0f));
        glm::mat4 star_projection = glm::ortho(-100.0f, 100.0f, -75.0f, 75.0f, -100.0f, 100.0f);
        starsShader.setUniform("projection", star_projection);
        initStars();
        glGenVertexArrays(1, &starsVAO);
        glGenBuffers(1, &starsVBO);
        glBindVertexArray(starsVAO);
        glBindBuffer(GL_ARRAY_BUFFER, starsVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(stars), stars, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glEnable(GL_PROGRAM_POINT_SIZE);
    }
    
    Uint32 lastUpdateTime = SDL_GetTicks();
    
    void draw(gl::GLWindow *win) override {
        shaderProgram.useProgram();
        CHECK_GL_ERROR();
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; // Convert to seconds
        lastUpdateTime = currentTime;
        update(deltaTime);

      
        /// draw objects
        glDisable(GL_DEPTH_TEST);
        starsShader.useProgram();
        glBindVertexArray(starsVAO);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDrawArrays(GL_POINTS, 0, numStars);
        glBindVertexArray(0);

        glEnable(GL_DEPTH_TEST);
        shaderProgram.useProgram();
        glm::vec3 cameraPos(0.0f, 0.0f, 10.0f);
        glm::vec3 lightPos(0.0f, 3.0f, 2.0f); 
        glm::vec3 viewPos = cameraPos;
        glm::vec3 lightColor(1.5f, 1.5f, 1.5f); 

        shaderProgram.setUniform("lightPos", lightPos);
        shaderProgram.setUniform("viewPos", viewPos);
        shaderProgram.setUniform("lightColor", lightColor);
        glm::mat4 model = glm::translate(glm::mat4(1.0f), ship_pos);
        if (isBarrelRolling) {
            model = glm::rotate(model, glm::radians(barrelRollAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        }
        model = glm::rotate(model, glm::radians(shipRotation), glm::vec3(0.0f, 0.0f, 1.0f)); 
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));        
        shaderProgram.setUniform("model", model);
        ship.drawArrays();

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        textShader.useProgram();
        SDL_Color white = {255, 255, 255, 255};
        int textWidth = 0, textHeight = 0;
        GLuint textTexture = createTextTexture("Score: " + std::to_string(score), font.wrapper().unwrap(), white, textWidth, textHeight);
        renderText(textTexture, textWidth, textHeight, win->w, win->h);
        glDeleteTextures(1, &textTexture);
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        shaderProgram.useProgram();
    }
        
    void event(gl::GLWindow *win, SDL_Event &e) override {

        if(stick.connectEvent(e)) {
            mx::system_out << "Controller Event\n";
        }
    }

    Uint32 lastActionTime = SDL_GetTicks();
        
    void checkInput(float deltaTime) {
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - lastActionTime >= 15) {
            float moveX = 0.0f; 
            float moveY = 0.0f; 

               if (!isBarrelRolling && stick.getButton(mx::Input_Button::BTN_B)) {
                    isBarrelRolling = true;
                    barrelRollAngle = 0.0f;
                }

            if (stick.getButton(mx::Input_Button::BTN_D_LEFT)) {
                moveX -= 50.0f; 
                shipRotation = glm::clamp(shipRotation + rotationSpeed * deltaTime, -maxTiltAngle, maxTiltAngle);
            }
            if (stick.getButton(mx::Input_Button::BTN_D_RIGHT)) {
                moveX += 50.0f; 
                shipRotation = glm::clamp(shipRotation - rotationSpeed * deltaTime, -maxTiltAngle, maxTiltAngle);
            }
            if (stick.getButton(mx::Input_Button::BTN_D_UP)) {
                moveY += 50.0f; 
            }
            if (stick.getButton(mx::Input_Button::BTN_D_DOWN)) {
                moveY -= 50.0f; 
            }
            if (moveX != 0.0f || moveY != 0.0f) {
                float magnitude = glm::sqrt(moveX * moveX + moveY * moveY);
                moveX = (moveX / magnitude) * 50.0f * deltaTime;
                moveY = (moveY / magnitude) * 50.0f * deltaTime;
            }
            ship_pos.x += moveX;
            ship_pos.y += moveY;
            if (moveX == 0.0f) {
                if (shipRotation > 0.0f) {
                    shipRotation = glm::max(shipRotation - rotationSpeed * deltaTime, 0.0f);
                } else if (shipRotation < 0.0f) {
                    shipRotation = glm::min(shipRotation + rotationSpeed * deltaTime, 0.0f);
                }
            }
            ship_pos.x = glm::clamp(ship_pos.x, std::get<0>(screenx) + 2.0f, std::get<1>(screenx) - 2.0f);
            ship_pos.y = glm::clamp(ship_pos.y, std::get<2>(screenx) + 4.5f, std::get<3>(screenx) - 4.5f);
            lastActionTime = currentTime;
        }
    }


    void update(float deltaTime) {
        checkInput(deltaTime);
         if (isBarrelRolling) {
           barrelRollAngle += barrelRollSpeed * deltaTime;
            if (barrelRollAngle >= 360.0f) {
                barrelRollAngle = 0.0f;
                isBarrelRolling = false;
            }
        }
        
        for (int i = 0; i < numStars; ++i) {
            stars[i].y -= deltaTime * 10.0f; 
            if (stars[i].y < -75.0f) {
                stars[i] = glm::vec3(rand() % 200 - 100, 75.0f, 50.0f);
            }
        }
   
        glBindBuffer(GL_ARRAY_BUFFER, starsVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(stars), stars);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        CHECK_GL_ERROR();
    }
    
    GLuint createTextTexture(const std::string &text, TTF_Font *font, SDL_Color color, int &textWidth, int &textHeight) {
        SDL_Surface *surface = TTF_RenderText_Blended(font, text.c_str(), color);
        if (!surface) {
            mx::system_err << "Failed to create text surface: " << TTF_GetError() << std::endl;
            return 0;
        }
        
        surface = mx::Texture::flipSurface(surface);
        
        textWidth = surface->w;
        textHeight = surface->h;
        
        if (surface->format->format != SDL_PIXELFORMAT_RGBA32) {
            SDL_Surface *converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
            if (!converted) {
                mx::system_err << "Failed to convert surface format: " << SDL_GetError() << std::endl;
                SDL_FreeSurface(surface);
                return 0;
            }
            SDL_FreeSurface(surface);
            surface = converted;
        }
        
        GLuint texture;
        glGenTextures(1, &texture);
        CHECK_GL_ERROR();
        glBindTexture(GL_TEXTURE_2D, texture);
        CHECK_GL_ERROR();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        CHECK_GL_ERROR();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textWidth, textHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
        CHECK_GL_ERROR();
        SDL_FreeSurface(surface);
        return texture;
    }
    
    void renderText(GLuint texture, int textWidth, int textHeight, int screenWidth, int screenHeight) {
        float x = -0.5f * (float)textWidth / screenWidth * 2.0f;
        float y = 0.9f;
        float w = (float)textWidth / screenWidth * 2.0f;
        float h = (float)textHeight / screenHeight * 2.0f;
        
        GLfloat vertices[] = {
            x,     y,      0.0f, 0.0f, 1.0f,
            x + w, y,      0.0f, 1.0f, 1.0f,
            x,     y - h,  0.0f, 0.0f, 0.0f,
            x + w, y - h,  0.0f, 1.0f, 0.0f
        };
        
        GLuint VBO, VAO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        textShader.useProgram();
        textShader.setUniform("textTexture", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        
        glBindVertexArray(0);
        glDeleteBuffers(1, &VBO);
        glDeleteVertexArrays(1, &VAO);
    }

private:
    void initStars() {
        std::srand(std::time(0));
        for (int i = 0; i < numStars; ++i) {
            stars[i] = glm::vec3(rand() % 200 - 100, rand() % 150 - 75, rand() % 100 + 1.0f);
        }
    }
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("Space3D", tw, th) {
        setPath(path);
        setObject(new SpaceGame(this));
        object->load(this);
    }
    
    ~MainWindow() override {}
    
    virtual void event(SDL_Event &e) override {
        object->event(this, e);
    }
    
    virtual void draw() override {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glViewport(0, 0, w, h);
        object->draw(this);
        swap();
        delay();
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
