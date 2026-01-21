#include"mx.hpp"
#include"argz.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <GLES3/gl3.h>
#endif

#include"gl.hpp"
#include"loadpng.hpp"
#include"model.hpp"

#define CHECK_GL_ERROR() \
    { GLenum err = glGetError(); \
      if (err != GL_NO_ERROR) \
        printf("OpenGL Error: %d at %s:%d\n", err, __FILE__, __LINE__); }
#if defined(__EMSCRIPTEN__) || defined(__ANDOIRD__)
const char *g_vSource = R"(#version 300 es
    precision highp float;
    layout (location = 0) in vec3 aPos;

    layout (location = 1) in vec3 aNormal;
    layout (location = 2) in vec2 aTexCoords;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    uniform vec3 lightPos;
    uniform vec3 viewPos;
    uniform vec3 lightColor;
    uniform vec3 objectColor;

    out vec3 vertexColor;
    out vec2 TexCoords;

    void main()
    {
        vec4 worldPos = model * vec4(aPos, 1.0);
        gl_Position = projection * view * worldPos;
        vec3 norm = normalize(mat3(transpose(inverse(model))) * aNormal);
        vec3 lightDir = normalize(lightPos - vec3(worldPos));
        float ambientStrength = 0.8; 
        vec3 ambient = ambientStrength * lightColor;
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lightColor;
        float specularStrength = 1.0;    // Increase from 0.5 to 1.0 for more shine
        float shininess = 64.0;          // Increase this value for a tighter, shinier highlight
        vec3 viewDir = normalize(viewPos - vec3(worldPos));
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
        vec3 specular = specularStrength * spec * lightColor;
        vec3 finalColor = (ambient + diffuse + specular) * objectColor;
        vertexColor = finalColor;
        TexCoords = aTexCoords;
    }

    )";
    const char *g_fSource = R"(#version 300 es
    precision highp float;
    in vec3 vertexColor;
    in vec2 TexCoords;
    uniform sampler2D texture1;
    uniform vec2 iResolution; // Screen resolution for aspect
    uniform float time_f;     // Time for animation
    out vec4 FragColor;

    vec4 distort4(vec2 tc, sampler2D samp) { 
    vec4 ctx;
    vec2 uv = tc * 2.0 - 1.0;
    uv *= iResolution.y / iResolution.x;

    vec2 center = vec2(0.0, 0.0);
    vec2 offset = uv - center;
    float dist = length(offset);
    float pulse = sin(time_f * 2.0 + dist * 10.0) * 0.15;
    float expansion = cos(time_f * 3.0 - dist * 15.0) * 0.2;
    float spiral = sin(atan(offset.y, offset.x) * 3.0 + time_f * 2.0) * 0.1;

    vec2 morphUV = uv + normalize(offset) * (pulse + expansion);
    morphUV += vec2(cos(atan(offset.y, offset.x)), sin(atan(offset.y, offset.x))) * spiral;

    vec4 texColor = texture(samp, morphUV * 0.5 + 0.5);
    texColor.rgb *= 1.0 + 0.3 * sin(dist * 20.0 - time_f * 5.0);

    ctx = vec4(texColor.rgb, texColor.a);
    return ctx;
    }

    void main() {
    FragColor = vec4(vertexColor, 1.0) * texture(texture1, TexCoords);
    vec4 ctx = distort4(TexCoords, texture1);
    FragColor = ctx * FragColor;
    FragColor.a = 1.0;
    }
    )";
    #else
    const char *g_vSource = R"(#version 330 core
        layout (location = 0) in vec3 aPos;

        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec2 aTexCoords;
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        uniform vec3 lightPos;
        uniform vec3 viewPos;
        uniform vec3 lightColor;
        uniform vec3 objectColor;

        out vec3 vertexColor;
        out vec2 TexCoords;

        void main()
        {
            vec4 worldPos = model * vec4(aPos, 1.0);
            gl_Position = projection * view * worldPos;
            vec3 norm = normalize(mat3(transpose(inverse(model))) * aNormal);
            vec3 lightDir = normalize(lightPos - vec3(worldPos));
            float ambientStrength = 0.8; 
            vec3 ambient = ambientStrength * lightColor;
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * lightColor;
            float specularStrength = 1.0;    
            float shininess = 64.0;          
            vec3 viewDir = normalize(viewPos - vec3(worldPos));
            vec3 reflectDir = reflect(-lightDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
            vec3 specular = specularStrength * spec * lightColor;
            vec3 finalColor = (ambient + diffuse + specular) * objectColor;
            vertexColor = finalColor;
            TexCoords = aTexCoords;
        }

    )";
    const char *g_fSource = R"(#version 330 core
