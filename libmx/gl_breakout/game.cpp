#include"game.hpp"
#include"intro.hpp"

void BreakoutGame::loadTextures(gl::GLWindow *win) {
    for (int i = 0; i < 4; ++i) {
        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        std::string texturePath = win->util.getFilePath("data/texture" + std::to_string(i) + ".png");
        SDL_Surface *surface = png::LoadPNG(texturePath.c_str());
        if (!surface) {
            throw mx::Exception("failed to load texture");
        }
        GLenum format = (surface->format->BytesPerPixel == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, surface->w, surface->h, 0, format, GL_UNSIGNED_BYTE, surface->pixels);
        glGenerateMipmap(GL_TEXTURE_2D);
        SDL_FreeSurface(surface);
        Textures.push_back(textureID); 
    }
}

void BreakoutGame::load(gl::GLWindow *win) {

#ifdef WITH_MIXER
    if(ping == -1)
        ping = win->mixer.loadWav(win->util.getFilePath("data/ping.wav"));
    if(clear == -1)
        clear = win->mixer.loadWav(win->util.getFilePath("data/pop.wav"));
    if(die == -1)
        die = win->mixer.loadWav(win->util.getFilePath("data/die.wav"));
#endif

    font.loadFont(win->util.getFilePath("data/font.ttf"), 24);
    if(stick.open(0)) {
        mx::system_out << "Controller initalized: " << stick.name() << "\n";
    }

    loadTextures(win);
    if (!shaderProgram.loadProgram(win->util.getFilePath("data/tri.vert"), win->util.getFilePath("data/tri.frag"))) {
        throw mx::Exception("Failed to load shader program");
    }
    if (!textShader.loadProgram(win->util.getFilePath("data/text.vert"), win->util.getFilePath("data/text.frag"))) {
        throw mx::Exception("Failed to load shader program");
    }

    shaderProgram.useProgram();
    float zoom = 0.8f; 
    glm::mat4 projection = glm::ortho(-5.0f / zoom, 5.0f / zoom, -3.75f / zoom, 3.75f / zoom, -100.0f, 100.0f);
    shaderProgram.setUniform("projection", projection);
    glm::mat4 view = glm::mat4(1.0f); 
    shaderProgram.setUniform("view", view);
    PlayerPaddle.setShaderProgram(&shaderProgram);
    GameBall.setShaderProgram(&shaderProgram);
    PlayerPaddle.load(win, true);
    PlayerPaddle.Position = glm::vec3(0.0f, -3.5f, 0.0f);
    GameBall.load(win, true);
    GameBall.Position = PlayerPaddle.Position + glm::vec3(0.0f, 0.5f, 0.0f);
    GameBall.Velocity = glm::vec3(2.5f, 2.5f, 0.0f);
    Blocks.reserve(5 * 11); 

    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < 11; ++j) {
            Blocks.emplace_back(); 
            Block& block = Blocks.back();
            block.setShaderProgram(&shaderProgram);
            block.load(win, false);
            block.Position = glm::vec3(-5.0f + j * 1.0f, 3.0f - i * 0.6f, 0.0f);
            int randomTextureIndex = rand() % Textures.size();
            block.texture = Textures[randomTextureIndex];
            std::vector<GLuint> textures{block.texture};
            block.cube.setTextures(textures);
        }
    }

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
        sprite.loadTexture(&program, win->util.getFilePath("data/crystal1.png"), 0.0f, 0.0f, win->w, win->h);
}

