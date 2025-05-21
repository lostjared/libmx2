#include"mx.hpp"
#include"gl.hpp"
#include"argz.hpp"
#include"ast.hpp"
#include"parser.hpp"
#include"scanner.hpp"
#include"types.hpp"
#include"string_buffer.hpp"
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <GLES3/gl3.h>
#endif
#include<atomic>
#include<thread>


#include"loadpng.hpp"

#define CHECK_GL_ERROR() \
{ GLenum err = glGetError(); \
if (err != GL_NO_ERROR) \
printf("OpenGL Error: %d at %s:%d\n", err, __FILE__, __LINE__); }

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include<filesystem>


class Game : public gl::GLObject {
public:
    Game() = default;
    virtual ~Game() override {}
    std::atomic<bool> interrupt_command{false};
    std::atomic<bool> program_running{false};
    cmd::AstExecutor &executor = cmd::AstExecutor::getExecutor();
    bool cmd_echo = true;
    
    void load(gl::GLWindow *win) override {
        font.loadFont(win->util.getFilePath("data/font.ttf"), 36);
        win->console.print("Console Skeleton Example\nLostSideDead Software\nhttps://lostsidedead.biz\n");
        win->console.setPrompt("$> ");
        win->console_visible = true;
        win->console.show();
        win->console.setInputCallback([this, win](gl::GLWindow *window, const std::string &text) -> int {
            try {
                std::cout << "Executing: " << text << std::endl;
                if(text == "@echo_on") {
                    cmd_echo = true;
                    window->console.thread_safe_print("Echoing commands on.\n");
                    return 0;
                } else if(text == "@echo_off") {
                    cmd_echo = false;
                    window->console.thread_safe_print("Echoing commands off.\n");
                    return 0;
                }  else if(!text.empty() && text[0] == '@') {
                    std::string command = text.substr(1);
                    auto tokenize = [](const std::string &text) {
                        std::vector<std::string> tokens;
                        std::istringstream iss(text);
                        std::string token;
                        while (iss >> token) {
                            tokens.push_back(token);
                        }
                        return tokens;
                    };
                    auto tokens = tokenize(command);
                    window->console.procDefaultCommands(tokens);
                    return  0;
                }
                if(cmd_echo) {
                    window->console.thread_safe_print("$ " + text + "\n");
                    window->console.process_message_queue();
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
                        std::ostringstream out_stream;
                        program_running = true;
                        executor.execute(input_stream, out_stream, ast);
                        if(!lineBuf.empty()) {
                            window->console.thread_safe_print(lineBuf);
                            window->console.process_message_queue();
                        }
                        program_running = false;
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
                win->console.process_message_queue();
                return 1;
            }
        });
#if defined(__EMSCRIPTEN__) 
    const char *vSource = R"(#version 300 es
            precision mediump float;
            layout (location = 0) in vec3 aPos;
            layout (location = 1) in vec2 aTexCoord;

            out vec2 TexCoord;

            void main() {
                gl_Position = vec4(aPos, 1.0); 
                TexCoord = aTexCoord;         
            }
    )";
    const char *fSource = R"(#version 300 es
    precision highp float;
    in vec2 TexCoord;
    out vec4 color;
    uniform sampler2D textTexture;
    uniform float time_f;
    uniform float alpha;

    void main(void) {
        vec2 tc = TexCoord;
        float rippleSpeed = 5.0;
        float rippleAmplitude = 0.03;
        float rippleWavelength = 10.0;
        float twistStrength = 1.0;
        float radius = length(tc - vec2(0.5, 0.5));
        float ripple = sin(tc.x * rippleWavelength + time_f * rippleSpeed) * rippleAmplitude;
        ripple += sin(tc.y * rippleWavelength + time_f * rippleSpeed) * rippleAmplitude;
        vec2 rippleTC = tc + vec2(ripple, ripple);
        float angle = twistStrength * (radius - 1.0) + time_f;
        float cosA = cos(angle);
        float sinA = sin(angle);
        mat2 rotationMatrix = mat2(cosA, -sinA, sinA, cosA);
        vec2 twistedTC = (rotationMatrix * (tc - vec2(0.5, 0.5))) + vec2(0.5, 0.5);
        vec4 originalColor = texture(textTexture, tc);
        vec4 twistedRippleColor = texture(textTexture, mix(rippleTC, twistedTC, 0.5));
        color = twistedRippleColor * alpha;
    }
)";
#else
    const char *vSource = R"(#version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec2 aTexCoord;
        out vec2 TexCoord;
        void main() {
            gl_Position = vec4(aPos, 1.0); 
            TexCoord = aTexCoord;        
        }
    )";

    const char *fSource = R"(#version 330 core
    in vec2 TexCoord;
    out vec4 color;
    uniform sampler2D textTexture;
    uniform float time_f;
    uniform float alpha;

    void main(void) {
        vec2 tc = TexCoord;
        float rippleSpeed = 5.0;
        float rippleAmplitude = 0.03;
        float rippleWavelength = 10.0;
        float twistStrength = 1.0;
        float radius = length(tc - vec2(0.5, 0.5));
        float ripple = sin(tc.x * rippleWavelength + time_f * rippleSpeed) * rippleAmplitude;
        ripple += sin(tc.y * rippleWavelength + time_f * rippleSpeed) * rippleAmplitude;
        vec2 rippleTC = tc + vec2(ripple, ripple);
        float angle = twistStrength * (radius - 1.0) + time_f;
        float cosA = cos(angle);
        float sinA = sin(angle);
        mat2 rotationMatrix = mat2(cosA, -sinA, sinA, cosA);
        vec2 twistedTC = (rotationMatrix * (tc - vec2(0.5, 0.5))) + vec2(0.5, 0.5);
        vec4 originalColor = texture(textTexture, tc);
        vec4 twistedRippleColor = texture(textTexture, mix(rippleTC, twistedTC, 0.5));
        color = twistedRippleColor * alpha;
    }
)";
#endif
        std::vector<std::string> img = { "data/crystal_red.png", "data/saphire.png", "data/crystal_blue.png", "data/crystal_green.png", "data/crystal_pink.png", "data/diamond.png" };
        std::string img_index = img.at(mx::generateRandomInt(0, img.size()-1));
        if (!program.loadProgramFromText(vSource, fSource)) {
            throw mx::Exception("Failed to load shader program");
        }
        program.useProgram();
        program.setUniform("textTexture", 0);
        program.setUniform("time_f", 0.0f);
        program.setUniform("alpha", 1.0f);
        logo.initSize(win->w, win->h);
        logo.loadTexture(&program, win->util.getFilePath(img_index), 0.0f, 0.0f, win->w, win->h);
    }

    

    void draw(gl::GLWindow *win) override {
        glDisable(GL_DEPTH_TEST);
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; 
        lastUpdateTime = currentTime;
        logo.draw();
        update(deltaTime);
    }
    
    void event(gl::GLWindow *win, SDL_Event &e) override {
        if(e.type == SDL_KEYDOWN) {
            if(e.key.keysym.sym == SDLK_c && (e.key.keysym.mod & KMOD_CTRL)) {
                if(program_running) {
                    win->console.thread_safe_print("\nCTRL+C Interrupt - Command interrupted\n");
                    interrupt_command = true;
                } else {
                    win->console.thread_safe_print("\nCTRL+C Interrupt - No Command Running\n");
                }
                win->console.process_message_queue();  
                return;
            }
        }
    }

    void update(float deltaTime) {
        static float time_f = 0.0f;
        time_f += deltaTime;
        program.setUniform("time_f", time_f);
    }
