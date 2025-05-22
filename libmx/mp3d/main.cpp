#include"mx.hpp"
#include"gl.hpp"
#include"model.hpp"
#include"quadtris.hpp"
#include"intro.hpp"
#include"start.hpp"
#include"gameover.hpp"
#include<cstdlib>
#include<ctime>
#include<filesystem>
#include<atomic>
#if defined(__APPLE__) || defined(_WIN32) || defined(__linux__)
#include"argz.hpp"
#endif

#include<mxcmd/ast.hpp>
#include<mxcmd/scanner.hpp>
#include<mxcmd/parser.hpp>

#if defined(__EMSCRIPTEN__) || defined(__ANDORID__)
    const char *m_vSource = R"(#version 300 es
            precision highp float; 
            layout (location = 0) in vec3 aPos;
            layout (location = 1) in vec2 aTexCoord;

            out vec2 TexCoord;

            void main() {
                gl_Position = vec4(aPos, 1.0); 
                TexCoord = aTexCoord;         
            }
    )";
    const char *m_fSource = R"(#version 300 es
        precision highp float; 
        in vec2 TexCoord;
        out vec4 FragColor;
        uniform sampler2D textTexture; 
        uniform float alpha;
        void main() {
            vec4 fcolor = texture(textTexture, TexCoord);
            FragColor = fcolor * alpha;
        }
    )";

    const char *v_mainShader = R"(#version 300 es
        precision mediump float; 
        layout(location = 0) in vec3 position;   
        layout(location = 1) in vec3 normal;     
        layout(location = 2) in vec2 texCoord;   

        out vec3 fragPos;       
        out vec3 fragNormal;    
        out vec2 fragTexCoord;  

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        void main() {
            fragPos = vec3(model * vec4(position, 1.0));              
            fragNormal = mat3(transpose(inverse(model))) * normal;    
            fragTexCoord = texCoord;                                 

            gl_Position = projection * view * vec4(fragPos, 1.0);    
        }
    )";
    const char *f_mainShader = R"(#version 300 es
        precision highp float;  

        in vec3 fragPos;       
        in vec3 fragNormal;    
        in vec2 fragTexCoord;  
        out vec4 fragColor;

        uniform sampler2D texture1; 
        uniform vec3 lightPos;
        uniform vec3 viewPos;
        uniform vec3 lightColor;

        void main() {
            float ambientStrength = 0.15;
            vec3 ambient = ambientStrength * lightColor;
            vec3 norm = normalize(fragNormal);
            vec3 lightDir = normalize(lightPos - fragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * lightColor;
            float specularStrength = 0.5;
            vec3 viewDir = normalize(viewPos - fragPos);
            vec3 reflectDir = reflect(-lightDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0); 
            vec3 specular = specularStrength * spec * lightColor;
            vec3 textureColor = texture(texture1, fragTexCoord).rgb; 
            vec3 result = (ambient + diffuse + specular) * textureColor;
            fragColor = vec4(result, 1.0); 
            fragColor.a = 0.8;
        }
)";

#else
    const char *v_mainShader = R"(#version 330 core
        layout(location = 0) in vec3 position;   
        layout(location = 1) in vec3 normal;     
        layout(location = 2) in vec2 texCoord;   

        out vec3 fragPos;       
        out vec3 fragNormal;    
        out vec2 fragTexCoord;  

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        void main() {
            fragPos = vec3(model * vec4(position, 1.0));              
            fragNormal = mat3(transpose(inverse(model))) * normal;    
            fragTexCoord = texCoord;                                 

            gl_Position = projection * view * vec4(fragPos, 1.0);    
        }
    )";
    
    const char *f_mainShader = R"(#version 330 core
        in vec3 fragPos;       
        in vec3 fragNormal;    
        in vec2 fragTexCoord;  
        out vec4 fragColor;

        uniform sampler2D texture1; 
        uniform vec3 lightPos;
        uniform vec3 viewPos;
        uniform vec3 lightColor;

        void main() {
            float ambientStrength = 0.15;
            vec3 ambient = ambientStrength * lightColor;
            vec3 norm = normalize(fragNormal);
            vec3 lightDir = normalize(lightPos - fragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * lightColor;
            float specularStrength = 0.5;
            vec3 viewDir = normalize(viewPos - fragPos);
            vec3 reflectDir = reflect(-lightDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0); 
            vec3 specular = specularStrength * spec * lightColor;
            vec3 textureColor = texture(texture1, fragTexCoord).rgb; 
            vec3 result = (ambient + diffuse + specular) * textureColor;
            fragColor = vec4(result, 1.0); 
            fragColor.a = 0.8;
        }
    )";
    

    const char *m_vSource = R"(#version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec2 aTexCoord;
        out vec2 TexCoord;
        void main() {
            gl_Position = vec4(aPos, 1.0); 
            TexCoord = aTexCoord;        
        }
    )";
    const char *m_fSource = R"(#version 330 core
        in vec2 TexCoord;
        out vec4 FragColor;
        uniform sampler2D textTexture; 
        uniform float alpha;
        void main() {
            vec4 fcolor = texture(textTexture, TexCoord);
            FragColor = fcolor * alpha; 
        }
    )";
