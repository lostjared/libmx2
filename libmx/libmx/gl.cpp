#ifdef __EMSCRIPTEN__
#include"config.hpp"
#else
#include"config.h"
#endif

#ifdef WITH_GL
#include"gl.hpp"
#ifdef __EMSCRIPTEN__
#include<emscripten/emscripten.h>
#include<emscripten/html5.h>
#include<GLES3/gl3.h>
#endif

namespace gl {
   
    GLWindow::~GLWindow() {
   
        if(object)
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
        mx::redirect();
        if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO) < 0) {
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
#ifdef __EMSCRIPTEN__
    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);
    attrs.majorVersion = 2; // WebGL 2.0
    attrs.minorVersion = 0;
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context = emscripten_webgl_create_context("#canvas", &attrs);
    if (context <= 0) {
        std::cerr << "Failed to create WebGL 2.0 context" << std::endl;
        exit(EXIT_FAILURE);
    }
    emscripten_webgl_make_context_current(context);
#else
    glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        throw std::runtime_error("Failed to create OpenGL context: " + std::string(SDL_GetError()));
    }

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        exit(EXIT_FAILURE);
    }
    mx::system_out << "OpenGL Version: [" << glGetString(GL_VERSION) << "]\n";
#endif
        w = width;
        h = height;
        if (TTF_Init() < 0) {
            mx::system_err << "Failed to initialize SDL_ttf: " << TTF_GetError() << std::endl;
            mx::system_err.flush();
            exit(EXIT_FAILURE);
        }
        text.init(this->w, this->h);
    }

    void GLWindow::setWindowSize(int w, int h) {
#ifdef __EMSCRIPTEN
        SDL_SetWindowSize(window, w, h);
        width = w;
        height = h;
        this->w = w;
        this->h = h;
        emscripten_webgl_make_context_current(context);
        emscripten_set_canvas_element_size("#canvas", w, h);
#else
        SDL_SetWindowSize(window, w, h);
#endif
        glViewport(0, 0, w, h); 
    }

    void GLWindow::setWindowTitle(const std::string &title) {
        SDL_SetWindowTitle(window, title.c_str());
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
            if(e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && 
e.key.keysym.sym == SDLK_ESCAPE)) {
                active = false;
                return;
            }
		

            event(e);
            object->event(this, e);
        }
        draw();
    }

    ShaderProgram::ShaderProgram() : shader_id{0} {
        
    }
    
    ShaderProgram::ShaderProgram(GLuint id) : shader_id{id} {
        
    }
    
    ShaderProgram::~ShaderProgram() {
        if(shader_id)
            glDeleteProgram(shader_id);
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
            printShaderLog(fShader);
            exit(EXIT_FAILURE);
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

    bool ShaderProgram::loadProgramFromText(const std::string &v, const std::string &f) {
        shader_id = createProgram(v.c_str(), f.c_str());
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

    void ShaderProgram::setUniform(const std::string &name, const glm::vec2 &value) {
        GLint location = glGetUniformLocation(shader_id, name.c_str());
        if (location == -1) {
            mx::system_err << "Uniform '" << name << "' not found or not used in shader.\n";
            return;
        }
        glUniform2fv(location, 1, glm::value_ptr(value));
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

    GLuint loadTexture(const std::string &filename) {
        GLuint texture = 0;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        SDL_Surface *surface = png::LoadPNG(filename.c_str());
        if (!surface) {
            throw mx::Exception("Error loading PNG file.");
        }
        SDL_Surface *converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
        SDL_FreeSurface(surface);
        surface = converted;
        surface = mx::Texture::flipSurface(surface);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
        SDL_FreeSurface(surface);
        glBindTexture(GL_TEXTURE_2D, 0);
        return texture;
    }

    GLuint loadTexture(const std::string &filename, int &w, int &h) {
            GLuint texture = 0;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        SDL_Surface *surface = png::LoadPNG(filename.c_str());
        if (!surface) {
            throw mx::Exception("Error loading PNG file.");
        }
        SDL_Surface *converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
        SDL_FreeSurface(surface);
        surface = converted;
        surface = mx::Texture::flipSurface(surface);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
        w = surface->w;
        h = surface->h;
        SDL_FreeSurface(surface);
        glBindTexture(GL_TEXTURE_2D, 0);
        return texture;
    }

    GLuint createTexture(SDL_Surface *surface, bool flip) {
        if (!surface) {
            throw mx::Exception("Surface is null: Unable to load PNG file.");
        }
        GLuint texture = 0;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        SDL_Surface *converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
      
        if(flip) {
            SDL_Surface *flipped = mx::Texture::flipSurface(converted);
            if (!flipped) {
                SDL_FreeSurface(converted);
                glBindTexture(GL_TEXTURE_2D, 0);
                throw mx::Exception("Failed to flip surface.");
            }
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, flipped->w, flipped->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, flipped->pixels);
        } else {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, converted->w, converted->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, converted->pixels);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
        SDL_FreeSurface(converted);
        return texture;
    }
    
    void updateTexture(GLuint texture, SDL_Surface *surface, bool flip) {
        if (!surface || !surface->pixels) {
            throw mx::Exception("Invalid surface provided to updateTexture");
        }
        if (surface->w <= 0 || surface->h <= 0) {
            throw mx::Exception("Surface dimensions are invalid in updateTexture");
        }

        surface = mx::Texture::flipSurface(surface);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, surface->w, surface->h, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    SDL_Surface *createSurface(int w, int h) {
        SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
        if(!surf) {
            throw mx::Exception("Error creating surface gl::createSurface");
        }
        return surf;
    }

#ifdef __EMSCRIPTEN__
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
        precision mediump float;
        in vec2 TexCoord;
        out vec4 FragColor;
        uniform sampler2D textTexture; 
        void main() {
            FragColor = texture(textTexture, TexCoord);
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
        out vec4 FragColor;
        uniform sampler2D textTexture; 
        void main() {
            FragColor = texture(textTexture, TexCoord);
        }
    )";
#endif
    GLText::GLText() {}

    void GLText::init(int width, int height) {
        if(!textShader.loadProgramFromText(vSource, fSource)) {
            throw mx::Exception("Could not load Text Shader ");
        }
        w = width;
        h = height;
    }

    void GLText::setColor(SDL_Color col) {
        color = col;
    }

    GLuint GLText::createText(const std::string &text, TTF_Font *font, SDL_Color color, int &textWidth, int &textHeight) {
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
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textWidth, textHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
        SDL_FreeSurface(surface);
        return texture;
    }
    
    void GLText::renderText(GLuint texture, float x, float y, int textWidth, int textHeight, int screenWidth, int screenHeight) {
        float ndcX = (x / screenWidth) * 2.0f - 1.0f;       
        float ndcY = 1.0f - (y / screenHeight) * 2.0f;      
        float w = (float)textWidth / screenWidth * 2.0f;    
        float h = (float)textHeight / screenHeight * 2.0f;  
        GLfloat vertices[] = {
            ndcX,       ndcY,       0.0f, 0.0f, 1.0f,  
            ndcX + w,   ndcY,       0.0f, 1.0f, 1.0f,  
            ndcX,       ndcY - h,   0.0f, 0.0f, 0.0f,  
            ndcX + w,   ndcY - h,   0.0f, 1.0f, 0.0f   
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

    void GLText::printText_Solid(const mx::Font &f, float x, float y, const std::string &text) {
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        textShader.useProgram();
        int textWidth = 0, textHeight = 0;
        GLuint textTexture= createText(text, f.wrapper().unwrap(), color, textWidth, textHeight);
        renderText(textTexture, x, y, textWidth, textHeight, w,h);
        glDeleteTextures(1, &textTexture);
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
    }

    GLSprite::GLSprite() : texture{0} {}

    void GLSprite::initSize(float w, float h) {
        screenWidth = w;
        screenHeight = h;
    }

    void GLSprite::setShader(ShaderProgram *program) {
        shader = program;
    }
    
    GLSprite::~GLSprite() {
        if (texture != 0) {
            glDeleteTextures(1, &texture);
            texture = 0;
        }
        if (VBO != 0) {
            glDeleteBuffers(1, &VBO);
            VBO = 0;
        }
        if (VAO != 0) {
            glDeleteVertexArrays(1, &VAO);
            VAO = 0;
        }
    }

    void GLSprite::initWithTexture(ShaderProgram *program, GLuint text, float x, float y, int textWidth, int textHeight) {
        if (!program) {
            throw mx::Exception("Shader program is null in GLSprite::initWithTexture");
        }
        if (screenWidth <= 0 || screenHeight <= 0) {
            throw mx::Exception("Invalid screen dimensions in GLSprite::initWithTexture");
        }
        this->shader = program;
        if(this->texture != 0) {
            glDeleteTextures(1, &this->texture);
        }
        this->texture = text;
        float ndcX = (x / screenWidth) * 2.0f - 1.0f;       
        float ndcY = 1.0f - (y / screenHeight) * 2.0f;      
        float w = (float)textWidth / screenWidth * 2.0f;    
        float h = (float)textHeight / screenHeight * 2.0f;  
        vertices = {
            ndcX,       ndcY,       0.0f, 0.0f, 1.0f,  
            ndcX + w,   ndcY,       0.0f, 1.0f, 1.0f,  
            ndcX,       ndcY - h,   0.0f, 0.0f, 0.0f,  
            ndcX + w,   ndcY - h,   0.0f, 1.0f, 0.0f   
        };
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        this->shader->useProgram();
        this->shader->setUniform("textTexture", 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void GLSprite::loadTexture(ShaderProgram *shader, const std::string &tex, float x, float y, int textWidth, int textHeight) {
        this->shader = shader;
        if (texture != 0) {
           glDeleteTextures(1, &texture);
}
        texture = gl::loadTexture(tex); 
        if (!texture) {
            throw mx::Exception("Failed to load texture: " + tex);
        }
        float ndcX = (x / screenWidth) * 2.0f - 1.0f;       
        float ndcY = 1.0f - (y / screenHeight) * 2.0f;      
        float w = (float)textWidth / screenWidth * 2.0f;    
        float h = (float)textHeight / screenHeight * 2.0f;  
        vertices = {
            ndcX,       ndcY,       0.0f, 0.0f, 1.0f,  
            ndcX + w,   ndcY,       0.0f, 1.0f, 1.0f,  
            ndcX,       ndcY - h,   0.0f, 0.0f, 0.0f,  
            ndcX + w,   ndcY - h,   0.0f, 1.0f, 0.0f   
        };
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        this->shader->useProgram();
        this->shader->setUniform("textTexture", 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

      void GLSprite::loadTexture(ShaderProgram *shader, const std::string &tex, float x, float y) {
        this->shader = shader;
        if (texture != 0) {
           glDeleteTextures(1, &texture);
}
        texture = gl::loadTexture(tex, width, height); 
        int textWidth = width;
        int textHeight = height;

        if (!texture) {
            throw mx::Exception("Failed to load texture: " + tex);
        }
        float ndcX = (x / screenWidth) * 2.0f - 1.0f;       
        float ndcY = 1.0f - (y / screenHeight) * 2.0f;      
        float w = (float)textWidth / screenWidth * 2.0f;    
        float h = (float)textHeight / screenHeight * 2.0f;  
        vertices = {
            ndcX,       ndcY,       0.0f, 0.0f, 1.0f,  
            ndcX + w,   ndcY,       0.0f, 1.0f, 1.0f,  
            ndcX,       ndcY - h,   0.0f, 0.0f, 0.0f,  
            ndcX + w,   ndcY - h,   0.0f, 1.0f, 0.0f   
        };
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        this->shader->useProgram();
        this->shader->setUniform("textTexture", 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }


    void GLSprite::updateTexture(SDL_Surface *surf) {
        if (!surf || !surf->pixels) {
            throw mx::Exception("Invalid surface provided to updateTexture");
        }
        if (surf->w <= 0 || surf->h <= 0) {
            throw mx::Exception("Surface dimensions are invalid in updateTexture");
        }
        SDL_Surface *converted = surf;
        if (surf->format->format != SDL_PIXELFORMAT_RGBA32) {
            converted = SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_RGBA32, 0);
            if (!converted) {
                throw mx::Exception("Failed to convert surface to RGBA32 format");
            }
        }

        glBindTexture(GL_TEXTURE_2D, this->texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, converted->w, converted->h, GL_RGBA, GL_UNSIGNED_BYTE, converted->pixels);
        if (converted != surf) {
            SDL_FreeSurface(converted);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void GLSprite::draw() {
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        this->shader->useProgram();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, this->texture);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
    }

    void GLSprite::draw(GLuint texture_id, float x, float y, int w, int h) {
        float ndcX = (x / screenWidth) * 2.0f - 1.0f;
        float ndcY = 1.0f - (y / screenHeight) * 2.0f;
        float width = (float)w / screenWidth * 2.0f;
        float height = (float)h / screenHeight * 2.0f;
        vertices = {
            ndcX,       ndcY,       0.0f, 0.0f, 1.0f,  
            ndcX + width, ndcY,     0.0f, 1.0f, 1.0f,  
            ndcX,       ndcY - height, 0.0f, 0.0f, 0.0f,  
            ndcX + width, ndcY - height, 0.0f, 1.0f, 0.0f   
        };  
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        this->shader->useProgram();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture_id);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
    }
}

#endif