private:
    mx::Font font;
    gl::GLSprite logo;
    gl::ShaderProgram program;
    Uint32 lastUpdateTime = SDL_GetTicks();
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("Console Skeleton", tw, th) {
        setPath(std::filesystem::current_path().string()+"/"+path);
        setObject(new Game());
        activateConsole({25, 25, tw-50, th-50}, util.getFilePath("data/font.ttf"), 16, {255, 255, 255, 255});
        showConsole(true);  
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
        delay();
    }
};

MainWindow *main_w = nullptr;

void eventProc() {
    main_w->proc();
}

int main(int argc, char **argv) {
#ifdef __EMSCRIPTEN__
    try {
        MainWindow main_window("/", 1920, 1080);
        main_w = &main_window;
        emscripten_set_main_loop(eventProc, 0, 1);
    } catch(const mx::Exception &e) {
        mx::system_err << "mx: Exception: " << e.text() << "\n";
        mx::system_err.flush();
        return EXIT_FAILURE;
    }
#else
    Arguments args = proc_args(argc, argv);
    try {
        MainWindow main_window(args.path, args.width, args.height);
        main_window.loop();
    } catch(const mx::Exception &e) {
        mx::system_err << "mx: Exception: " << e.text() << "\n";
        mx::system_err.flush();
        return EXIT_FAILURE;
    }
#endif
    return 0;
}