#endif

class Game : public gl::GLObject {
    cmd::AstExecutor &executor = cmd::AstExecutor::getExecutor();
    std::function<int(const std::vector<std::string> &args, std::istream &input, std::ostream &output)> new_game = nullptr;
    std::function<int(const std::vector<std::string> &args, std::istream &input, std::ostream &output)> drop_game = nullptr;
    std::function<int(const std::vector<std::string> &args, std::istream &input, std::ostream &output)> toggle_console = nullptr;
    std::function<int(const std::vector<std::string> &args, std::istream &input, std::ostream &output)> left_game = nullptr;
    std::function<int(const std::vector<std::string> &args, std::istream &input, std::ostream &output)> right_game = nullptr;
    std::function<int(const std::vector<std::string> &args, std::istream &input, std::ostream &output)> down_game= nullptr;
    std::function<int(const std::vector<std::string> &args, std::istream &input, std::ostream &output)> shift_game = nullptr;
    std::function<int(const std::vector<std::string> &args, std::istream &input, std::ostream &output)> rotate_game= nullptr;
    std::function<int(const std::vector<std::string> &args, std::istream &input, std::ostream &output)> settimeout_game= nullptr;
    std::atomic<bool> running{false};
public:

    bool cmd_echo = true;
 
    Game(int diff) : mp(diff) {
        mp.grid.game_piece.setCallback([]() {});
    }
    ~Game() override {
        for (auto &t : textures)  {
            glDeleteTextures(1, &t);
        }
    }

    gl::GLWindow *window_handle = nullptr;
    std::atomic<bool> interrupt_command{false};

