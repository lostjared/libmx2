#include"intro.hpp"
#include"game.hpp"

void Intro::load(gl::GLWindow  *win) {
    if (!shaderProgram.loadProgram(win->util.getFilePath("data/tri.vert"), win->util.getFilePath("data/tri.frag"))) {
        throw mx::Exception("Failed to load shader program");
    }
    if(!cube.openModel(win->util.getFilePath("data/cube.mxmod"))) {
        throw mx::Exception("Error loading cube model in intro");
    }
    cube.setShaderProgram(&shaderProgram, "texture1");
    shaderProgram.useProgram();
    float fov = 10.0f;
    float aspectRatio = static_cast<float>(win->w) / static_cast<float>(win->h);
    glm::mat4 projection = glm::perspective(glm::radians(fov), aspectRatio, 0.1f, 100.0f);
    shaderProgram.setUniform("projection", projection);
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 10.0f), 
        glm::vec3(0.0f, 0.0f, 0.0f),  
        glm::vec3(0.0f, 1.0f, 0.0f)   
    );
    shaderProgram.setUniform("view", view);    
    glEnable(GL_DEPTH_TEST);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    SDL_Surface *surface = png::LoadPNG(win->util.getFilePath("data/breakout-intro.png").c_str());
    if (!surface) {
        throw std::runtime_error("Failed to load texture image");
    }
    surface = mx::Texture::flipSurface(surface);
    GLenum format = (surface->format->BytesPerPixel == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, surface->w, surface->h, 0, format, GL_UNSIGNED_BYTE, surface->pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    SDL_FreeSurface(surface);
    shaderProgram.setUniform("texture1", 0);
    std::vector<GLuint> textures{texture};
    cube.setTextures(textures);
    
    font.loadFont(win->util.getFilePath("data/font.ttf"), 32);
    
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
out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D textTexture;
uniform float time_f;
uniform float alpha;

void main(void) {
    vec2 uv = TexCoord * 2.0 - 1.0;
    float len = length(uv);
    float bubble = smoothstep(0.8, 1.0, 1.0 - len);
    vec2 distort = uv * (1.0 + 0.1 * sin(time_f + len * 20.0));
    vec4 texColor = texture(textTexture, distort * 0.5 + 0.5);
    FragColor = mix(texColor, vec4(1.0, 1.0, 1.0, 1.0), bubble);
    FragColor = FragColor * alpha;
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
    out vec4 FragColor;
    in vec2 TexCoord;
    uniform sampler2D textTexture;
    uniform float time_f;
    uniform float alpha;
    void main(void) {
        vec2 uv = TexCoord * 2.0 - 1.0;
        float len = length(uv);
        float bubble = smoothstep(0.8, 1.0, 1.0 - len);
        vec2 distort = uv * (1.0 + 0.1 * sin(time_f + len * 20.0));
        vec4 texColor = texture(textTexture, distort * 0.5 + 0.5);
        FragColor = mix(texColor, vec4(1.0, 1.0, 1.0, 1.0), bubble);
        FragColor = FragColor * alpha;
    }
)";
#endif
    if (!program.loadProgramFromText(vSource, fSource)) {
        throw mx::Exception("Failed to load shader program");
    }
    program.useProgram();
    program.setUniform("textTexture", 0);
    program.setUniform("time_f", 0.0f);
    program.setUniform("alpha", 1.0f);
    sprite.initSize(win->w, win->h);
    sprite.loadTexture(&program, win->util.getFilePath("data/bluecrystal.png"), 0.0f, 0.0f, win->w, win->h);

    if(stick.open(0)) {
        mx::system_out << ">> [Controller] Intro initialized: " << stick.name() << "\n";
    }
}

void Intro::draw(gl::GLWindow *win) {
    
    
    
    CHECK_GL_ERROR();
#ifdef __EMSCRIPTEN__
    static Uint32 lastTime = emscripten_get_now(); 
    float currentTime = emscripten_get_now();
    float deltaTime = (currentTime - lastTime) / 1000.0f;
    lastTime = currentTime;
#else
    static Uint32 lastTime = SDL_GetTicks(); 
    Uint32 currentTime = SDL_GetTicks();
    float deltaTime = (currentTime - lastTime) / 1000.0f;
    lastTime = currentTime;
#endif
    
    glDisable(GL_DEPTH_TEST);
    program.useProgram();
    program.setUniform("alpha", 0.7f);
    static float time_f = 0.0f;
    time_f += deltaTime;
    program.setUniform("time_f", time_f);
    sprite.draw();
    
    glEnable(GL_DEPTH_TEST);
    shaderProgram.useProgram();
    static float rotationAngle = 0.0f;
    rotationAngle += 50.0f * deltaTime;
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::rotate(model, glm::radians(rotationAngle), glm::vec3(1.0f, 0.0f, 0.0f));
    shaderProgram.setUniform("model", model);
    glm::vec3 lightPos(0.0f, 3.0f, 2.0f); 
    glm::vec3 viewPos(0.0f, 0.0f, 10.0f);
    glm::vec3 lightColor(1.2f, 1.2f, 1.2f); 
    shaderProgram.setUniform("lightPos", lightPos);
    shaderProgram.setUniform("viewPos", viewPos);
    shaderProgram.setUniform("lightColor", lightColor);
    cube.drawArrays();
    glBindVertexArray(0);

    if(stick.getButton(SDL_CONTROLLER_BUTTON_A) || stick.getButton(SDL_CONTROLLER_BUTTON_START)) {
        win->setObject(new BreakoutGame());
        win->object->load(win);
        return;
    }

    if(stick.getButton(SDL_CONTROLLER_BUTTON_BACK)) {
        win->quit();
        return;
    }

    glDisable(GL_DEPTH_TEST);
    win->text.setColor({255,255,255,255});
    win->text.printText_Solid(font, 25.0f, 25.0f, "Press Enter or Tap to Start");
    if(stick.active()) {
        win->text.printText_Solid(font, 25.0f, 55.0f, "Controller: A/Start to Play, Back to Quit");
    }
    
}

void Intro::event(gl::GLWindow *win, SDL_Event &e) {

    if(stick.connectEvent(e)) {
        mx::system_out << "Connected.\n";
    }

    if((e.type == SDL_MOUSEBUTTONDOWN && e.button.button == 1) || (e.type == SDL_FINGERDOWN) || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_RETURN)) {
        win->setObject(new BreakoutGame());
        win->object->load(win);
        return;
    }

    if(e.type == SDL_CONTROLLERBUTTONDOWN) {
        if(e.cbutton.button == SDL_CONTROLLER_BUTTON_A || e.cbutton.button == SDL_CONTROLLER_BUTTON_START) {
            win->setObject(new BreakoutGame());
            win->object->load(win);
            return;
        }
        if(e.cbutton.button == SDL_CONTROLLER_BUTTON_BACK) {
            win->quit();
            return;
        }
    }
}