in vec3 vertexColor;
in vec2 TexCoords;
uniform sampler2D texture1;
uniform vec2 iResolution; // Screen resolution for aspect
uniform float time_f;     // Time for animation
out vec4 FragColor;

vec4 distort4(vec2 tc, sampler2D samp) { 
    vec4 ctx;
    vec2 uv = tc * 2.0 - 1.0;
    uv *= iResolution.y / iResolution.x;

    vec2 center = vec2(0.0, 0.0);
    vec2 offset = uv - center;
    float dist = length(offset);
    float pulse = sin(time_f * 2.0 + dist * 10.0) * 0.15;
    float expansion = cos(time_f * 3.0 - dist * 15.0) * 0.2;
    float spiral = sin(atan(offset.y, offset.x) * 3.0 + time_f * 2.0) * 0.1;

    vec2 morphUV = uv + normalize(offset) * (pulse + expansion);
    morphUV += vec2(cos(atan(offset.y, offset.x)), sin(atan(offset.y, offset.x))) * spiral;

    vec4 texColor = texture(samp, morphUV * 0.5 + 0.5);
    texColor.rgb *= 1.0 + 0.3 * sin(dist * 20.0 - time_f * 5.0);

    ctx = vec4(texColor.rgb, texColor.a);
    return ctx;
}

void main() {
    FragColor = vec4(vertexColor, 1.0) * texture(texture1, TexCoords);
    vec4 ctx = distort4(TexCoords, texture1);
    FragColor = ctx * FragColor;
    FragColor.a = 1.0;
}
)";
#endif

class Cube : public gl::GLObject {
public:
    gl::ShaderProgram shaderProgram;
    GLuint VAO, VBO;
    GLuint texture;
    mx::Model cube;        
    mx::Font font;
    Cube() = default;
    virtual ~Cube() override {
    }

    std::vector<GLfloat> cubeData;

    virtual void load(gl::GLWindow *win) override {     
        font.loadFont(win->util.getFilePath("data/font.ttf"), 18);
        if(cube.openModel(win->util.getFilePath("data/cube.mxmod.z")) == false) {
            throw mx::Exception("Error loading model");
        }
        cube.saveOriginal();  
     
        if (!shaderProgram.loadProgramFromText(g_vSource, g_fSource)) {
            throw mx::Exception("Failed to load shader program");
        }
        shaderProgram.useProgram();
        cube.setTextures(win, win->util.getFilePath("data/cube.tex"), win->util.getFilePath("data"));
    }

