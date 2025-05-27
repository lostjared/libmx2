#include"mx.hpp"
#include"gl.hpp"
#include"argz.hpp"
#include"ast.hpp"
#include"parser.hpp"
#include"scanner.hpp"
#include"types.hpp"
#include"string_buffer.hpp"
#include"command.hpp"
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <GLES3/gl3.h>
#endif
#include<atomic>
#include<thread>
#include<filesystem>
#include<mutex>
#include<fstream>
#include<iostream>
#include"loadpng.hpp"
#include"version_info.hpp"
#define CHECK_GL_ERROR() \
{ GLenum err = glGetError(); \
if (err != GL_NO_ERROR) \
printf("OpenGL Error: %d at %s:%d\n", err, __FILE__, __LINE__); }

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


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
    std::mutex task_mutex;
    std::vector<std::function<void(gl::GLWindow*)>> main_thread_tasks;

    void queueTaskForMainThread(std::function<void(gl::GLWindow*)> task) {
        std::lock_guard<std::mutex> lock(task_mutex);
        main_thread_tasks.push_back(task);
    }

    void processMainThreadTasks(gl::GLWindow *win) {
        std::vector<std::function<void(gl::GLWindow*)>> tasks_to_run;
        {
            std::lock_guard<std::mutex> lock(task_mutex);
            if (!main_thread_tasks.empty()) {
                tasks_to_run.swap(main_thread_tasks);
            }
        }
        for (const auto& task : tasks_to_run) {
            task(win); 
        }
    }

    void useWSL(gl::GLWindow *win, const bool &enable) {
#ifdef _WIN32
        if(enable) {
            cmd::cmd_type = "wsl.exe ";
            win->console.thread_safe_print("WSL enabled. Commands will be executed in WSL.\n");
            win->console.process_message_queue();
        } else {
            cmd::cmd_type = "cmd.exe /c ";
            win->console.thread_safe_print("WSL disabled. Commands will be executed in cmd.exe.\n");
            win->console.process_message_queue();
        }
#endif
    }
    
    void load(gl::GLWindow *win) override {
        font.loadFont(win->util.getFilePath("data/font.ttf"), 36);
        win->console.print("Console Skeleton Example\nLostSideDead Software\nhttps://lostsidedead.biz\n");
        win->console.setPrompt("$> ");
        win->console_visible = true;
        win->console.show();
        shaders = { 
            win->util.getFilePath("data/shaders/default.glsl"), 
            win->util.getFilePath("data/shaders/cyclone.glsl"), 
            win->util.getFilePath("data/shaders/geometric.glsl"),
            win->util.getFilePath("data/shaders/distort.glsl"),
            win->util.getFilePath("data/shaders/atan.glsl"),
            win->util.getFilePath("data/shaders/huri.glsl"),
            win->util.getFilePath("data/shaders/vhs.glsl"),
            win->util.getFilePath("data/shaders/fractal.glsl"),
            win->util.getFilePath("data/shaders/color-f2.glsl"),
            win->util.getFilePath("data/shaders/color-cycle.glsl"),
            win->util.getFilePath("data/shaders/color-swirl.glsl"),
            win->util.getFilePath("data/shaders/color-shift.glsl")
        };   
    
        executor.setInterrupt(&interrupt_command);
        win->console.setInputCallback([this](gl::GLWindow *window, const std::string &text) -> int {
            auto tokenize = [](const std::string &text) {
                std::vector<std::string> tokens;
                std::istringstream iss(text);
                std::string token;
                while (iss >> token) {
                    tokens.push_back(token);
                }
                return tokens;
            };

            try {
                if(text.empty()) {
                    return 0; 
                }
                if(text == "random_shader") {
                    queueTaskForMainThread([this](gl::GLWindow* main_thread_win_param) {
                        this->setRandomShader(main_thread_win_param, -1);
                    });
                    window->console.thread_safe_print("Random shader command queued.\n"); // Feedback
                    window->console.process_message_queue();
                    return 0;
                } else if(text == "default_shader") {
                    queueTaskForMainThread([this](gl::GLWindow* main_thread_win_param) {
                        this->setRandomShader(main_thread_win_param, 0);
                    });
                    window->console.thread_safe_print("Default shader command queued.\n");
                    window->console.process_message_queue();    
                    return 0;
                } else if(text.compare(0, 10, "set_shader") == 0) {
                    std::string arg = text.substr(10); 
                    size_t start = arg.find_first_not_of(" \t\n\r\f\v");
                    if (start == std::string::npos) {
                        window->console.thread_safe_print("Error: No shader path provided.\n");
                        window->console.process_message_queue();
                        return 0;
                    }
                    arg = arg.substr(start);
                    if (arg.size() >= 2 && arg.front() == '"' && arg.back() == '"') {
                        std::string shader_path = arg.substr(1, arg.size() - 2);
                        if (!std::filesystem::exists(shader_path)) {
                            window->console.thread_safe_print("Error: Shader file does not exist: " + shader_path + "\n");
                            window->console.process_message_queue();
                            return 0;
                        }
                        queueTaskForMainThread([this, shader_path](gl::GLWindow* main_thread_win_param) {
                            this->setShader(main_thread_win_param, shader_path);
                        });
                        window->console.thread_safe_print("Set shader command queued for: " + shader_path + "\n");
                        window->console.process_message_queue();
                    } else {
                        window->console.thread_safe_print("Error: Shader path must be enclosed in quotes.\n");
                        window->console.process_message_queue();
                    }
                    return 0;
                } else if(text == "wsl_on") {
#ifdef _WIN32
                    queueTaskForMainThread([this](gl::GLWindow* main_thread_win_param) {
                        this->useWSL(main_thread_win_param, true);
                    });
                    return 0;
#else
                    window->console.thread_safe_print("WSL is not supported on this platform.\n");
                    window->console.process_message_queue();
                    return 1;
#endif
                } else if(text == "wsl_off") {
#ifdef _WIN32
                    queueTaskForMainThread([this](gl::GLWindow* main_thread_win_param) {
                        this->useWSL(main_thread_win_param, false);
                    });
                    return 0;
#else
                    window->console.thread_safe_print("WSL is not supported on this platform.\n");
                    window->console.process_message_queue();
                    return 1;
#endif
                }
                else if(text == "clear") {
                    window->console.clearText();
                    window->console.process_message_queue();    
                    return 0;
                } else if(text == "about") {
                    window->console.thread_safe_print("Console Skeleton Example v1.0\n");
                    window->console.thread_safe_print("MX2 version " + std::to_string(PROJECT_VERSION_MAJOR) + "." + std::to_string(PROJECT_VERSION_MINOR) + "\n");
                    window->console.thread_safe_print("Written by Jared Bruni\n");
                    window->console.thread_safe_print("https://lostsidedead.biz\n");
                    window->console.process_message_queue();
                    return 0;
                } else if(text == "exit") {
                    window->console.thread_safe_print("Exiting console...\n");
                    window->quit();
                    return 0;
                } else if(text == "help") {
                    window->console.thread_safe_print("Available Built-in Console Control commands:\n");
                    window->console.thread_safe_print(" - random_shader: Set a random shader.\n");
                    window->console.thread_safe_print(" - default_shader: Set the default shader.\n");
                    window->console.thread_safe_print(" - set_shader \"path/to/shader.glsl\": Set a specific shader.\n");
                    window->console.thread_safe_print(" - wsl_on: Enable WSL for command execution (Windows only).\n");
                    window->console.thread_safe_print(" - wsl_off: Disable WSL for command execution (Windows only).\n");
                    window->console.thread_safe_print(" - clear: Clear the console.\n");
                    window->console.thread_safe_print(" - about: Show information about this application.\n");
                    window->console.thread_safe_print(" - exit: Exit the application.\n");
                    window->console.process_message_queue();
                    return 0;
                }
                 else if(text == "@echo_on") {
                    cmd_echo = true;
                    window->console.thread_safe_print("Echoing commands on.\n");
                    window->console.process_message_queue();
                    return 0;
                } else if(text == "@echo_off") {
                    cmd_echo = false;
                    window->console.thread_safe_print("Echoing commands off.\n");
                    window->console.process_message_queue();
                    return 0;
                }  else if(!text.empty() && text[0] == '@') {
                    std::string command = text.substr(1);
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
                window->console.thread_safe_print("Error: " + std::string(e.what()) + "\n");
                window->console.process_message_queue();
                return 1;
            }
        });
        std::vector<std::string> img = { 
            "data/crystal_red.png", 
            "data/saphire.png", 
            "data/crystal_blue.png", 
            "data/crystal_green.png", 
            "data/crystal_pink.png", 
            "data/diamond.png"
        };
        std::string img_index = img.at(mx::generateRandomInt(0, img.size()-1));
        setRandomShader(win, start_shader);
        logo.initSize(win->w, win->h);
        logo.loadTexture(&program, win->util.getFilePath(img_index), 0.0f, 0.0f, win->w, win->h);
        CHECK_GL_ERROR();
    }
    
    const char *vSource = R"(#version 330 core
            layout (location = 0) in vec3 aPos;
            layout (location = 1) in vec2 aTexCoord;
            out vec2 TexCoord;
            void main() {
                gl_Position = vec4(aPos, 1.0); 
                TexCoord = aTexCoord;        
            }
    )";

    std::vector<std::string> shaders;

    void setRandomShader(gl::GLWindow *win, int index = -1) {
     
        std::fstream file;
        std::string filename;
        if(index == -1) {
            filename = shaders.at(mx::generateRandomInt(0, shaders.size()-1));
            file.open(filename, std::ios::in);
        } else {
            if(index < 0 || index >= static_cast<int>(shaders.size())) {
                std::cerr << "Invalid shader index. Using random shader." << std::endl;
                filename = shaders.at(mx::generateRandomInt(0, shaders.size()-1));
                file.open(filename, std::ios::in);
            } else {
                filename = shaders.at(index);
                file.open(filename, std::ios::in);
            }
        }
        if (!file.is_open()) {
            std::cerr << "Failed to open shader file: " << filename << std::endl;
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
            std::cerr << "Failed to load shader program from file: " << filename  << std::endl;
        }
        CHECK_GL_ERROR();
    }
    
    void setShader(gl::GLWindow *win, const std::string &shader_path) {
        std::fstream file(shader_path, std::ios::in);
        if (!file.is_open()) {
            std::cerr << "Failed to open shader file: " << shader_path << std::endl;
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
            win->console.thread_safe_print("Shader program loaded successfully from file: " + shader_path + "\n");
            win->console.process_message_queue();
        } else {
            win->console.thread_safe_print("Failed to load shader program from file: " + shader_path + "\n");
            win->console.thread_safe_print("Using default shader instead.\n");
            setRandomShader(win, 0);
            win->console.process_message_queue();
        }
    }
    void draw(gl::GLWindow *win) override {
        processMainThreadTasks(win);
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
        SDL_Surface *icon = png::LoadPNG(util.getFilePath("data/term.icon.png").c_str());
        if(icon) {
            setWindowIcon(icon);
            SDL_FreeSurface(icon);
        } else {
            std::cerr << "Failed to load window icon." << std::endl;
        }
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
        .addOptionDouble('H', "help", "Display help message")
        .addOptionSingle('v', "Display version")
        .addOptionDouble('V', "version", "Display version")     
        .addOptionSingleValue('p', "assets path")
        .addOptionDoubleValue('P', "path", "assets path")
        .addOptionSingleValue('r',"Resolution WidthxHeight")
        .addOptionDoubleValue('R',"resolution", "Resolution WidthxHeight")
        .addOptionDoubleValue('S', "shader", "Shader index")
        .addOptionSingle('s', "shader index")
        #ifdef _WIN32
        .addOptionSingle('w', "Use WSL")
        .addOptionDouble('W', "wsl", "Use WSL")
        #endif
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
#ifdef _WIN32
                case 'w':
                case 'W':
                    if(!std::filesystem::exists("C:\\Windows\\System32\\wsl.exe")) {
                        std::cerr << "mx: WSL not found, please install WSL to use this feature.\n";
                        std::cerr.flush();
                        exit(EXIT_FAILURE);
                    }
                    mx::system_out << "mx: WSL enabled\n";
                    mx::system_out.flush();
                    cmd::cmd_type = "wsl.exe";
                    break;
#endif
                case 'h':
                case 'H':
                case 'v':
                case 'V':
                    std::cout << "mx: Console Skeleton Example\n";
                    std::cout << "mx: Version " << version_string << "\n";
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
        if(args.fullscreen) 
            main_window.setFullScreen(true);
        main_window.loop();
    } catch(const mx::Exception &e) {
        mx::system_err << "mx: Exception: " << e.text() << "\n";
        mx::system_err.flush();
        return EXIT_FAILURE;
    }
#endif
    return 0;
}
