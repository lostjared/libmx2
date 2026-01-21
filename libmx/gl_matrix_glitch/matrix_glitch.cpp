#include"mx.hpp"
#include"argz.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <GLES3/gl3.h>
#endif
#include<random>
#include"gl.hpp"
#include"loadpng.hpp"
#include"model.hpp"
#define CHECK_GL_ERROR() \
{ GLenum err = glGetError(); \
if (err != GL_NO_ERROR) \
printf("OpenGL Error: %d at %s:%d\n", err, __FILE__, __LINE__); }

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include<list>


#ifdef __EMSCRIPTEN__
const char *vSource = R"(#version 300 es
precision highp float;

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char *fSource = R"(#version 300 es
precision highp float;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform sampler2D texture1;
uniform float time;
uniform vec3 xorColor;

out vec4 FragColor;


void main() {

    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, lightDir), 0.0);
    

    vec2 animatedTexCoord = TexCoord;
    animatedTexCoord.y += sin(time * 0.5 + TexCoord.x * 10.0) * 0.02;
    

    vec4 texColor = texture(texture1, animatedTexCoord);
    
    if(texColor.r < 0.1 && texColor.g < 0.1 && texColor.b < 0.1) {
        discard;
    }

    FragColor = vec4(sin(xorColor * time), 1.0);
}
)";
#else
const char *vSource = R"(#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char *fSource = R"(#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform sampler2D texture1;
uniform float time;
uniform vec3 xorColor;

out vec4 FragColor;


void main() {

    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, lightDir), 0.0);
    

    vec2 animatedTexCoord = TexCoord;
    animatedTexCoord.y += sin(time * 0.5 + TexCoord.x * 10.0) * 0.02;
    

    vec4 texColor = texture(texture1, animatedTexCoord);
    
    if(texColor.r < 0.1 && texColor.g < 0.1 && texColor.b < 0.1) {
        discard;
    }

    FragColor = vec4(sin(xorColor * time), 1.0);
}
)";
#endif

float generateRandomFloat(float min, float max) {
    static std::random_device rd; 
    static std::default_random_engine eng(rd()); 
    std::uniform_real_distribution<float> dist(min, max);
    return dist(eng);
}

class Matrix {

public:
    mx::Font the_font;
    std::unordered_map<std::string, SDL_Surface*> char_textures;
    std::list<std::string> cache_access_order;
    const size_t MAX_CACHE_SIZE = 1000;
    Matrix() = default;
    ~Matrix() {
        for(auto &it : char_textures) {
            SDL_FreeSurface(it.second);
        }
    }       

    void load(gl::GLWindow *win) {
        the_font.loadFont(win->util.getFilePath("data/keifont.ttf"), 36);
    }

    std::string unicodeToUTF8(int codepoint) {
        std::string utf8;
        if (codepoint <= 0x7F) {
            utf8 += static_cast<char>(codepoint);
        } else if (codepoint <= 0x7FF) {
            utf8 += static_cast<char>((codepoint >> 6) | 0xC0);
            utf8 += static_cast<char>((codepoint & 0x3F) | 0x80);
        } else if (codepoint <= 0xFFFF) {
            utf8 += static_cast<char>((codepoint >> 12) | 0xE0);
            utf8 += static_cast<char>(((codepoint >> 6) & 0x3F) | 0x80);
            utf8 += static_cast<char>((codepoint & 0x3F) | 0x80);
        } else if (codepoint <= 0x10FFFF) {
            utf8 += static_cast<char>((codepoint >> 18) | 0xF0);
            utf8 += static_cast<char>(((codepoint >> 12) & 0x3F) | 0x80);
            utf8 += static_cast<char>(((codepoint >> 6) & 0x3F) | 0x80);
            utf8 += static_cast<char>((codepoint & 0x3F) | 0x80);
        }
        return utf8;
    }

