/**
 * @file gl.hpp
 * @brief OpenGL window, shader program, sprite, and text rendering classes.
 *
 * Provides the core OpenGL abstractions used by libmx applications:
 * - ShaderProgram  – GLSL vertex/fragment shader compilation and uniform upload.
 * - GLText         – SDL_ttf text rendered into an OpenGL texture.
 * - GLSprite       – A textured quad rendered with an arbitrary shader.
 * - GLWindow       – SDL2 + OpenGL/ES window and event loop.
 * - GLObject       – Abstract interface for scene objects attached to GLWindow.
 *
 * Requires the WITH_GL compile-time flag.
 */
#ifndef GL_H__
#define GL_H__

#ifdef __EMSCRIPTEN__
#include"config.hpp"
#else
#include"config.h"
#endif

#ifdef WITH_GL
#include "mx.hpp"
#include<string>
#include<memory>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <GLES3/gl3.h>
#include<emscripten/html5.h>
#include "glm.hpp"
#include "gtc/type_ptr.hpp"
#else
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#endif

#include"console.hpp"

namespace gl {

    extern const char *vSource; ///< Default vertex shader source.
    extern const char *fSource; ///< Default fragment shader source.

/**
 * @class ShaderProgram
 * @brief GLSL shader program wrapper (shared-ownership).
 *
 * Compiles a vertex and fragment shader, links them into a GL program, and
 * exposes uniform-upload helpers.  Uses shared_ptr internally so multiple
 * copies refer to the same underlying GL object; the program is deleted when
 * the last copy is destroyed.
 */
    class ShaderProgram {
    private:
        struct SharedState {
            GLuint vertex_shader = 0;
            GLuint fragment_shader = 0;
            GLuint shader_id = 0;
            std::string name_;
            bool silent_ = false;
            
            ~SharedState();  
        };
        std::shared_ptr<SharedState> state;
    public:
        /** @brief Default constructor — allocates shared state, no program compiled yet. */
        ShaderProgram();

        /**
         * @brief Wrap an existing GL program ID.
         * @param id Pre-compiled GL program identifier.
         */
        ShaderProgram(GLuint id);
        ~ShaderProgram() = default;  
        ShaderProgram(const ShaderProgram&) = default;
        ShaderProgram& operator=(const ShaderProgram&) = default;    
        ShaderProgram(ShaderProgram&&) noexcept = default;
        ShaderProgram& operator=(ShaderProgram&&) noexcept = default;

        /** @brief Release the underlying GL program (decrements shared count). */
        void release();

        /** @return The GL program ID, or 0 if not loaded. */
        GLuint id() const;

        /** @return @c true if the program has been successfully linked. */
        bool loaded() const;

        /** @brief Bind this shader program for subsequent draw calls. */
        void useProgram();

        /**
         * @brief Assign a human-readable name used in error messages.
         * @param n Name string.
         */
        void setName(const std::string &n);

        /**
         * @brief Suppress compilation/link error output.
         * @param s If @c true, errors are not printed.
         */
        void setSilent(bool s);

        /**
         * @brief Compile and link shaders from file paths.
         * @param v Path to the GLSL vertex shader file.
         * @param f Path to the GLSL fragment shader file.
         * @return @c true on success.
         */
        bool loadProgram(const std::string &v, const std::string &f);

        /**
         * @brief Compile and link shaders from source-code strings.
         * @param v Vertex shader GLSL source.
         * @param f Fragment shader GLSL source.
         * @return @c true on success.
         */
        bool loadProgramFromText(const std::string &v, const std::string &f);

        /**
         * @brief Upload an integer uniform variable.
         * @param name  Uniform name in the shader.
         * @param value Value to upload.
         */
        void setUniform(const std::string &name, int value);

        /**
         * @brief Upload a float uniform variable.
         * @param name  Uniform name.
         * @param value Float value.
         */
        void setUniform(const std::string &name, float value);

        /**
         * @brief Upload a vec2 uniform.
         * @param name  Uniform name.
         * @param value glm::vec2 value.
         */
        void setUniform(const std::string &name, const glm::vec2 &value);

