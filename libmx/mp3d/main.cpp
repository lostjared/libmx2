#include "mx.hpp"
#include "gl.hpp"
#include "model.hpp"
#include "quadtris.hpp"
#include "intro.hpp"
#include "start.hpp"

class Game : public gl::GLObject {
public:
    Game(int diff) : mp(diff) {
        mp.setCallback([this]() {
            mp.procBlocks();
        });
    }

    ~Game() override {
        for (auto &t : textures)  {
            glDeleteTextures(1, &t);
        }
    }

    virtual void load(gl::GLWindow *win) override {
        mp.newGame();
        if(cube.openModel(win->util.getFilePath("data/cube.mxmod")) == false) {
            throw mx::Exception("Failed to open model");
        }
        if(program.loadProgram(win->util.getFilePath("data/tri.vert"), win->util.getFilePath("data/tri.frag")) == false) {
            throw mx::Exception("Failed to load shader program");
        }
        const char *texture_files[] = {"data/block_clear.png","data/block_dblue.png","data/block_gray.png","data/block_green.png","data/block_ltblue.png", "data/block_purple.png", 0 };
        for(int i = 0; texture_files[i] != 0; ++i) {
            GLuint tex = gl::loadTexture(win->util.getFilePath(texture_files[i]));
            if(tex == 0) {
                throw mx::Exception("Failed to load texture: " + std::string(texture_files[i]));
            }
            textures.push_back(tex);
        }
        background.loadProgramFromText(gl::vSource, gl::fSource);
        bg.initSize(win->w, win->h);
        bg.loadTexture(&background, win->util.getFilePath("data/logo.png"), 0.0f, 0.0f, win->w, win->h);
        resize(win, win->w, win->h);
    }

    virtual void draw(gl::GLWindow *win) override {

        glDisable(GL_DEPTH_TEST);
        background.useProgram();
        bg.initSize(win->w, win->h);                                                      
        bg.draw();

        glEnable(GL_DEPTH_TEST);
        program.useProgram();
        glm::vec3 cameraPos(0.0f, 0.0f,10.0f);
        glm::vec3 lightPos(0.0f, 3.0f, 2.0f); 
        glm::vec3 viewPos = cameraPos;
        glm::vec3 lightColor(1.2f, 1.2f, 1.2f); 
        program.setUniform("lightPos", lightPos);
        program.setUniform("viewPos", viewPos);
        program.setUniform("lightColor", lightColor);
    
        for(int i = 0; i < mp.grid.width(); ++i) {
            for(int z = 0; z < mp.grid.height(); ++z) {
                glActiveTexture(GL_TEXTURE0);
                program.setName("texture1");  
                puzzle::Block *b = mp.grid.at(i, z);
                if(b != nullptr && b->color != 0)     
                    drawGridXY(i, z, b->color);   
            }
        }

        drawGridXY(mp.grid.game_piece.getX(), mp.grid.game_piece.getY(), mp.grid.game_piece.at(0)->color);
        drawGridXY(mp.grid.game_piece.getX(), mp.grid.game_piece.getY()+1, mp.grid.game_piece.at(1)->color);
        drawGridXY(mp.grid.game_piece.getX(), mp.grid.game_piece.getY()+2, mp.grid.game_piece.at(2)->color);

    }

    void drawGridXY(int x, int y, int color) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 10.0f, -25.0f));
        model = glm::translate(model, glm::vec3(-10.0f, 0.0f, 0.0f));
        model = glm::translate(model, glm::vec3(x * 1.2f, -y * 1.2f, 0.0f)); 
        program.setUniform("model", model);
        glActiveTexture(GL_TEXTURE0);
        program.setName("texture1");          
        glm::mat4 rotated = glm::rotate(glm::mat4(1.0f), (float)SDL_GetTicks() / 1000.0f, glm::vec3(0.0f, 1.0f, 0.0f));
        program.setUniform("model", model * rotated);
        cube.drawArraysWithTexture(textures[color], "texture1");
    }
    
    virtual void event(gl::GLWindow *win, SDL_Event &e) override {
        switch(e.type) {
            case SDL_KEYDOWN:
            switch(e.key.keysym.sym) {
                case SDLK_LEFT:
                    mp.grid.game_piece.moveLeft();
                break;
                case SDLK_RIGHT:
                    mp.grid.game_piece.moveRight();  
                break;
                case SDLK_UP:
                    mp.grid.game_piece.shiftColors();
                break;
                case SDLK_DOWN:
                    mp.grid.game_piece.moveDown();
                break;
            }
            break;
        }
    }

    virtual void resize(gl::GLWindow *win, int w, int h) override {
        program.useProgram();
        cube.setShaderProgram(&program);
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = glm::lookAt(
             glm::vec3(0.0f, 0.0f, 5.0f),  // Camera position
             glm::vec3(0.0f, 0.0f, 0.0f),  // Look at origin
             glm::vec3(0.0f, 1.0f, 0.0f)   // Up vector
         );
         glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)win->w / win->h, 0.1f, 100.0f);
         program.setUniform("model", model);
         program.setUniform("view", view);
         program.setUniform("projection", projection); 
    }
private:
    gl::ShaderProgram program, background;
    mx::Model cube;
    std::vector<GLuint> textures;
    puzzle::MasterPiece mp;
    gl::GLSprite bg;
};

void Intro::draw(gl::GLWindow *win) {
    glDisable(GL_DEPTH_TEST);
#ifndef __EMSCRIPTEN__
    Uint32 currentTime = SDL_GetTicks();
#else
    double currentTime = emscripten_get_now();
#endif
    program.useProgram();
    program.setUniform("alpha", fade);
    intro.draw();
    if((currentTime - lastUpdateTime) > 25) {
        lastUpdateTime = currentTime;
        fade += 0.01;
    }
    if(fade >= 1.0) {
        win->setObject(new Start());
        win->object->load(win);
        return;
    }
}
void Start::event(gl::GLWindow *win, SDL_Event &e) {
    if(e.type == SDL_KEYDOWN || e.type == SDL_FINGERDOWN || e.type == SDL_MOUSEBUTTONDOWN) {
        win->setObject(new Game(0));
        win->object->load(win);
        return;
    }
}

class MainWindow : public gl::GLWindow {
public:
    MainWindow(const std::string &path) : gl::GLWindow("mp3d", 1280, 720) {
        setPath(path);
        setObject(new Intro());
        object->load(this);
    }

    virtual void draw() override {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        object->draw(this);
        swap();
        delay();
    }

    virtual void event(SDL_Event &e) override {
    //    Game *g = dynamic_cast<Game *>(object.get());
    }
};

int main(int argc, char **argv) {
    #if defined(_WIN32) || defined(__linux__) || defined(__APPLE__)
    if(argc == 2) {
        try {
            MainWindow main_window(argv[1]);
            main_window.loop();
        }  catch(mx::Exception &e) {
            SDL_Log("mx: Exception: %s", e.text().c_str());
        }
    }
    #elif defined(__ANDROID__)
        try {
            MainWindow main_window("");
            main_window.loop();
        }  catch(mx::Exception &e) {
            SDL_Log("mx: Exception: %s", e.text().c_str());
        }
    #endif
    return 0;
}