    std::vector<std::pair<int, int>> codepoint_ranges = {
        {0x3041, 0x3096},   
        {0x30A0, 0x30FF}
    };

    int getRandomCodepoint() {
        int range_index = rand() % codepoint_ranges.size();
        int start = codepoint_ranges[range_index].first;
        int end = codepoint_ranges[range_index].second;
        return start + rand() % (end - start + 1);
    }

    SDL_Color computeTrailColor(int trail_offset, int trail_length) {
        float intensity = 1.0f - (float)trail_offset / (float)trail_length;
        if (intensity < 0.0f) intensity = 0.0f;

        Uint8 alpha = static_cast<Uint8>(255 * intensity);
        if (alpha < 50) alpha = 50;

        Uint8 green = static_cast<Uint8>(255 * intensity);
        if (green < 100) green = 100; 
        SDL_Color color = {0, green, 0, alpha};
        return color;
    }

    SDL_Surface* createMatrixRain(TTF_Font* font, int screen_width, int screen_height) {
        SDL_Surface* matrix_surface = SDL_CreateRGBSurfaceWithFormat(
            0, screen_width, screen_height, 32, SDL_PIXELFORMAT_RGBA32);
        
        if (!matrix_surface) {
            printf("Failed to create surface: %s\n", SDL_GetError());
            return nullptr;
        }
        
        SDL_FillRect(matrix_surface, NULL, SDL_MapRGBA(matrix_surface->format, 0, 0, 0, 50));
        
        int char_width = 0;
        int char_height = 0;
        TTF_SizeUTF8(font, "A", &char_width, &char_height);
        int num_columns = screen_width / char_width + 1;
        int num_rows = screen_height / char_height + 1;
        
        static std::vector<float> fall_positions(num_columns, 0.0f);
        static std::vector<int> fall_speeds(num_columns, 0);
        static std::vector<int> trail_lengths(num_columns, 0);
        static Uint32 last_time = 0;
        float speed_multiplier = 2.0f;
        
        if (fall_positions[0] == 0.0f) {
            for (int col = 0; col < num_columns; ++col) {
                fall_positions[col] = static_cast<float>(rand() % num_rows);
                fall_speeds[col] = (rand() % 7 + 3) * speed_multiplier;
                trail_lengths[col] = rand() % 15 + 5;
            }
            last_time = SDL_GetTicks();
        }
        
        Uint32 current_time = SDL_GetTicks();
        float delta_time = (current_time - last_time) / 1000.0f;
        last_time = current_time;
        std::unordered_map<std::string, SDL_Surface*>& char_surfaces = char_textures;
        
        for (int col = 0; col < num_columns; ++col) {
            fall_positions[col] += fall_speeds[col] * delta_time;
            if (fall_positions[col] >= num_rows) {
                fall_positions[col] -= num_rows;
                trail_lengths[col] = rand() % 15 + 5;
                fall_speeds[col] = (rand() % 7 + 3) * speed_multiplier;
            }
            
            for (int trail_offset = 0; trail_offset < trail_lengths[col]; ++trail_offset) {
                int row = static_cast<int>(fall_positions[col] - trail_offset + num_rows) % num_rows;
                
                int random_char_code = getRandomCodepoint();
                std::string random_char = unicodeToUTF8(random_char_code);
                SDL_Color color = computeTrailColor(trail_offset, trail_lengths[col]);
                SDL_Surface* char_surface = nullptr;
                std::string cache_key = random_char + "_" + 
                                       std::to_string(color.r) + "_" + 
                                       std::to_string(color.g) + "_" + 
                                       std::to_string(color.b) + "_" + 
                                       std::to_string(color.a);
                
                if (char_surfaces.find(cache_key) == char_surfaces.end()) {
                    if (char_surfaces.size() >= MAX_CACHE_SIZE && !cache_access_order.empty()) {
                        std::string oldest_key = cache_access_order.front();
                        cache_access_order.pop_front();
                        SDL_FreeSurface(char_surfaces[oldest_key]);
                        char_surfaces.erase(oldest_key);
                    }
                    
                    char_surface = TTF_RenderUTF8_Blended(font, random_char.c_str(), color);
                    if (char_surface) {
                        char_surfaces[cache_key] = char_surface;
                        cache_access_order.push_back(cache_key);
                    } else {
                        continue; 
                    }
                } else {
                    auto it = std::find(cache_access_order.begin(), cache_access_order.end(), cache_key);
                    if (it != cache_access_order.end()) {
                        cache_access_order.erase(it);
                    }
                    cache_access_order.push_back(cache_key);
                    char_surface = char_surfaces[cache_key];
                }
                
                SDL_Rect dst_rect = {col * char_width, row * char_height, char_width, char_height};
                SDL_BlitSurface(char_surface, nullptr, matrix_surface, &dst_rect);
            }
        }
        
        return matrix_surface;
    }
};
    