    virtual void load(gl::GLWindow *win) override {
        if (win == nullptr) {
            throw mx::Exception("Window is null");
        }
        window_handle = win;
        fade_in = true;
        mp.newGame();
        if(cube.openModel(win->util.getFilePath("data/cube.mxmod")) == false) {
            throw mx::Exception("Failed to open model");
        }
        if(program.loadProgramFromText(v_mainShader, f_mainShader) == false) {
            throw mx::Exception("Failed to load shader program");
        }
        const char *texture_files[] = {"data/block_clear.png","data/punk.png","data/block_ltblue.png","data/block_yellow.png","data/block_purple.png", "data/block_green.png", 0 };
        for(int i = 0; texture_files[i] != 0; ++i) {
            GLuint tex = gl::loadTexture(win->util.getFilePath(texture_files[i]));
            if(tex == 0) {
                throw mx::Exception("Failed to load texture: " + std::string(texture_files[i]));
            }
            textures.push_back(tex);
        }
        background.loadProgramFromText(m_vSource,m_fSource);
        bg.initSize(win->w, win->h);
        bg.loadTexture(&background, win->util.getFilePath("data/bg.png"), 0.0f, 0.0f, win->w, win->h);
        font.loadFont(win->util.getFilePath("data/font.ttf"), 24);
        resize(win, win->w, win->h);
        mouse_x = mouse_y = 0;
      
        
        new_game = [win,this](const std::vector<std::string> &args, std::istream &in, std::ostream &output) -> int {
            mp.newGame();
            win->console.thread_safe_print("Piece Drop\n");
            win->console.process_message_queue();
            return 0;
        };
        left_game = [win, this](const std::vector<std::string> &args, std::istream &in, std::ostream &output) -> int {
            mp.grid.game_piece.moveLeft();
            win->console.thread_safe_print("Piece: Move Left\n");
            win->console.process_message_queue();
            return 0;
        };

        right_game = [win, this](const std::vector<std::string> &args, std::istream &in, std::ostream &output) -> int {
            mp.grid.game_piece.moveRight();
            win->console.thread_safe_print("Piece: Move Right\n");
            win->console.process_message_queue();
            return 0;
        };

        down_game = [win, this](const std::vector<std::string> &args, std::istream &in, std::ostream &output) -> int {
            mp.grid.game_piece.moveDown();
            win->console.thread_safe_print("Piece: Move Down\n");
            win->console.process_message_queue();
            return 0;
        };

        rotate_game = [win, this](const std::vector<std::string> &args, std::istream &in, std::ostream &output) -> int {
            mp.grid.game_piece.shiftDirection();
            win->console.thread_safe_print("Piece: Rotated\n");
            win->console.process_message_queue();
            return 0;
        };

        shift_game = [win, this](const std::vector<std::string> &args, std::istream &in, std::ostream &output) -> int {
            mp.grid.game_piece.shiftColors();
            win->console.thread_safe_print("Piece: Shifted\n");
            win->console.process_message_queue();
            return 0;
        };

        settimeout_game = [win, this](const std::vector<std::string> &args, std::istream &in, std::ostream &output) -> int {
            if(!args.empty() && args.size() == 1) {
                int value = std::stoi(args[0]);
                mp.timeout = value;
                win->console.thread_safe_print("Game Timeout Set to: " + std::to_string(value) + "\n");
                win->console.process_message_queue();
                return 0;
            } else {
                win->console.thread_safe_print("timeout: requires one argument the timeout\n");
                win->console.process_message_queue();
                return 1;
            }
            return 1;
        };

        toggle_console = [win, this](const std::vector<std::string> &args, std::istream &in, std::ostream &output) -> int {
            win->console_visible = false;
            win->console.hide();
            return 0;
        };

        drop_game = [win, this](const std::vector<std::string> &args, std::istream &in, std::ostream &output) -> int {
            mp.grid.game_piece.drop();
            win->console.thread_safe_print("Piece Drop\n");
            win->console.process_message_queue();
            return 0;
        };
        
        executor.addCommand("newgame", new_game);
        executor.addCommand("drop", drop_game);
        executor.addCommand("toggle", toggle_console);
        executor.addCommand("left",left_game);
        executor.addCommand("right", right_game);
        executor.addCommand("down", down_game);
        executor.addCommand("rotate", rotate_game);
        executor.addCommand("shift", shift_game);
        executor.addCommand("timeout", settimeout_game);
        win->console.setPrompt("$> ");
        win->console.setInputCallback([this, win](gl::GLWindow *window, const std::string &text) -> int {
            try {
                if(text == "@echo_on") {
                    cmd_echo = true;
                    window->console.thread_safe_print("Echoing commands on.\n");
                    window->console.process_message_queue();
                    return 0;
                } else if(text == "@echo_off") {
                    cmd_echo = false;
                    window->console.thread_safe_print("Echoing commands off.\n");
                    window->console.process_message_queue();
                    return 0;
                } 
                if(cmd_echo) {
                    window->console.thread_safe_print("$ " + text + "\n");
                }
                std::thread([this, text, window]() {
                    try {
                        std::cout << "Executing: " << text << std::endl;
                        std::string lineBuf;
                        executor.setInterrupt(&interrupt_command);
                        executor.setUpdateCallback(
                            [&lineBuf,window,this](const std::string &chunk) {
                                lineBuf += chunk;
                                size_t nl;
                                while ((nl = lineBuf.find('\n')) != std::string::npos) {
                                    std::string oneLine = lineBuf.substr(0, nl+1);
                                    lineBuf.erase(0, nl+1);
                                    window->console.thread_safe_print(oneLine);
                                    window->console.process_message_queue();             
                                }
                                if(interrupt_command) {
                                    throw cmd::Exit_Exception(100);
                                }
                            }
                        );
                        std::stringstream input_stream(text);
                        scan::TString string_buffer(text);
                        scan::Scanner scanner(string_buffer);
                        cmd::Parser parser(scanner);
                        auto ast = parser.parse();
                        std::stringstream output_stream;
                        running = true;
                        executor.execute(input_stream, output_stream, ast);
                        running = false;
                        if(!lineBuf.empty()) {
                            window->console.thread_safe_print(lineBuf);
                            window->console.process_message_queue();
                        }
                        if(!output_stream.str().empty()) {
                            window->console.thread_safe_print(output_stream.str());
                            window->console.process_message_queue();
                        }
                    } catch(const scan::ScanExcept &e) {
                        window->console.thread_safe_print("Scanner Exception: " + e.why() + "\n");
                        window->console.process_message_queue();
                    } catch(const cmd::Exit_Exception &e) {
                           if(e.getCode() == 100) {
                            window->console.thread_safe_print("Execution interrupted\n");
                        } else {
                            window->console.thread_safe_print("Execution exited with code " + std::to_string(e.getCode()) + "\n");
                            window->quit();
                            return;
                        }
                        window->console.process_message_queue();
                        interrupt_command = false;
                    } catch(const std::runtime_error &e) {
                        window->console.thread_safe_print("Runtime Exception: " + std::string(e.what()) + "\n");
                        window->console.process_message_queue();
                    } catch(const std::exception &e) {
                        window->console.thread_safe_print("Exception: " + std::string(e.what()) + "\n");
                        window->console.process_message_queue();
                    } catch (const state::StateException &e) {
                        window->console.thread_safe_print("State Exception: " + std::string(e.what()) + "\n");
                        window->console.process_message_queue();
                    } catch(const cmd::AstFailure  &e) {
                        window->console.thread_safe_print("Failure: " + std::string(e.what()) + "\n");
                        window->console.process_message_queue();
                    } catch(...) {
                        window->console.thread_safe_print("Unknown Error: Command execution failed\n");
                        window->console.process_message_queue();
                    }
                }).detach();
                
                return 0;
            } catch(const std::exception &e) {
                win->console.thread_safe_print("Error: " + std::string(e.what()) + "\n");
                window->console.process_message_queue();
                return 1;
            }
        });      
    }
    virtual void draw(gl::GLWindow *win) override {
        Uint32 currentTime = SDL_GetTicks();
        deltaTime = (currentTime - lastUpdateTime) / 1000.0f;        
        if((currentTime - lastUpdateTime) > 25) {
            lastUpdateTime = currentTime;
            mp.grid.spin();
            mp.procBlocks();
            if(fade_in == true && fade <= 0.5f) fade += 0.05;
            else fade_in = false;
        }
        static Uint32 previous_time = SDL_GetTicks();
        Uint32 current_time = SDL_GetTicks();
        if (current_time - previous_time >= mp.timeout) {
            if(fade_in == false) {
                if(win->console_visible == false) {
                    if(mp.grid.canMoveDown()) {
                        if(mp.drop == false) {
                            mp.grid.game_piece.moveDown();
                        }
                        previous_time = current_time;
                    } else if(mp.drop == false) {
                        // Game over
                        win->setObject(new GameOver(mp.score, mp.level));
                        win->object->load(win);
                        return;
                    }
                }
            }
        }
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND); 
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        background.useProgram();
        background.setUniform("alpha", fade); 
        bg.initSize(win->w, win->h);
        bg.draw();
        glDisable(GL_BLEND); 
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
                if(b != nullptr && b->color > 0) {
                    drawGridXY(i, z, b->color, 0);   
                } else if(b->color == -1) {
                    drawGridXY(i, z, 1+(rand()%3), b->rotate_x);
                }
            }
        }
        if(fade_in == false && mp.drop == false) {
            int x_val = mp.grid.game_piece.getX();
            int y_val = mp.grid.game_piece.getY();  
            for(int q = 0; q < 3; ++q) {
                int cx =  x_val;
                int cy =  y_val;
                switch(mp.grid.game_piece.getDirection()) {
                    case 0:
                    cx = x_val;
                    cy = y_val + q;
                    break;
                    case 1:
                    cx = x_val + q;
                    cy = y_val;
                    break;
                    case 2:
                    cx = x_val - q;
                    cy = y_val;
                    break;
                    case 3:
                    cx = x_val;
                    cy = y_val - q;
                    break;
                }
                puzzle::Block *b = mp.grid.game_piece.at(q);
                if (b != nullptr) {
                    drawGridXY(cx, cy, b->color, 0.0f);
                }
            }
        }
        if(fade_in == false)  {
            win->text.setColor({255, 255, 255, 255});
            win->text.printText_Solid(font, 25.0f, 25.0f, "Score: " + std::to_string(mp.score));
            win->text.setColor({255, 0, 0, 255});
            win->text.printText_Solid(font, 25.0f, 60.0f, "Level: " + std::to_string(mp.level));
        }
    }
    void drawGridXY(int x, int y, int color, float rotate_) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(rotateX), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotateY), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotateZ), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::translate(model, glm::vec3(0.0f, 0.0f,zoom ));
        float gridWidthOffset = -(mp.grid.width() * 1.2f) / 2.0f;
        model = glm::translate(model, glm::vec3(gridWidthOffset, 0.0f, 0.0f));
        model = glm::translate(model, glm::vec3(x * 1.2f, -y * 1.2f, 0.0f)); 
        program.setUniform("model", model);
        glActiveTexture(GL_TEXTURE0);
        program.setName("texture1"); 
        if(rotate_ > 0) {         
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
            glm::mat4 rotated = glm::rotate(glm::mat4(1.0f), rotate_ * deltaTime, glm::vec3(0.0f, 1.0, 0.0f));
            program.setUniform("model", model * rotated);
        } else if(color > 0){
            program.setUniform("model", model);
        }
        cube.drawArraysWithTexture(textures[color], "texture1");
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }
    void dropPiece() {
        if(mp.drop == false) {
            mp.drop = true;
            mp.grid.game_piece.drop();
        }
    }
    virtual void event(gl::GLWindow *win, SDL_Event &e) override {
        if(win->console_visible) {    
            if(e.type == SDL_KEYDOWN) {
                if(e.key.keysym.sym == SDLK_c && (e.key.keysym.mod & KMOD_CTRL)) {
                    if(running) {
                        win->console.thread_safe_print("\nCTRL+C Interrupt - Command interrupted\n");
                        interrupt_command = true;
                    } else {
                        win->console.thread_safe_print("\nCTRL+C Interrupt - No Command Running\n");
                    }
                    win->console.process_message_queue();  
                    return;          
                }
            }
            return;
        }
        
            switch(e.type) {
            case SDL_KEYUP:
                if(!win->console_visible && e.key.keysym.sym == SDLK_RETURN) {
                    dropPiece();
                    win->console.thread_safe_print("\nCTRL+C Interrupt - No Command Running\n");
                    win->console.process_message_queue();  
                }
            break;
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
                        win->console.print("Shift Colors\n");
                    break;
                    case SDLK_DOWN:
                        mp.grid.game_piece.moveDown();
                    break;
                    case SDLK_SPACE:
                        mp.grid.game_piece.shiftDirection();
                        win->console.print("Shift Direction\n");
                    break;
                    case SDLK_w:
                        rotateX += 0.5f;
                        win->console.printf("RotateX: %f ", rotateX);
                    break;
                    case SDLK_s:
                        rotateX -= 0.5;
                        win->console.printf("RotateX: %f ", rotateX);
                    break;
                    case SDLK_a:
                        rotateY -= 0.5f;
                        win->console.printf("RotateY: %f ", rotateY);
                    break;
                    case SDLK_d:
                        rotateY += 0.5f;
                        win->console.printf("RotateY: %f ", rotateY);
                    break;
                    case SDLK_z:
                        rotateZ -= 0.5f;
                        win->console.printf("RotateZ: %f ", rotateZ);
                    break;
                    case SDLK_x:
                        rotateZ +=  0.5f;
                        win->console.printf("RotateZ: %f ", rotateZ);
                    break;
                    case SDLK_EQUALS:
                        zoom += 0.5f;
                        win->console.printf("Zoom: %f ",zoom);
                    break;
                    case SDLK_MINUS:
                        zoom -= 0.5f;
                        win->console.printf("Zoom: %f ",zoom);
                    break;
                    case SDLK_k:
                    //std::cout << "ZOOM: " << zoom << " X,Y,Z" << rotateX << "," << rotateY << "," << rotateZ << std::endl;
                    break;
                }
                break;
                case SDL_MOUSEBUTTONDOWN:
                case SDL_FINGERDOWN:
                {
                    int x, y;
                    if (e.type == SDL_MOUSEBUTTONDOWN) {
                        x = e.button.x;
                        y = e.button.y;
                        mouse_down = true;
                        mouse_x = x;
                        mouse_y = y;
                        mouse_click_time = SDL_GetTicks();
                        double_click = false;
                        if (SDL_GetTicks() - last_click_time < double_click_interval) {
                            double_click = true;
                        }
                        last_click_time = SDL_GetTicks();
                    } else {
                        x = e.tfinger.x * win->w;
                        y = e.tfinger.y * win->h;
                        num_fingers++;
                        if (num_fingers == 1) {
                            mouse_down = true;
                            mouse_x = x;
                            mouse_y = y;
                        } else if (num_fingers == 2) {
                            dropPiece();
                        }
                    }
                    break;
                }
                case SDL_MOUSEBUTTONUP:
                case SDL_FINGERUP:
                {
                    if (e.type == SDL_MOUSEBUTTONUP) {
                        mouse_down = false;
                        int dy = e.button.y - mouse_y;
                        if (dy < -50) {
                            mp.grid.game_piece.shiftColors();
                        } else if (dy > 50) {
                            mp.grid.game_piece.shiftDirection();
                        } else if (double_click == true) {
                            dropPiece();
                            double_click = false;
                        }
                    } else {
                        num_fingers--;
                        if (num_fingers <= 0) {
                            mouse_down = false;
                            num_fingers = 0;
                        }
                        int y = e.tfinger.y * win->h;
                        int dy = y - mouse_y;
                        if (dy < -50) {
                            mp.grid.game_piece.shiftColors();
                        } else if (dy > 50) {
                            mp.grid.game_piece.shiftDirection();
                        }
                    }
                    break;
                }
                case SDL_MOUSEMOTION:
                case SDL_FINGERMOTION:
                {
                    int x;
                    if (e.type == SDL_MOUSEMOTION) {
                        x = e.motion.x;
                    } else {
                        x = e.tfinger.x * win->w;
                    }
                    if (mouse_down) {
                        int dx = x - mouse_x;
                        if (dx > 25) {
                            mp.grid.game_piece.moveRight();
                            mouse_x = x;
                        } else if (dx < -25) {
                            mp.grid.game_piece.moveLeft();
                            mouse_x = x;
                        }
                    }
                    break;
            }
        }
     }

    virtual void resize(gl::GLWindow *win, int w, int h) override {
        program.useProgram();
        cube.setShaderProgram(&program);
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = glm::lookAt(
             glm::vec3(0.0f, 3.0f, 5.0f),  
             glm::vec3(0.0f, 0.0f, 0.0f),  
             glm::vec3(0.0f, 1.0f, 0.0f)   
         );
         glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)win->w / win->h, 0.1f, 1000.0f);
         program.setUniform("model", model);
         program.setUniform("view", view);
         program.setUniform("projection", projection); 
    }
