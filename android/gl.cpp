#include"gl.hpp"
#include <unistd.h> // Required for access()

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


    void GLWindow::setWindowIcon(SDL_Surface *ico) {
        SDL_SetWindowIcon(window, ico);
    }

    void GLWindow::updateViewport() {
        SDL_GL_GetDrawableSize(window, &this->w, &this->h);
        glViewport(0, 0, this->w, this->h);
        object->resize(this, this->w, this->h);
        text.init(this->w, this->h);
    }

    void GLWindow::initGL(const std::string &title, int width, int height) {
        if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO) < 0) {
            return;
        }

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    
        window = SDL_CreateWindow(title.c_str(),SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,width,height,SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
        if (!window) {
            throw std::runtime_error("SDL_Error: Failed to create SDL window: " + std::string(SDL_GetError()));
        }
        glContext = SDL_GL_CreateContext(window);
        if (!glContext) {
            throw std::runtime_error("SDL_Error: Failed to create OpenGL context: " + std::string(SDL_GetError()));
        }
        if (SDL_GL_SetSwapInterval(1) < 0) {
            SDL_Log("Warning: Unable to set VSync! SDL_Error: %s\n", SDL_GetError());
            }
        SDL_Log("OpenGL Version: [%s]\n" , glGetString(GL_VERSION));
        w = width;
        h = height;
        if (TTF_Init() < 0) {
            SDL_Log("TTF Failed to init\n");
            return;
        }
        IMG_Init(IMG_INIT_PNG);
        SDL_GL_GetDrawableSize(window, &this->w, &this->h);
        glViewport(0, 0, this->w, this->h);
        text.init(this->w, this->h);
    }

    void GLWindow::setWindowSize(int wx, int wy) {
        SDL_GL_GetDrawableSize(window, &this->w, &this->h);
        glViewport(0, 0, w, h);
    }

    void GLWindow::setWindowTitle(const std::string &title) {
        SDL_SetWindowTitle(window, title.c_str());
    }
    void GLWindow::setFullScreen(bool full) {
        if(full) {
            SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
        } else {
            SDL_SetWindowFullscreen(window, 0);
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

    void GLWindow::quit() {
        active = false;
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
        release();
    }

    void ShaderProgram::release() {
        if(vertex_shader) {
            glDeleteShader(vertex_shader);
            vertex_shader = 0;
        }
        if(fragment_shader) {
            glDeleteShader(fragment_shader);
            fragment_shader = 0;
        }
        if(shader_id) {
            glDeleteProgram(shader_id);
            shader_id = 0;
        }
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
            SDL_Log("Shader: %s\n", log);
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
            SDL_Log("Program: %s\n", log);
            delete [] log;
        }
    }
    bool ShaderProgram::checkError() {
        bool e = false;
        int glErr = glGetError();
        while(glErr != GL_NO_ERROR) {
            SDL_Log("OpenGL Error: %d\n", glErr);
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
        
        if(vertCompiled != GL_TRUE) {
            SDL_Log("Error on vertex compile");
            printShaderLog(vShader);
            glDeleteShader(vShader);
            glDeleteShader(fShader);
            return 0;
        }
        
        glCompileShader(fShader);
        checkError();
        
        glGetShaderiv(fShader, GL_COMPILE_STATUS, &fragCompiled);
        
        if(fragCompiled != GL_TRUE) {
            SDL_Log("Error on fragment compile\n");
            printShaderLog(fShader);
            glDeleteShader(vShader);
            glDeleteShader(fShader);
            return 0;
        }
        GLuint vfProgram = glCreateProgram();
        glAttachShader(vfProgram, vShader);
        glAttachShader(vfProgram, fShader);
        glLinkProgram(vfProgram);
        checkError();
        glGetProgramiv(vfProgram, GL_LINK_STATUS, &linked);
        if(linked != GL_TRUE) {
            SDL_Log("Linking failed..\n");
            printProgramLog(vfProgram);
            glDeleteShader(vShader);
            glDeleteShader(fShader);
            glDeleteProgram(vfProgram);
            return 0;
        }
        vertex_shader = vShader;
        fragment_shader = fShader;
        return vfProgram;
    }
    
    GLuint ShaderProgram::createProgramFromFile(const std::string &vert, const std::string &frag) {
        std::string v,f;
        v = mx::LoadTextFile(vert.c_str());
        f = mx::LoadTextFile(frag.c_str());
        return createProgram(v.c_str(), f.c_str());
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
            return;
        }
        glUniform1i(location, value);
    }

    void ShaderProgram::setUniform(const std::string &name, float value) {
        GLint location = glGetUniformLocation(shader_id, name.c_str());
        if (location == -1) {
            return;
        }
        glUniform1f(location, value);
    }

    void ShaderProgram::setUniform(const std::string &name, const glm::vec2 &value) {
        GLint location = glGetUniformLocation(shader_id, name.c_str());
        if (location == -1) {
            return;
        }
        glUniform2fv(location, 1, glm::value_ptr(value));
    }

    void ShaderProgram::setUniform(const std::string &name, const glm::vec3 &value) {
        GLint location = glGetUniformLocation(shader_id, name.c_str());
        if (location == -1) {
            return;
        }
        glUniform3fv(location, 1, glm::value_ptr(value));
    }

    void ShaderProgram::setUniform(const std::string &name, const glm::vec4 &value) {
        GLint location = glGetUniformLocation(shader_id, name.c_str());
        if (location == -1) {
            return;
        }
        glUniform4fv(location, 1, glm::value_ptr(value));
    }

    void ShaderProgram::setUniform(const std::string &name, const glm::mat4 &value) {
        GLint location = glGetUniformLocation(shader_id, name.c_str());
        if (location == -1) {
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
        SDL_Surface *surface = png::LoadPNG(filename.c_str());
        if (!surface) {
            throw mx::Exception("Error loading PNG file.");
        }
        SDL_Surface *converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
        SDL_FreeSurface(surface);
        surface = converted;
        mx::Texture::flipSurface(surface);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
        SDL_FreeSurface(surface);
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            throw mx::Exception("OpenGL error occurred.");
        }
        glBindTexture(GL_TEXTURE_2D, 0);
        return texture;
    }

    GLuint loadTexture(const std::string &filename, int &w, int &h) {
        GLuint texture = 0;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        SDL_Surface *surface = png::LoadPNG(filename.c_str());
        if (!surface) {
            throw mx::Exception("Error loading PNG file.");
        }
        SDL_Surface *converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
        SDL_FreeSurface(surface);
        surface = converted;
        surface = mx::Texture::flipSurface(surface);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
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
        SDL_Surface *converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
        if(!converted) {
            glDeleteTextures(1, &texture);
            throw mx::Exception("Failed to flip surface.");
        }

        if(flip) {
            SDL_Surface *flipped = mx::Texture::flipSurface(converted);
            if (!flipped) {
                glDeleteTextures(1, &texture);
                SDL_FreeSurface(converted);
                glBindTexture(GL_TEXTURE_2D, 0);
                throw mx::Exception("Failed to flip surface.");
            }
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, flipped->w, flipped->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, flipped->pixels);
        } else {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, converted->w, converted->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, converted->pixels);    
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
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
            return 0;
        }
        surface = mx::Texture::flipSurface(surface);
        textWidth = surface->w;
        textHeight = surface->h;
        if (surface->format->format != SDL_PIXELFORMAT_RGBA32) {
            SDL_Surface *converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
            if (!converted) {
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

    GLSprite::GLSprite() : texture{0}, textureName{"textTexture"} {}

    void GLSprite::initSize(float w, float h) {
        screenWidth = w;
        screenHeight = h;
    }

    void GLSprite::setShader(ShaderProgram *program) {
        shader = program;
    }

    void GLSprite::setName(const std::string &name) {
        textureName = name;
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
        this->shader->setUniform(textureName, 0);
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
        this->shader->setUniform(textureName, 0);
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
        this->shader->setUniform(textureName, 0);
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

