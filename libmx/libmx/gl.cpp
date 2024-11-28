#ifdef WITH_GL
#include"gl.hpp"

namespace gl {

    GLWindow::~GLWindow() {
        
        object.reset();

        if (glContext) {
            SDL_GL_DeleteContext(glContext);
        }
        if(window)
            SDL_DestroyWindow(window);

        TTF_Quit();
        SDL_Quit();
    }

    void GLWindow::initGL(const std::string &title, int width, int height) {

        if(SDL_Init(SDL_INIT_VIDEO) < 0) {
            mx::system_err << "Error initalizing SDL.\n";
            mx::system_err.flush();
            exit(EXIT_FAILURE);
        }
#ifndef __EMSCRIPTEN__
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif
        window = SDL_CreateWindow(title.c_str(),SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,width,height,SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

        if (!window) {
            throw std::runtime_error("Failed to create SDL window: " + std::string(SDL_GetError()));
        }

        glContext = SDL_GL_CreateContext(window);
        if (!glContext) {
            throw std::runtime_error("Failed to create OpenGL context: " + std::string(SDL_GetError()));
        }
#ifdef __EMSCRIPTEN__
        glewExperimental = GL_TRUE;
        GLenum glewError = glewInit();
        if (glewError != GLEW_OK) {
            throw std::runtime_error("Failed to initialize GLEW: " + std::string((const char*)glewGetErrorString(glewError)));
        }
        if (SDL_GL_SetSwapInterval(1) < 0) {
            throw std::runtime_error("Failed to set VSync: " + std::string(SDL_GetError()));
        }
#else
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        exit(EXIT_FAILURE);
    }
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << "\n";
#endif
        w = width;
        h = height;
        if (TTF_Init() < 0) {
            mx::system_err << "Failed to initialize SDL_ttf: " << TTF_GetError() << std::endl;
            mx::system_err.flush();
            exit(EXIT_FAILURE);
        }
    }

    void GLWindow::setObject(gl::GLObject *o) {
        object.reset(o);
    }

    void GLWindow::swap() {
         if (window) {
            SDL_GL_SwapWindow(window);
        }
    }

    void GLWindow::loop() {
        if(!object) {
            throw mx::Exception("Requires you set an Object");
        }
        active = true;
        while(active) {
            proc();
        }
    }

    void GLWindow::delay() {
#ifndef __EMSCRIPTEN__
        const int frameDelay = 1000 / 60;
        Uint32 frameStart = SDL_GetTicks();
        int frameTime = SDL_GetTicks() - frameStart;
        
        if (frameDelay > frameTime) {
            SDL_Delay(frameDelay - frameTime);
        }
#endif
    }