private:
    gl::ShaderProgram program, background, grid;
    mx::Model cube;
    std::vector<GLuint> textures;
    puzzle::MasterPiece mp;
    gl::GLSprite bg;
    gl::GLSprite grid_bg;
    mx::Font font;
    Uint32 lastUpdateTime = 0;
    float rotateX = 0.0f;
    float rotateY = 0.0f;
    float rotateZ = -0.5f;
    float zoom = -14.0f;
    bool mouse_down = false;
    int mouse_x = 0;
    int mouse_y = 0;
    Uint32 mouse_click_time = 0;
    Uint32 last_click_time = 0;
    bool double_click = false;
    Uint32 double_click_interval = 250;
    int num_fingers = 0;
    bool fade_in = false;
    float fade = 0.0f;
    float deltaTime = 0.0f;
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
    program.setUniform("time_f", SDL_GetTicks() / 1000.0f);
    intro.draw();
    if((currentTime - lastUpdateTime) > 25) {
        lastUpdateTime = currentTime;
        fade -= .01;
    }
    if(fade <= 0.1) {
        win->setObject(new Start());
        win->object->load(win);
        return;
    }
}

void Start::setGame(gl::GLWindow *win) {
    win->setObject(new Game(0));
    win->object->load(win);
}

void Start::load_shader() {
    if(!program.loadProgramFromText(m_vSource, m_fSource)) {
        throw mx::Exception("Failed to load shader program Intro::load_shader()");
    }
}