        /**
         * @brief Upload a vec3 uniform.
         * @param name  Uniform name.
         * @param value glm::vec3 value.
         */
        void setUniform(const std::string &name, const glm::vec3 &value);

        /**
         * @brief Upload a vec4 uniform.
         * @param name  Uniform name.
         * @param value glm::vec4 value.
         */
        void setUniform(const std::string &name, const glm::vec4 &value);

        /**
         * @brief Upload a mat4 uniform.
         * @param name  Uniform name.
         * @param value glm::mat4 value.
         */
        void setUniform(const std::string &name, const glm::mat4 &value);
    private:
        GLuint createProgram(const char *vshaderSource, const char *fshaderSource);
        GLuint createProgramFromFile(const std::string &vert, const std::string &frag);
        int printShaderLog(GLuint shader);
        void printProgramLog(int p);
        bool checkError();
    };
    
/**
 * @class GLText
 * @brief Render SDL_ttf text into an OpenGL texture and display it on screen.
 *
 * Manages a simple text shader and provides helpers that rasterise a string
 * to an SDL_Surface, upload it as an OpenGL texture, and draw a screen-space
 * quad at the specified position.
 */
    class GLText {
    public:
        /** @brief Default constructor. */
        GLText();

        /**
         * @brief Initialise internal state with the viewport dimensions.
         * @param w Viewport width in pixels.
         * @param h Viewport height in pixels.
         */
        void init(int w, int h);

        /**
         * @brief Rasterise a string to an OpenGL texture.
         * @param text        String to render.
         * @param font        SDL_ttf font handle.
         * @param color       Text colour.
         * @param textWidth   Output: texture pixel width.
         * @param textHeight  Output: texture pixel height.
         * @param solid       If @c true, use solid (fast) rendering; otherwise blended.
         * @return GL texture ID.
         */
        GLuint createText(const std::string &text, TTF_Font *font, SDL_Color color, int &textWidth, int &textHeight, bool solid = true);

        /**
         * @brief Draw a pre-created text texture at a given position.
         * @param texture      GL texture ID from createText().
         * @param x            Destination X (pixels).
         * @param y            Destination Y (pixels).
         * @param textWidth    Texture pixel width.
         * @param textHeight   Texture pixel height.
         * @param screenWidth  Current viewport width.
         * @param screenHeight Current viewport height.
         */
        void renderText(GLuint texture, float x, float y, int textWidth, int textHeight, int screenWidth, int screenHeight);

        /**
         * @brief Convenience: create and immediately draw solid text.
         * @param f    mx::Font reference.
         * @param x    X position.
         * @param y    Y position.
         * @param text String to render.
         */
        void printText_Solid(const mx::Font &f, float x, float y, const std::string &text);

        /**
         * @brief Convenience: create and immediately draw blended (anti-aliased) text.
         * @param f    mx::Font reference.
         * @param x    X position.
         * @param y    Y position.
         * @param text String to render.
         */
        void printText_Blended(const mx::Font &f, float x, float y, const std::string &text);

        /**
         * @brief Set the colour used for subsequent printText calls.
         * @param col SDL_Color.
         */
        void setColor(SDL_Color col);
    private:
        ShaderProgram textShader; ///< Internal text quad shader.
        SDL_Color color = {255,255,255,255}; ///< Current text colour.
        int w = 0, h = 0; ///< Viewport dimensions.
    };

/**
 * @class GLSprite
 * @brief A textured screen-space quad drawn with an OpenGL shader.
 *
 * Manages a VAO/VBO pair that holds the quad geometry and a GL texture.
 * Supports loading from file, drawing at arbitrary positions/sizes, and
 * updating the texture from an SDL_Surface or raw pixel buffer at runtime.
 * Copy and move are disabled; create on the heap or as a member.
 */
    class GLSprite {
    public:
        /** @brief Default constructor. */
        GLSprite();
        /** @brief Destructor — releases GL buffers and texture. */
        ~GLSprite();

        GLSprite(const GLSprite&) = delete;
        GLSprite& operator=(const GLSprite&) = delete;
        GLSprite(GLSprite&&) = delete;
        GLSprite& operator=(GLSprite&&) = delete;