    float twistAngle = 1.0f;
    float twistSpeed = 0.1f;
    bool twistX = false, twistY = false, twistZ = false;
    bool morphT = false;
    float uvScrollU = 0.0f;
    float uvScrollV = 0.0f;
    float hueShift = 0.0f;
    float saturation = 1.0f;
    float brightness = 1.0f;
    glm::vec3 colorGradingShift = glm::vec3(0.0f);
    bool enableUVScroll = false;
    bool enableColorGrading = false;
    bool enableHSV = false;
    float scrollIntensity = 0.3f;
    virtual void draw(gl::GLWindow *win) override {
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
        rotationAngle += deltaTime * 50.0f; 
        cube.setShaderProgram(&shaderProgram, "texture1");
        shaderProgram.useProgram();
        cameraPosition = glm::vec3(0.0f, 2.0f, 5.0f);
        glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);
        modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::scale(modelMatrix, glm::vec3(2.0f, 2.0f, 2.0f));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(rotationAngle), glm::vec3(rot_x, 1.0f, 0.0f));
        viewMatrix = glm::lookAt(cameraPosition, cameraTarget, upVector);
        float aspectRatio = static_cast<float>(win->w) / static_cast<float>(win->h);
        projectionMatrix = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);      
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram.id(), "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram.id(), "view"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram.id(), "projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
        glUniform3f(glGetUniformLocation(shaderProgram.id(), "lightPos"), lightPos.x, lightPos.y, lightPos.z);
        glUniform3f(glGetUniformLocation(shaderProgram.id(), "viewPos"), cameraPosition.x, cameraPosition.y, cameraPosition.z);
        glUniform3f(glGetUniformLocation(shaderProgram.id(), "lightColor"), 2.0f, 2.0f, 2.0f);
        glUniform3f(glGetUniformLocation(shaderProgram.id(), "objectColor"), 1.0f, 1.0f, 1.0f); 
        glUniform2f(glGetUniformLocation(shaderProgram.id(), "iResolution"), 
                    static_cast<float>(win->w), static_cast<float>(win->h));
        glUniform1f(glGetUniformLocation(shaderProgram.id(), "time_f"), 
                    static_cast<float>(currentTime) / 1000.0f);
        CHECK_GL_ERROR();
        glDisable(GL_BLEND);
        twistAngle += twistSpeed;
        cube.resetToOriginal();
        if (enableUVScroll) {
            static float scrollTime = 0.0f;
            scrollTime += deltaTime;
            float uOffset = sin(scrollTime * 0.5f) * scrollIntensity;
            float vOffset = cos(scrollTime * 0.3f) * scrollIntensity;
            cube.uvScroll(uOffset * deltaTime, vOffset * deltaTime);
        }
        
        if (enableColorGrading) {
            static float colorTime = 0.0f;
            colorTime += deltaTime;
            float colorCycle = sin(colorTime * 0.2f);
            colorGradingShift = glm::vec3(colorCycle * 0.3f, colorCycle * 0.2f, -colorCycle * 0.25f);
            cube.applyColorGrading(colorGradingShift);
        }
        
        if (enableHSV) {
            static float hsvTime = 0.0f;
            hsvTime += deltaTime;
            float satCycle = 0.5f + sin(hsvTime * 0.1f) * 0.5f;  
            cube.applyHSV(hueShift, satCycle, brightness);
        }
        
        if(twistY) cube.twist(mx::DeformAxis::Y, twistAngle);
        if(twistZ) cube.twist(mx::DeformAxis::Z, twistAngle);
        if(twistX) cube.twist(mx::DeformAxis::X, twistAngle);
        if(morphT) cube.bend(mx::DeformAxis::Y, twistAngle);
        
        cube.updateBuffers();
        cube.recalculateNormals();
        cube.drawArrays();
        win->text.setColor({255, 255, 255, 255});
        win->text.printText_Solid(font, 25.0f, 25.0f, "Press Arrow keys / Page Up Down to move light source Enter to reset");
        win->text.printText_Solid(font, 25.0f, 60.0f, "Press Space to rotate cube");
        win->text.printText_Solid(font, 25.0f, 95.0f, "U: UV Scroll  C: Color Grade  H: HSV  -/+: Adjust Intensity");
        win->text.printText_Solid(font, 25.0f, 130.0f, "X/Y/Z: Twist  T: Bend");
    }
  
    void event(gl::GLWindow *win, SDL_Event &e) override {
        if (e.type == SDL_KEYDOWN) {
            float moveSpeed = 0.05f; 
            switch (e.key.keysym.sym) {
                case SDLK_LEFT:
                    lightPos.x -= moveSpeed;
                    break;
                case SDLK_RIGHT:
                    lightPos.x += moveSpeed;
                    break;
                case SDLK_UP:
                    lightPos.z -= moveSpeed;
                    break;
                case SDLK_DOWN:
                    lightPos.z += moveSpeed;
                    break;
                case SDLK_PAGEUP:
                    lightPos.y += moveSpeed;
                    break;
                case SDLK_PAGEDOWN:
                    lightPos.y -= moveSpeed;
                    break;
                case SDLK_x:
                    twistX = !twistX;
                    break;
                case SDLK_y:
                    twistY = !twistY;
                    break;
                case SDLK_z:
                    twistZ = !twistZ;
                    break;
                case SDLK_t:
                    morphT = !morphT;
                    break;
                case SDLK_v:
                    twistSpeed += 0.1f;
                    std::cout << "Twist Increase: " << twistSpeed << std::endl;
                    break;
                case SDLK_b:
                    if(twistSpeed > 0.1f)
                        twistSpeed -= 0.1f;
                    std::cout << "Twist Decrease: " << twistSpeed << std::endl;

                case SDLK_RETURN:
                    lightPos = glm::vec3(0.0f, 5.0f, 0.0f);
                    break;
                case SDLK_SPACE:
                    if(rot_x >  0) rot_x = 0; else rot_x = 1.0f;
                    break;
                case SDLK_u:  
                    enableUVScroll = !enableUVScroll;
                    mx::system_out << "UV Scroll: " << (enableUVScroll ? "ON" : "OFF") << "\n";
                    break;
                case SDLK_c:  
                    enableColorGrading = !enableColorGrading;
                    mx::system_out << "Color Grading: " << (enableColorGrading ? "ON" : "OFF") << "\n";
                    break;
                case SDLK_h:  
                    enableHSV = !enableHSV;
                    mx::system_out << "HSV Adjustments: " << (enableHSV ? "ON" : "OFF") << "\n";
                    break;
                case SDLK_MINUS:
                    scrollIntensity = glm::max(0.0f, scrollIntensity - 0.01f);
                    mx::system_out << "Scroll Intensity: " << scrollIntensity << "\n";
                    break;
                case SDLK_EQUALS:
                    scrollIntensity += 0.01f;
                    mx::system_out << "Scroll Intensity: " << scrollIntensity << "\n";
                    break;
            }
            std::cout << "Light Position: " << lightPos.x << ", " << lightPos.y << ", " << lightPos.z << "\n";
        }
    }
