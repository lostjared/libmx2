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
}

void Intro::draw(gl::GLWindow *win) {
    shaderProgram.useProgram();
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
    rotationAngle += 50.0f * deltaTime;
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::rotate(model, glm::radians(rotationAngle), glm::vec3(1.0f, 0.0f, 0.0f));
    shaderProgram.setUniform("model", model);
    glm::vec3 lightPos(0.0f, 3.0f, 2.0f); 
    glm::vec3 viewPos(0.0f, 0.0f, 10.0f);
    glm::vec3 lightColor(1.5f, 1.5f, 1.5f); 
    shaderProgram.setUniform("lightPos", lightPos);
    shaderProgram.setUniform("viewPos", viewPos);
    shaderProgram.setUniform("lightColor", lightColor);
    cube.drawArrays();
    glBindVertexArray(0);

    if(stick.getButton(SDL_CONTROLLER_BUTTON_A)) {
        win->setObject(new BreakoutGame());
        win->object->load(win);
        return;
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
}