void BreakoutGame::update(float deltaTime, gl::GLWindow *win) {
    const Uint8 *state = SDL_GetKeyboardState(NULL);
    const float axisThreshold = 8000.0f; 

    if (state[SDL_SCANCODE_W] || stick.getAxis(SDL_CONTROLLER_AXIS_RIGHTY) < -axisThreshold) {
        gridRotation -= rotationSpeed * deltaTime;
    }
    if (state[SDL_SCANCODE_S] || stick.getAxis(SDL_CONTROLLER_AXIS_RIGHTY) > axisThreshold) {
        gridRotation += rotationSpeed * deltaTime;
    }
    

    if (state[SDL_SCANCODE_A] || stick.getAxis(SDL_CONTROLLER_AXIS_RIGHTX) < -axisThreshold) {
        gridYRotation -= rotationSpeed * deltaTime;
    }
    if (state[SDL_SCANCODE_D] || stick.getAxis(SDL_CONTROLLER_AXIS_RIGHTX) > axisThreshold) {
        gridYRotation += rotationSpeed * deltaTime;
    }
    if (state[SDL_SCANCODE_Q] || stick.getButton(SDL_CONTROLLER_BUTTON_A)) {
        gridRotation = 0;
        gridYRotation = 0;
    }
/*
    if(state[SDL_SCANCODE_SPACE]) {
        mx::system_out << "X: " << gridRotation << " Y: " << gridYRotation << "\n";
    }
*/
    if(state[SDL_SCANCODE_1]) {
        gridRotation = -33.4;
        gridYRotation = -18.4;
    }
    if(state[SDL_SCANCODE_2]) {
        gridRotation = -21.4;
        gridYRotation = 31.15;
    }
    if(state[SDL_SCANCODE_3]) {
        gridRotation = -47.25;
        gridYRotation = 0.848669;        
    }

    if (gridRotation >= 360.0f) gridRotation -= 360.0f;
    if (gridRotation <= -360.0f) gridRotation += 360.0f;
    if (gridYRotation >= 360.0f) gridYRotation -= 360.0f;
    if (gridYRotation <= -360.0f) gridYRotation += 360.0f;

    PlayerPaddle.processInput(stick, state, deltaTime);

    if (state[SDL_SCANCODE_SPACE] || stick.getButton(SDL_CONTROLLER_BUTTON_B)) {
        GameBall.Stuck = false;
    }
    PlayerPaddle.update(deltaTime); 
    GameBall.move(deltaTime);
    for (auto &block : Blocks) {
        block.update(deltaTime);
    }
    doCollisions(win);
}
void BreakoutGame::draw(gl::GLWindow *win) {
    bool all_gone = true;
    for(auto &b : Blocks) {
        if(!b.Destroyed) {
            all_gone = false;
            break;
        }
    }
    if(all_gone == true) {
        win->setObject(new Intro());
        win->object->load(win);
        return;
    }

#ifdef __EMSCRIPTEN__
    static Uint32 lastTime = emscripten_get_now();
    float currentTime = emscripten_get_now();
#else
    static Uint32 lastTime = SDL_GetTicks();
    Uint32 currentTime = SDL_GetTicks();
#endif

    glDisable(GL_DEPTH_TEST);
    program.useProgram();
    program.setUniform("time_f", (float)currentTime / 1000.0f);
    program.setUniform("alpha", 0.7f);
    sprite.draw();

    float deltaTime = (currentTime - lastTime) / 1000.0f;
    if (deltaTime > 0.05f) deltaTime = 0.05f; 
    lastTime = currentTime;
    update(deltaTime, win);
    glEnable(GL_DEPTH_TEST);
    shaderProgram.useProgram();
    glm::mat4 modelView = glm::mat4(1.0f);
    modelView = glm::rotate(modelView, glm::radians(gridRotation), glm::vec3(1.0f, 0.0f, 0.0f));
    modelView = glm::rotate(modelView, glm::radians(gridYRotation), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::vec3 cameraPos(0.0f, 0.0f, 10.0f); 
    glm::vec3 lightPos(0.0f, 12.0f, 10.0f); 
    glm::vec3 viewPos = cameraPos;
    glm::vec3 lightColor(1.65f, 1.65f, 1.65f); 
    shaderProgram.setUniform("lightPos", lightPos);
    shaderProgram.setUniform("viewPos", viewPos);
    shaderProgram.setUniform("lightColor", lightColor);
    shaderProgram.setUniform("view", modelView);
       
    PlayerPaddle.draw(win);
    GameBall.draw(win);
    for (auto &block : Blocks) {
        if (!block.Destroyed) {
            block.draw(win);
        }
    }
    glBindVertexArray(0);
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

void BreakoutGame::doCollisions(gl::GLWindow *win) {
    if (checkCollision(GameBall, PlayerPaddle)) {
        float paddleCenter = PlayerPaddle.Position.x;
        float ballCenter = GameBall.Position.x;
        float distance = ballCenter - paddleCenter;
        float normalizedDistance = distance / (PlayerPaddle.Size.x / 2.0f);
        float speed = glm::length(GameBall.Velocity);
        GameBall.Velocity.x = normalizedDistance * speed;
        GameBall.Velocity.y = abs(GameBall.Velocity.y);
        GameBall.Position.y = PlayerPaddle.Position.y + PlayerPaddle.Size.y / 2.0f + GameBall.Radius;\
        PlayerPaddle.startRotation();
#ifdef WITH_MIXER
        win->mixer.playWav(ping, 0, 0);
#endif
    }

    for (Block &block : Blocks) {
        if (!block.Destroyed && checkCollision(GameBall, block)) {
            block.startRotation();
#ifdef WITH_MIXER
            if(!win->mixer.isPlaying(clear))
                win->mixer.playWav(clear, 0, 1);
#endif
            score += 10;
            GameBall.Velocity.y = -GameBall.Velocity.y;
            if (GameBall.Position.y > block.Position.y) {
                GameBall.Position.y += GameBall.Radius;
            } else {
                GameBall.Position.y -= GameBall.Radius;
            }
        }
    }

    if (GameBall.Position.y <= -3.75f) {
        resetGame();
#ifdef WITH_MIXER
        win->mixer.playWav(die, 0, 2);
#endif
    }
}

void BreakoutGame::resetGame() {
    GameBall.Position = PlayerPaddle.Position + glm::vec3(0.0f, 0.5f, 0.0f);
    GameBall.Velocity = glm::vec3(2.5f, 2.5f, 0.0f);
    GameBall.Stuck = true;
}

bool BreakoutGame::checkCollision(Ball &ball, GameObject &object) {
    glm::vec3 center(ball.Position);
    glm::vec3 aabb_half_extents(object.Size.x / 2.0f, object.Size.y / 2.0f, 0.0f);
    glm::vec3 aabb_center(
        object.Position.x,
        object.Position.y,
        0.0f
    );
    glm::vec3 difference = center - aabb_center;
    glm::vec3 clamped = glm::clamp(difference, -aabb_half_extents, aabb_half_extents);
    glm::vec3 closest = aabb_center + clamped;
    difference = closest - center;
    return glm::length(difference) < ball.Radius;
}

void BreakoutGame::event(gl::GLWindow *win, SDL_Event &e)  {
    
    if(stick.connectEvent(e)) {
        mx::system_out << "Controller " << stick.name() <<"\n";
    }
    
    if (e.type == SDL_FINGERDOWN) {
        Uint32 currentTapTime = SDL_GetTicks(); 

        if (currentTapTime - lastTapTime <= doubleTapThreshold) {
            if (GameBall.Stuck) {
                GameBall.Stuck = false; 
                std::cout << "Double-tap detected: Ball released!\n";
            }
        }
        lastTapTime = currentTapTime;
    } else if (e.type == SDL_FINGERMOTION) {
        float normalizedX = e.tfinger.x;
        float gameX = (normalizedX * 10.0f) - 5.0f;
        PlayerPaddle.handleTouch(gameX);
    }


}

GLuint BreakoutGame::createTextTexture(const std::string &text, TTF_Font *font, SDL_Color color, int &textWidth, int &textHeight) {
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
    
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        mx::system_err << "OpenGL Error: " << error << " while creating texture." << std::endl;
        SDL_FreeSurface(surface);
        return 0;
    }
    SDL_FreeSurface(surface);
    return texture;
}

void BreakoutGame::renderText(GLuint texture, int textWidth, int textHeight, int screenWidth, int screenHeight) {
    float x = -0.5f * (float)textWidth / screenWidth * 2.0f;
    float y = 1.0f;
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