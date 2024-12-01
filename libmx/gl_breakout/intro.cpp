#include"intro.hpp"
#include"game.hpp"

void Intro::load(gl::GLWindow  *win) {

    stick.open(0);

    if (!shaderProgram.loadProgram(win->util.getFilePath("data/tri.vert"), win->util.getFilePath("data/tri.frag"))) {
        throw mx::Exception("Failed to load shader program");
    }
    shaderProgram.useProgram();
    glm::mat4 model = glm::mat4(1.0f); 
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 2.0f),  
        glm::vec3(0.0f, 0.0f, 0.0f),  
        glm::vec3(0.0f, 1.0f, 0.0f)   
    );
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)win->w / win->h, 0.1f, 100.0f);

    shaderProgram.setUniform("model", model);
    shaderProgram.setUniform("view", view);
    shaderProgram.setUniform("projection", projection);
    
    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
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
    surface = flipSurface(surface);
    GLenum format = (surface->format->BytesPerPixel == 4) ? GL_RGBA : GL_RGB;

    glTexImage2D(GL_TEXTURE_2D, 0, format, surface->w, surface->h, 0, format, GL_UNSIGNED_BYTE, surface->pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    SDL_FreeSurface(surface);
}

void Intro::draw(gl::GLWindow *win) {
    shaderProgram.useProgram();
    CHECK_GL_ERROR();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glBindVertexArray(VAO);
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
    static float rotationAngle = 0.0f;
    rotationAngle += deltaTime * 50.0f; 
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(rotationAngle), glm::vec3(1.0f, 0.0f, 0.0f));
    shaderProgram.setUniform("model", model);      
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    if(stick.getButton(0)) {
        win->setObject(new BreakoutGame());
        win->object->load(win);
        return;
    }
}

void Intro::event(gl::GLWindow *win, SDL_Event &e) {
    if((e.type == SDL_MOUSEBUTTONDOWN && e.button.button == 1) || (e.type == SDL_FINGERDOWN) || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_RETURN)) {
        win->setObject(new BreakoutGame());
        win->object->load(win);
        return;
    }
}