private:
    glm::mat4 modelMatrix{1.0f};
    glm::mat4 viewMatrix{1.0f};
    glm::mat4 projectionMatrix{1.0f};
    glm::vec3 cameraPosition{0.0f, 2.0f, 5.0f};
    glm::vec3 lightPos{0.0f, 5.0f, 0.0f};
    float rot_x =  1.0f;
};

class MainWindow : public gl::GLWindow {
public:
    MainWindow(std::string path, int tw, int th) : gl::GLWindow("Transparent Rotating Cube", tw, th) {
        setPath(path);
        setObject(new Cube());
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
    }
private:
};

MainWindow *main_w = nullptr;

void eventProc() {
    main_w->proc();
}

int main(int argc, char **argv) {
#ifdef __EMSCRIPTEN__
    try {
        MainWindow main_window("", 1920, 1080);
        main_w = &main_window;
        emscripten_set_main_loop(eventProc, 0, 1);
    } catch(const mx::Exception &e) {
        mx::system_err << "mx: Exception: " << e.text() << "\n";
        mx::system_err.flush();
        exit(EXIT_FAILURE);
    }   
#else
    Arguments args = proc_args(argc, argv);
    try {
        MainWindow main_window(args.path, args.width, args.height);
        main_window.setFullScreen(args.fullscreen);
        main_window.loop();
    } catch(const mx::Exception &e) {
        mx::system_err << "mx: Exception: " << e.text() << "\n";
        mx::system_err.flush();
        exit(EXIT_FAILURE);
    } 
#endif
    return 0;
}