class Game : public gl::GLObject {
public:
    Matrix matrix;
    Game() = default;
    virtual ~Game() override {

        if(texture != 0) {
            glDeleteTextures(1, &texture);
        }

    }

    GLuint texture = 0;

    void load(gl::GLWindow *win) override {
        font.loadFont(win->util.getFilePath("data/font.ttf"), 24);
        if(!shader_program.loadProgramFromText(vSource, fSource)) {
            throw mx::Exception("Failed to load shader program");
        }
        if(!cube.openModel(win->util.getFilePath("data/cube.mxmod.z"))) {
            throw mx::Exception("Failed to load model");
        }
        cube.saveOriginal();  
        cube.setShaderProgram(&shader_program, "texture1");
        shader_program.useProgram();
        matrix.load(win);
        SDL_Surface *surf = matrix.createMatrixRain(matrix.the_font.wrapper().unwrap(), 1440, 1080); 
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surf->w, surf->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surf->pixels);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        SDL_FreeSurface(surf);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }

    void draw(gl::GLWindow *win) override {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; 
        lastUpdateTime = currentTime;
        
        update(deltaTime);
        
        
        deformTime += deltaTime;
        wavePhase += deltaTime * 4.0f;  
        ripplePhase += deltaTime * 5.0f;  

        pulseScale = 1.0f + 0.35f * std::sin(deformTime * 2.0f);  
        cube.resetToOriginal();
        cube.scale(pulseScale);
        cube.wave(mx::DeformAxis::Y, 0.4f * std::sin(deformTime * 1.2f), 3.0f, wavePhase);
        cube.wave(mx::DeformAxis::X, 0.3f * std::cos(deformTime * 0.9f), 2.5f, wavePhase * 0.7f);
        cube.twist(mx::DeformAxis::Y, 0.8f * std::sin(deformTime * 0.6f), 0.0f);
        cube.ripple(0.25f * std::sin(deformTime * 1.5f), 2.0f, ripplePhase);
        cube.bulge(0.3f * std::sin(deformTime * 1.8f), 0.0f, 0.0f, 0.0f, 2.0f);
        cube.recalculateNormals();
        cube.updateBuffers();
        SDL_Surface *matrix_surface = matrix.createMatrixRain(matrix.the_font.wrapper().unwrap(), 1440, 1080);
        mx::Texture::flipSurface(matrix_surface);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, matrix_surface->w, matrix_surface->h, GL_RGBA, GL_UNSIGNED_BYTE, matrix_surface->pixels);
        glGenerateMipmap(GL_TEXTURE_2D);
        SDL_FreeSurface(matrix_surface);
        if (!insideCube) {
            rotation_x += deltaTime * 15.0f; 
            rotation_y += deltaTime * 20.0f;
        }
        
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(4.0f, 4.0f, 4.0f));
        model = glm::rotate(model, glm::radians(rotation_y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotation_x), glm::vec3(1.0f, 0.0f, 0.0f));
        
        glm::mat4 view;
        glm::vec3 viewPos;
        float nearPlane = 0.01f; 
        
        if (insideCube) {
            viewPos = glm::vec3(0.0f, 0.0f, 0.0f);
            nearPlane = 0.05f;
            glm::vec3 direction;
            direction.x = cos(glm::radians(cameraPitch)) * cos(glm::radians(cameraYaw));
            direction.y = sin(glm::radians(cameraPitch));
            direction.z = cos(glm::radians(cameraPitch)) * sin(glm::radians(cameraYaw));
            glCullFace(GL_FRONT);
            view = glm::lookAt(
                viewPos,
                viewPos + glm::normalize(direction),
                glm::vec3(0.0f, 1.0f, 0.0f)
            );
        } else {
            viewPos = glm::vec3(0.0f, 0.0f, 12.0f); 
            glCullFace(GL_BACK);
            view = glm::lookAt(
                viewPos,
                glm::vec3(0.0f, 0.0f, 0.0f),
                glm::vec3(0.0f, 1.0f, 0.0f)
            );
        }
        float aspectRatio = static_cast<float>(win->w) / static_cast<float>(win->h);
        glm::mat4 projection = glm::perspective(
            glm::radians(insideCube ? 90.0f : 45.0f), 
            aspectRatio, 
            nearPlane,  
            400.0f  
        );
        
        shader_program.useProgram();
        shader_program.setUniform("model", model);
        shader_program.setUniform("view", view);
        shader_program.setUniform("projection", projection);
        shader_program.setUniform("texture1", 0);
        shader_program.setUniform("time", static_cast<float>(currentTime) / 1000.0f);
        //shader_program.setUniform("viewPos", viewPos);
        shader_program.setUniform("xorColor", glm::vec3(generateRandomFloat(0.4f, 1.5f), generateRandomFloat(0.4f, 1.5f), generateRandomFloat(0.4f, 1.5f)));
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        cube.drawArraysWithTexture(texture, "texture1");
    
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_CULL_FACE);

        win->text.setColor({255, 255, 255, 255});
        if(menu_shown && insideCube == false) {
            win->text.printText_Solid(font, 25.0f, 25.0f, "Matrix Cube - FPS: " + std::to_string(int(1.0f/deltaTime)));        
            std::string cameraMode = insideCube ? "Inside" : "Outside";
            win->text.printText_Solid(font, 25.0f, 60.0f, "Press ENTER to toggle " + cameraMode + " mode");
            win->text.printText_Solid(font, 25.0f, 95.0f, "Use Arrow Keys to look around");
        }
    }
    
    void event(gl::GLWindow *win, SDL_Event &e) override {
        if (e.type == SDL_KEYDOWN) {
            switch(e.key.keysym.sym) {
                case SDLK_RETURN: 
                    insideCube = !insideCube;
                    if (insideCube) {
                        cameraPitch = 0.0f;
                        cameraYaw = 0.0f;
                    }
                    break;
                case SDLK_SPACE:
                    menu_shown = !menu_shown;
                    break;
            }
        }  else if (e.type == SDL_FINGERDOWN) {
            touchActive = true;
            lastTouchX = static_cast<int>(e.tfinger.x * win->w);
            lastTouchY = static_cast<int>(e.tfinger.y * win->h);
            
            int tapX = static_cast<int>(e.tfinger.x * win->w);
            int tapY = static_cast<int>(e.tfinger.y * win->h);
            Uint32 currentTime = SDL_GetTicks();
            
            int dx = tapX - lastTapX;
            int dy = tapY - lastTapY;
            int distanceSquared = dx*dx + dy*dy;
            
            if (static_cast<int>(currentTime - lastTapTime) < DOUBLE_TAP_TIME && 
                distanceSquared < DOUBLE_TAP_DISTANCE * DOUBLE_TAP_DISTANCE) {
                
                insideCube = !insideCube;
                if (insideCube) {
                    cameraPitch = 0.0f;
                    cameraYaw = 0.0f;
                }
            }
            
            lastTapTime = currentTime;
            lastTapX = tapX;
            lastTapY = tapY;
        }
        else if (e.type == SDL_FINGERUP) {
            touchActive = false;
        }
        else if (e.type == SDL_FINGERMOTION && touchActive) {
            if (insideCube) {
                
                int touchX = static_cast<int>(e.tfinger.x * win->w);
                int touchY = static_cast<int>(e.tfinger.y * win->h);
                int dx = touchX - lastTouchX;
                int dy = touchY - lastTouchY;
                float touchSensitivity = 0.2f;
                cameraYaw += dx * touchSensitivity;
                cameraPitch -= dy * touchSensitivity; 
                if (cameraPitch > 89.0f) cameraPitch = 89.0f;
                if (cameraPitch < -89.0f) cameraPitch = -89.0f;       
                if (cameraYaw < 0.0f) cameraYaw += 360.0f;
                if (cameraYaw > 360.0f) cameraYaw -= 360.0f;
                lastTouchX = touchX;
                lastTouchY = touchY;
            }
        }
    }
    void update(float deltaTime) {
        const Uint8* keyState = SDL_GetKeyboardState(NULL);
        if (insideCube) {
            float rotationSpeed = 100.0f * deltaTime;
            
            if (keyState[SDL_SCANCODE_LEFT]) {
                cameraYaw -= rotationSpeed;
                if (cameraYaw < 0.0f) 
                    cameraYaw += 360.0f;
            }
            
            if (keyState[SDL_SCANCODE_RIGHT]) {
                cameraYaw += rotationSpeed;
                if (cameraYaw > 360.0f)
                    cameraYaw -= 360.0f;
            }
            
            if (keyState[SDL_SCANCODE_UP]) {
                cameraPitch += rotationSpeed;
                if (cameraPitch > 89.0f)
                    cameraPitch = 89.0f;
            }
            
            if (keyState[SDL_SCANCODE_DOWN]) {
                cameraPitch -= rotationSpeed;
                if (cameraPitch < -89.0f)
                    cameraPitch = -89.0f;
            }
        }
    }