    void GLWindow::proc() {
        if(!object) {
            throw mx::Exception("Requires an active Object");
        }
        while(SDL_PollEvent(&e)) {
            if(e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE))
                active = false;

            event(e);
            object->event(this, e);
        }
        draw();
    }

       ShaderProgram::ShaderProgram() : shader_id{0} {
        
    }
    
    ShaderProgram::ShaderProgram(GLuint id) : shader_id{id} {
        
    }
    
    ShaderProgram &ShaderProgram::operator=(const ShaderProgram &p) {
        shader_id = p.shader_id;
        name_ = p.name_;
        return *this;
    }
    
    int ShaderProgram::printShaderLog(GLuint shader) {
        int len = 0;
        int ch = 0;
        char *log;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        if(len > 0) {
            log = new char [len+1];
            glGetShaderInfoLog(shader, len, &ch, log);
            mx::system_err << "Shader: " << log << "\n";
            delete [] log;
        }
        return 0;
    }
    void ShaderProgram::printProgramLog(int p) {
        int len = 0;
        int ch = 0;
        char *log;
        glGetShaderiv(p, GL_INFO_LOG_LENGTH, &len);
        if(len > 0) {
            log = new char [len+1];
            glGetProgramInfoLog(p, len, &ch, log);
            mx::system_err << "Program: " << log << "\n";
            delete [] log;
        }
    }
    bool ShaderProgram::checkError() {
        bool e = false;
        int glErr = glGetError();
        while(glErr != GL_NO_ERROR) {
            mx::system_err << "GL Error: " << glErr << "\n";
            //mx::system_err << "Error String: " << glewGetErrorString(glErr) << "\n";
            e = true;
            glErr = glGetError();
        }
        return e;
    }
    
    GLuint ShaderProgram::createProgram(const char *vshaderSource, const char *fshaderSource) {
        GLint vertCompiled;
        GLint fragCompiled;
        GLint linked;
        GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
        GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
        
        glShaderSource(vShader, 1, &vshaderSource, NULL);
        glShaderSource(fShader, 1, &fshaderSource, NULL);
        
        glCompileShader(vShader);
        checkError();
        glGetShaderiv(vShader, GL_COMPILE_STATUS, &vertCompiled);
        
        if(vertCompiled != 1) {
            mx::system_err << "Error on Vertex compile\n";
            printShaderLog(vShader);
            return 0;
        }
        
        glCompileShader(fShader);
        checkError();
        
        glGetShaderiv(fShader, GL_COMPILE_STATUS, &fragCompiled);
        
        if(fragCompiled != 1) {
            mx::system_err << "Error on Fragment compile\n";
            mx::system_err.flush();
            exit(EXIT_FAILURE);
            printShaderLog(fShader);
            return 0;
        }
        GLuint vfProgram = glCreateProgram();
        glAttachShader(vfProgram, vShader);
        glAttachShader(vfProgram, fShader);
        glLinkProgram(vfProgram);
        checkError();
        glGetProgramiv(vfProgram, GL_LINK_STATUS, &linked);
        if(linked != 1) {
            mx::system_err << "Linking failed...\n";
            mx::system_err.flush();
            printProgramLog(vfProgram);
            exit(EXIT_FAILURE);
            return 0;
        }
        return vfProgram;
    }
    
    GLuint ShaderProgram::createProgramFromFile(const std::string &vert, const std::string &frag) {
        std::fstream v,f;
        v.open(vert, std::ios::in);
        if(!v.is_open()) {
            mx::system_err << "Error could not open file: " << vert << "\n";
            mx::system_err.flush();
            exit(EXIT_FAILURE);
            
        }
        f.open(frag, std::ios::in);
        if(!f.is_open()) {
            mx::system_err << "Error could not open file: " << frag << "\n";
            mx::system_err.flush();
            exit(EXIT_FAILURE);

        }
        std::ostringstream stream1, stream2;
        stream1 << v.rdbuf();
        stream2 << f.rdbuf();
        return createProgram(stream1.str().c_str(), stream2.str().c_str());
    }
    
    bool ShaderProgram::loadProgram(const std::string &v, const std::string &f) {
        shader_id = createProgramFromFile(v,f);
        if(shader_id)
            return true;
        return false;
    }
    
    void ShaderProgram::setName(const std::string &n) {
        name_ = n;
    }
    
    void ShaderProgram::useProgram() {
        glUseProgram(shader_id);
    }

    void ShaderProgram::setUniform(const std::string &name, int value) {
        GLint location = glGetUniformLocation(shader_id, name.c_str());
        if (location == -1) {
            mx::system_err << "Uniform '" << name << "' not found or not used in shader.\n";
            return;
        }
        glUniform1i(location, value);
    }

    void ShaderProgram::setUniform(const std::string &name, float value) {
        GLint location = glGetUniformLocation(shader_id, name.c_str());
        if (location == -1) {
            mx::system_err << "Uniform '" << name << "' not found or not used in shader.\n";
            return;
        }
        glUniform1f(location, value);
    }

    void ShaderProgram::setUniform(const std::string &name, const glm::vec3 &value) {
        GLint location = glGetUniformLocation(shader_id, name.c_str());
        if (location == -1) {
            mx::system_err << "Uniform '" << name << "' not found or not used in shader.\n";
            return;
        }
        glUniform3fv(location, 1, glm::value_ptr(value));
    }

    void ShaderProgram::setUniform(const std::string &name, const glm::vec4 &value) {
        GLint location = glGetUniformLocation(shader_id, name.c_str());
        if (location == -1) {
            mx::system_err << "Uniform '" << name << "' not found or not used in shader.\n";
            return;
        }
        glUniform4fv(location, 1, glm::value_ptr(value));
    }

    void ShaderProgram::setUniform(const std::string &name, const glm::mat4 &value) {
        GLint location = glGetUniformLocation(shader_id, name.c_str());
        if (location == -1) {
            mx::system_err << "Uniform '" << name << "' not found or not used in shader.\n";
            return;
        }
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
    }

}



#endif