void Start::event(gl::GLWindow *win, SDL_Event &e) {
    if(fade_in == true && (e.type == SDL_KEYDOWN || e.type == SDL_FINGERUP || e.type == SDL_MOUSEBUTTONUP)) {
        fade = 1.0f;
        fade_in = false;
        return;
    }
}

void Intro::event(gl::GLWindow *win, SDL_Event &e) {
    if(e.type == SDL_KEYDOWN || e.type == SDL_FINGERUP || e.type == SDL_MOUSEBUTTONUP) {
        win->setObject(new Start());
        win->object->load(win);
        return;
    }
}

void GameOver::draw(gl::GLWindow *win) {
    glDisable(GL_DEPTH_TEST);
    program.useProgram();
    GLint loc = glGetUniformLocation(program.id(), "iResolution");
    glUniform2f(loc, win->w, win->h);
    program.setUniform("time_f", static_cast<float>(SDL_GetTicks()) / 1000.0f);
    game_over.draw();
    win->text.setColor({255, 255, 255, 255});
    int tw = 0, th = 0;
    TTF_Font *f = font.unwrap();
    std::string gameover_text = "Game Over Your Score: " + std::to_string(score_);
    std::string play_again = "Tap or Enter to play again";
    TTF_SizeText(f, gameover_text.c_str(), &tw, &th);
    float centerX = static_cast<float>(win->w - tw) / 2.0f;
    float centerY = static_cast<float>(win->h - th) / 2.0f;
    win->text.setColor({150, 80, 255, 255});
    win->text.printText_Solid(font, centerX, centerY, gameover_text.c_str());
    TTF_SizeText(f, play_again.c_str(), &tw, &th);
    centerX = static_cast<float>(win->w - tw) / 2.0f;
    centerY = static_cast<float>(win->h - th) / 2.0f;
    win->text.setColor({255, 255, 255, 255});
    win->text.printText_Solid(font, centerX, centerY + 80.0f, play_again.c_str());
}