private:
    mx::Font font;
    Uint32 lastUpdateTime = SDL_GetTicks();
    mx::Model cube;
    gl::ShaderProgram shader_program;
    float rotation_x = 0.0f;
    float rotation_y = 0.0f;
    bool insideCube = false;
    float cameraYaw = 0.0f;
    float cameraPitch = 0.0f;
    glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 3.0f);
    bool touchActive = false;
    int lastTouchX = 0;
    int lastTouchY = 0;
    Uint32 lastTapTime = 0;
    int lastTapX = 0;
    int lastTapY = 0;
    const int DOUBLE_TAP_TIME = 300; 
    const int DOUBLE_TAP_DISTANCE = 30; 
    bool menu_shown = true;
    
    // Deformation state
    float deformTime = 0.0f;
    float wavePhase = 0.0f;
    float ripplePhase = 0.0f;
    float pulseScale = 1.0f;
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("Enter the Matrix", tw, th) {
        setPath(path);
        setObject(new Game());
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
    MainWindow main_window("", 1920, 1080);
    main_w =&main_window;
    emscripten_set_main_loop(eventProc, 0, 1);
    } catch(const mx::Exception &e) {
        std::cerr << "Error: " << e.text() << "\n";
        exit(EXIT_FAILURE);
    }
#else
    Arguments args = proc_args(argc, argv);
    try {
        MainWindow main_window(args.path, args.width, args.height);
        main_window.loop();
    } catch(const mx::Exception &e) {
        mx::system_err << "mx: Exception: " << e.text() << "\n";
        mx::system_err.flush();
        exit(EXIT_FAILURE);
    }
#endif
    return 0;
}