        /** @brief Free all GL resources associated with this sprite. */
        void release();

        /**
         * @brief Set the native display size of the sprite.
         * @param w Width in pixels.
         * @param h Height in pixels.
         */
        void initSize(float w, float h);

        /**
         * @brief Assign a debug name (used in log messages).
         * @param name Name string.
         */
        void setName(const std::string &name);

        /**
         * @brief Bind a shader program by extracting its GL ID.
         * @tparam ShaderT Any type exposing id() -> GLuint.
         * @param program  Pointer to the shader (nullptr clears the binding).
         */
        template<typename ShaderT>
        void setShader(ShaderT *program) {
            active_shader_id = program ? program->id() : 0;
        }

        /**
         * @brief Initialise the sprite from an existing GL texture.
         * @tparam ShaderT    Shader type.
         * @param program     Active shader.
         * @param texture     Existing GL texture ID.
         * @param x           Destination X.
         * @param y           Destination Y.
         * @param textWidth   Texture width.
         * @param textHeight  Texture height.
         */
        template<typename ShaderT>
        void initWithTexture(ShaderT *program, GLuint texture, float x, float y, int textWidth, int textHeight) {
            active_shader_id = program ? program->id() : 0;
            initWithTextureImpl(texture, x, y, textWidth, textHeight);
        }

        /**
         * @brief Load a texture from a file and set the sprite geometry.
         * @tparam ShaderT    Shader type.
         * @param shader      Active shader.
         * @param tex         Image file path.
         * @param x           Display X.
         * @param y           Display Y.
         * @param textWidth   Display width.
         * @param textHeight  Display height.
         */
        template<typename ShaderT>
        void loadTexture(ShaderT *shader, const std::string &tex, float x, float y, int textWidth, int textHeight) {
            active_shader_id = shader ? shader->id() : 0;
            loadTextureImpl(tex, x, y, textWidth, textHeight);
        }

        /**
         * @brief Load a texture from a file (natural dimensions).
         * @tparam ShaderT Shader type.
         * @param shader   Active shader.
         * @param tex      Image file path.
         * @param x        Display X.
         * @param y        Display Y.
         */
        template<typename ShaderT>
        void loadTexture(ShaderT *shader, const std::string &tex, float x, float y) {
            active_shader_id = shader ? shader->id() : 0;
            loadTextureImpl(tex, x, y);
        }

        /** @brief Draw at the sprite's stored position and size. */
        void draw();

        /**
         * @brief Draw at the given position (stored size).
         * @param x Destination X.
         * @param y Destination Y.
         */
        void draw(int x, int y);

        /**
         * @brief Draw at an explicit position and size.
         * @param x Destination X.
         * @param y Destination Y.
         * @param w Display width.
         * @param h Display height.
         */
        void draw(int x, int y, int w, int h);

        /**
         * @brief Draw an arbitrary texture at a position and size.
         * @param texture_id External GL texture ID.
         * @param x          Destination X.
         * @param y          Destination Y.
         * @param w          Display width.
         * @param h          Display height.
         */
        void draw(GLuint texture_id, float x, float y, int w, int h);

        /**
         * @brief Replace the sprite texture with the contents of an SDL_Surface.
         * @param surf New surface (not consumed; caller still owns it).
         */
        void updateTexture(SDL_Surface *surf);

        /**
         * @brief Replace the sprite texture from a raw pixel buffer.
         * @param buffer Pointer to RGBA pixel data.
         * @param width  Buffer width in pixels.
         * @param height Buffer height in pixels.
         */
        void updateTexture(void *buffer, int width, int height);

        int width = 0;  ///< Texture pixel width.
        int height = 0; ///< Texture pixel height.
    private:
        void initWithTextureImpl(GLuint texture, float x, float y, int textWidth, int textHeight);
        void loadTextureImpl(const std::string &tex, float x, float y, int textWidth, int textHeight);
        void loadTextureImpl(const std::string &tex, float x, float y);
        GLuint active_shader_id = 0;
        GLuint texture = 0;
        GLuint VBO = 0, VAO = 0;
        std::vector<float> vertices;
        float screenWidth = 0.0f, screenHeight = 0.0f;
        int texWidth = 0, texHeight = 0;
        std::string textureName;
    };

