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
    int start_shader = -1;
    public:

    Game(int start_shader_ = -1) : gl::GLObject(), start_shader(start_shader_) {
        cmd::AstExecutor::getExecutor().setInterrupt(&interrupt_command);
    }
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
        executor.setInterrupt(&interrupt_command);
        win->console.setInputCallback([this, win](gl::GLWindow *window, const std::string &text) -> int {
            try {
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
                        executor.setInterrupt(&this->interrupt_command);
                        std::cout << "Executing: " << text << std::endl;
                        std::string lineBuf;
                        
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
				    interrupt_command = false;
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
                        if(!out_stream.str().empty()) {
                            window->console.thread_safe_print(out_stream.str());
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
        std::vector<std::string> img = { "data/crystal_red.png", "data/saphire.png", "data/crystal_blue.png", "data/crystal_green.png", "data/crystal_pink.png", "data/diamond.png" };
        std::string img_index = img.at(mx::generateRandomInt(0, img.size()-1));
        setRandomShader(win, start_shader);
        logo.initSize(win->w, win->h);
        logo.loadTexture(&program, win->util.getFilePath(img_index), 0.0f, 0.0f, win->w, win->h);
    }

    void setRandomShader(gl::GLWindow *win, int index = -1) {
        static const char *vSource = R"(#version 330 core
            layout (location = 0) in vec3 aPos;
            layout (location = 1) in vec2 aTexCoord;
            out vec2 TexCoord;
            void main() {
                gl_Position = vec4(aPos, 1.0); 
                TexCoord = aTexCoord;        
            }
        )";

        static std::vector<std::string> shaders = { 
            win->util.getFilePath("data/shaders/default.glsl"), 
            win->util.getFilePath("data/shaders/cyclone.glsl"), 
            win->util.getFilePath("data/shaders/geometric.glsl"),
            win->util.getFilePath("data/shaders/distort.glsl"),
            win->util.getFilePath("data/shaders/atan.glsl"),
            win->util.getFilePath("data/shaders/huri.glsl"),
            win->util.getFilePath("data/shaders/vhs.glsl"),
            win->util.getFilePath("data/shaders/fractal.glsl"),
            win->util.getFilePath("data/shaders/color-f2.glsl")
        };   
        
        std::fstream file;
        if(index == -1) {
            file.open(shaders.at(mx::generateRandomInt(0, shaders.size()-1)), std::ios::in);
        } else {
            if(index < 0 || index >= static_cast<int>(shaders.size())) {
                std::cerr << "Invalid shader index. Using random shader." << std::endl;
                file.open(shaders.at(mx::generateRandomInt(0, shaders.size()-1)), std::ios::in);
            } else {
                file.open(shaders.at(index), std::ios::in);
            }
        }
        if (!file.is_open()) {
            std::cerr << "Failed to open shader file." << std::endl;
            return;
        } 
        std::ostringstream shader_source;
        shader_source << file.rdbuf();
        file.close();
        if(program.loadProgramFromText(vSource, shader_source.str())) {
            program.silent(true);
            program.useProgram();
            program.setUniform("textTexture", 0);
            program.setUniform("time_f", 0.0f);
            program.setUniform("alpha", 1.0f);
            GLint windowSizeLoc = glGetUniformLocation(program.id(), "iResolution");
            glUniform2f(windowSizeLoc, static_cast<float>(win->w), static_cast<float>(win->h));
        } else {
            std::cerr << "Failed to load shader program from file." << std::endl;
        }
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
                if(program_running && interrupt_command == false) {
                    win->console.thread_safe_print("\nCTRL+C Interrupt - Command interrupted\n");
                    interrupt_command = true;
                } else {
                    win->console.thread_safe_print("\nCTRL+C Interrupt - No Command Running\n");
                }
                win->console.process_message_queue();  
                return;
            }
            if(e.key.keysym.sym == SDLK_r && (e.key.keysym.mod & KMOD_CTRL)) {
                win->console.thread_safe_print("\nCTRL+R - Reloading Random shader\n");
                win->console.process_message_queue();
                setRandomShader(win, -1);
                return;
            }
        }
    }

    void update(float deltaTime) {
        float time_f = 0.0f;
        time_f = fmod(static_cast<float>(SDL_GetTicks()) / 1000.0f, 10000.0f);
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
    MainWindow(int shader_index, std::string path, int tw, int th) : gl::GLWindow("Console Skeleton", tw, th) {
        setPath(std::filesystem::current_path().string()+"/"+path);
        setObject(new Game(shader_index));
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

struct CustomArguments : Arguments {
    int shader_index = -1;

};

CustomArguments proc_custom_args(int &argc, char **argv) {
	CustomArguments args;
	Argz<std::string> parser(argc, argv);
    parser.addOptionSingle('h', "Display help message")
        .addOptionSingleValue('p', "assets path")
        .addOptionDoubleValue('P', "path", "assets path")
        .addOptionSingleValue('r',"Resolution WidthxHeight")
        .addOptionDoubleValue('R',"resolution", "Resolution WidthxHeight")
        .addOptionDoubleValue('s', "shader", "Shader index")
        .addOptionSingle('f', "fullscreen")
        .addOptionDouble('F', "fullscreen", "fullscreen");
    Argument<std::string> arg;
    std::string path;
    int value = 0;
    int tw = 1280, th = 720;
    bool fullscreen = false;
    int shader_index = -1;
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
                    	std::cerr << "Error invalid resolution use WidthxHeight\n";
                        std::cerr.flush();
                        exit(EXIT_FAILURE);
                    }
                    std::string left, right;
                    left = arg.arg_value.substr(0, pos);
                    right = arg.arg_value.substr(pos+1);
                    tw = atoi(left.c_str());
                    th = atoi(right.c_str());
                }
                    break;
                case 'f':
                case 'F':
                    fullscreen = true;
                    break;
                case 's':
                case 'S':
                    shader_index = atoi(arg.arg_value.c_str());
                    break;

            }
        }
    } catch (const ArgException<std::string>& e) {
        std::cerr << "mx: Argument Exception" << e.text() << std::endl;
		args.width = 1280;
		args.height = 720;
		args.path = ".";
		args.fullscreen = false;
        args.shader_index = -1;
		return args;
    }
    if(path.empty()) {
        std::cerr << "mx: No path provided trying default current directory.\n";
        path = ".";
    }
	args.width = tw;
	args.height = th;	
	args.path = path;
	args.fullscreen = fullscreen;
    args.shader_index = shader_index;
	return args;
}


int main(int argc, char **argv) {
    cmd::AstExecutor::getExecutor().setInterrupt(nullptr);
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
    CustomArguments args = proc_custom_args(argc, argv);
    try {
        MainWindow main_window(args.shader_index, args.path, args.width, args.height);
        main_window.loop();
    } catch(const mx::Exception &e) {
        mx::system_err << "mx: Exception: " << e.text() << "\n";
        mx::system_err.flush();
        return EXIT_FAILURE;
    }
#endif
    return 0;
}