void GameOver::event(gl::GLWindow *win, SDL_Event &e) {
    if((e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_RETURN) || (e.type == SDL_FINGERUP) || (e.type == SDL_MOUSEBUTTONUP)) {
        win->setObject(new Intro());
        win->object->load(win);
        return;
    }
}

class MainWindow : public gl::GLWindow {
public:
    MainWindow(const std::string &path, int wx, int wy) : gl::GLWindow("MasterPiece3D", wx, wy) {
        setPath(std::filesystem::current_path().string()+"/"+path);
        SDL_Surface *ico = png::LoadPNG(util.getFilePath("data/punk.png").c_str());
        if(ico != nullptr) {
            setWindowIcon(ico);
            SDL_FreeSurface(ico);
        }
        setObject(new Intro());
        object->load(this);
        activateConsole(util.getFilePath("data/font.ttf"),16,{255,255,255});
        console.hide();
        updateViewport();
    }

    virtual void draw() override {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        object->draw(this);
        swap();
        delay();
    }
    
    virtual void event(SDL_Event &e) override {
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
        if (e.type == SDL_WINDOWEVENT) {
            if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                updateViewport();
            }
        } 
#endif
    }
};

MainWindow *main_w = nullptr;

void eventProc() {
    main_w->proc();
}

int main(int argc, char **argv) {
    srand(static_cast<int>(time(0)));
#ifdef __EMSCRIPTEN__
    MainWindow main_window(".", 1920, 1080);
    main_w =&main_window;
    emscripten_set_main_loop(eventProc, 0, 1);
#else
#if defined(_WIN32) || defined(__linux__) || defined(__APPLE__)
    Arguments args = proc_args(argc, argv);
    try {
        MainWindow main_window(args.path, args.width, args.height);
        if(args.fullscreen)
            main_window.setFullScreen(true);
        main_window.loop();
    }  catch(mx::Exception &e) {
        SDL_Log("mx: Exception: %s", e.text().c_str());
    }
#elif defined(__ANDROID__)
    try {
        MainWindow main_window("");
        main_window.loop();
    }  catch(mx::Exception &e) {
        SDL_Log("mx: Exception: %s", e.text().c_str());
    }
#endif
#endif
    return 0;
}