    /** @brief OpenGL mode: desktop GL or OpenGL ES. */
    enum class GLMode  { DESKTOP, ES };

    class GLObject;

/**
 * @class GLWindow
 * @brief SDL2 + OpenGL/ES window with event loop and console support.
 *
 * Creates an SDL2 window with an OpenGL (or GLES) context.  Derived classes
 * implement event() and draw().  An optional GLObject can be attached to
 * delegate rendering.  A built-in GLConsole can be activated for in-app
 * debugging.
 */
    class GLWindow {
        private:
        GLMode gl_mode;
    public:
#ifdef __EMSCRIPTEN__
        EMSCRIPTEN_WEBGL_CONTEXT_HANDLE webglContext = 0;    
        void restoreContext();
#endif
        /**
         * @brief Construct and open an SDL2 + OpenGL window.
         * @param text   Window title.
         * @param width  Width in pixels.
         * @param height Height in pixels.
         * @param resize_ Allow window resizing if @c true.
         * @param mode   GL mode (DESKTOP or ES).
         */
        GLWindow(const std::string &text, int width, int height, bool resize_ = true, GLMode mode = GLMode::DESKTOP) : gl_mode(mode), glContext{nullptr}, window{nullptr} { 
            initGL(text, width, height, resize_);
        }

        /**
         * @brief Construct a window from dimensions and mode (internal use).
         * @param width  Width.
         * @param height Height.
         * @param mode   GL mode.
         */
        GLWindow(int width, int height, GLMode mode);

        /** @brief Destructor — destroys the GL context and SDL window. */
        virtual ~GLWindow();

        GLWindow(const GLWindow&) = delete;
        GLWindow& operator=(const GLWindow&) = delete;
        GLWindow(GLWindow&&) = delete;
        GLWindow& operator=(GLWindow&&) = delete;

        /**
         * @brief Initialise the GL context and SDL window.
         * @param title  Window title.
         * @param width  Width.
         * @param height Height.
         * @param resize_ Allow resizing.
         */
        void initGL(const std::string &title, int width, int height, bool resize_ = true);

        /**
         * @brief Reinitialise with only dimensions (used by Emscripten).
         * @param width  New width.
         * @param height New height.
         */
        void initGL(int width, int height);

        /** @brief Update the GL viewport to match the current window size. */
        void updateViewport();

        /** @brief Swap front and back buffers to display the rendered frame. */
        void swap();

        /**
         * @brief Process a single SDL event (pure virtual).
         * @param e SDL event.
         */
        virtual void event(SDL_Event &e) = 0;

        /** @brief Render the current frame (pure virtual). */
        virtual void draw() = 0;

        /**
         * @brief Called when the window is resized (optional override).
         * @param w New width.
         * @param h New height.
         */
        virtual void resize(int w, int h) {}

        /**
         * @brief Attach a GLObject to receive draw/event callbacks.
         * @param o Pointer to the object (window does not take ownership).
         */
        void setObject(GLObject *o);

        /** @brief Request the event loop to stop. */
        void quit();

        /** @brief Run the event+draw loop until quit() is called. */
        void proc();

        /** @brief Start the blocking main loop. */
        void loop();

        /** @brief Insert a small sleep to cap frame rate. */
        void delay();

        /**
         * @brief Set the asset search path.
         * @param path Directory containing assets.
         */
        void setPath(const std::string &path) { util.path = path; }

        /**
         * @brief Change the window title bar text.
         * @param title New title.
         */
        void setWindowTitle(const std::string &title);

        /**
         * @brief Resize the SDL window.
         * @param w New width.
         * @param h New height.
         */
        void setWindowSize(int w, int h);

        /**
         * @brief Set the window icon from an SDL_Surface.
         * @param ico Icon surface.
         */
        void setWindowIcon(SDL_Surface *ico);

        /**
         * @brief Toggle fullscreen mode.
         * @param full @c true for fullscreen.
         */
        void setFullScreen(bool full);

        /**
         * @brief Activate the built-in debug console.
         * @param fnt  Font file path.
         * @param size Font size in points.
         * @param color Text colour.
         */
        void activateConsole(const std::string &fnt, int size, const SDL_Color &color);

        /**
         * @brief Activate the console at a specific screen region.
         * @param rc   Console rectangle.
         * @param fnt  Font file path.
         * @param size Font size.
         * @param color Text colour.
         */
        void activateConsole(const SDL_Rect &rc, const std::string &fnt, int size, const SDL_Color &color);

        /** @brief Draw the console overlay onto the current frame. */
        void drawConsole();

        /**
         * @brief Show or hide the console overlay.
         * @param show @c true to show, @c false to hide.
         */
        void showConsole(bool show);

        std::unique_ptr<gl::GLObject> object; ///< Optional attached scene object.
        mx::mxUtil util;                      ///< Asset path and utility helpers.
        console::GLConsole console;           ///< Built-in debug console.
        int w = 0;                            ///< Current window width.
        int h = 0;                            ///< Current window height.

        /** @return Pointer to the underlying SDL_Window. */
        SDL_Window *getWindow() { return window; }
#ifdef WITH_MIXER
        mx::Mixer mixer;
#endif
        GLText text;  
        bool console_visible = false;
        bool console_active = false;
    private:
        SDL_GLContext glContext;
        SDL_Window *window = nullptr;
        bool active = false;
        SDL_Event e;
        bool hide_console = false;
    };

/**
 * @class GLObject
 * @brief Abstract interface for scene objects managed by a GLWindow.
 *
 * Derived classes implement load(), draw(), event(), and optionally resize()
 * to participate in the GLWindow render/event loop.  Attach an instance via
 * GLWindow::setObject().
 */
    class GLObject {
    public:
        GLObject() = default;
        virtual ~GLObject() = default;

        /**
         * @brief Load GPU resources (textures, shaders, geometry).
         * @param win Parent window.
         */
        virtual void load(GLWindow *win) = 0;

        /**
         * @brief Render the scene for the current frame.
         * @param win Parent window.
         */
        virtual void draw(GLWindow *win) = 0;

        /**
         * @brief Handle an SDL event.
         * @param window Parent window.
         * @param e      SDL event.
         */
        virtual void event(GLWindow *window, SDL_Event &e) = 0;

        /**
         * @brief Called when the window is resized (optional).
         * @param win Parent window.
         * @param w   New width.
         * @param h   New height.
         */
        virtual void resize(gl::GLWindow *win, int w, int h) {}
    };

    /**
     * @brief Load a GL texture from a file.
     * @param filename Image file path.
     * @return GL texture ID.
     */
    GLuint loadTexture(const std::string &filename);

    /**
     * @brief Load a GL texture from a file and retrieve its dimensions.
     * @param filename Image file path.
     * @param w        Output: width.
     * @param h        Output: height.
     * @return GL texture ID.
     */
    GLuint loadTexture(const std::string &filename, int &w, int &h);

    /**
     * @brief Replace a GL texture's contents from an SDL_Surface.
     * @param texture Target GL texture ID.
     * @param surface Source surface.
     * @param flip    If @c true, flip vertically before uploading.
     */
    void updateTexture(GLuint texture, SDL_Surface *surface, bool flip);

    /**
     * @brief Create a new GL texture from an SDL_Surface.
     * @param surface Source surface.
     * @param flip    If @c true, flip vertically.
     * @return New GL texture ID.
     */
    GLuint createTexture(SDL_Surface *surface, bool flip);

    /**
     * @brief Create a new GL texture from a raw pixel buffer.
     * @param buffer Pointer to RGBA data.
     * @param width  Image width.
     * @param height Image height.
     * @return New GL texture ID.
     */
    GLuint createTexture(void *buffer, int width, int height);

    /**
     * @brief Allocate a blank RGBA SDL_Surface.
     * @param w Width in pixels.
     * @param h Height in pixels.
     * @return Newly allocated surface (caller owns it).
     */
    SDL_Surface *createSurface(int w, int h);
}
#endif
#endif