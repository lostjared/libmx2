/**
 * @file console.hpp
 * @brief OpenGL-backed in-game terminal console for libmx2.
 *
 * Provides four related classes:
 *  - console::CommandQueue  – thread-safe command dispatch queue
 *  - console::ConsoleChars  – per-character SDL_Surface glyph cache
 *  - console::Console       – core text-entry / output widget (SDL_Surface)
 *  - console::GLConsole     – OpenGL wrapper that renders Console via GLSprite
 *
 * Depends on mx.hpp, gl.hpp (forward-declared) and SDL2/SDL_ttf.
 * On Emscripten the threading primitives are replaced with no-op stubs.
 */
#ifndef __CONSOLE__H__MX2
#define __CONSOLE__H__MX2

#include<iostream>
#include<string>
#include<sstream>
#include<unordered_map>
#include<memory>
#include<vector>
#include<functional>
#include<mutex>
#include<queue>
#include<atomic>
#include<condition_variable>
#include<thread>
#include"mx.hpp"

namespace gl {
    class GLWindow;
    class GLSprite;
    class ShaderProgram;
}

#ifndef GL_UNSIGNED_INT
typedef unsigned int GLuint;
#endif

#if defined(__EMSCRIPTEN__)
    class EmscriptenMutex {
    public:
        void lock() {}
        void unlock() {}
    };
    
    class EmscriptenGuard {
    public:
        EmscriptenGuard(EmscriptenMutex&) {}
    };
    
    #define THREAD_MUTEX EmscriptenMutex
    #define THREAD_GUARD(mutex) EmscriptenGuard guard(mutex)
    #define THREAD_ENABLED 0
#else
    #define THREAD_MUTEX std::recursive_mutex
    #define THREAD_GUARD(mutex) std::lock_guard<std::recursive_mutex> guard(mutex)
    #define THREAD_ENABLED 1
#endif


namespace console {
    
    /**
     * @class CommandQueue
     * @brief Thread-safe FIFO queue for dispatching console commands to a worker thread.
     *
     * Commands are pushed from the rendering / event thread and consumed by a dedicated
     * background thread that invokes the associated callback.
     */
    class CommandQueue {
    public:
        /** @brief A single command entry with its text and completion callback. */
        struct Command {
            std::string text;                                                          ///< Raw command string.
            std::function<void(const std::string&, std::ostream&)> callback; ///< Handler invoked on the worker thread.
        };

        /** @brief Push a command onto the queue (thread-safe). */
        void push(const Command& cmd);

        /**
         * @brief Pop the next command (blocking on native, non-blocking on Emscripten).
         * @param cmd Output: the dequeued command.
         * @return @c true if a command was retrieved; @c false if the queue was shut down.
         */
        bool pop(Command& cmd);

        /** @brief Signal the worker to drain and stop. */
        void shutdown();
        
    private:
        std::queue<Command> queue;
#if THREAD_ENABLED
        std::mutex queue_mutex;
        std::condition_variable cv;
#else
        EmscriptenMutex queue_mutex;
#endif
        std::atomic<bool> active{true};
    };

    /**
     * @class ConsoleChars
     * @brief Glyph cache that maps each ASCII character to a pre-rendered SDL_Surface.
     *
     * Surfaces are produced via SDL_ttf and stored in an unordered_map keyed by char.
     * Recreate by calling load() after font changes.
     */
    class ConsoleChars {
    public:
        ConsoleChars();
        ~ConsoleChars();

        ConsoleChars(const ConsoleChars&) = delete;
        ConsoleChars& operator=(const ConsoleChars&) = delete;
        ConsoleChars(ConsoleChars&&) = delete;
        ConsoleChars& operator=(ConsoleChars&&) = delete;

        /**
         * @brief Load/reload the glyph cache from a font file.
         * @param fnt  Path to a TTF font file.
         * @param size Point size.
         * @param c    Colour applied to each glyph surface.
         */
        void load(const std::string &fnt, int size, const SDL_Color &c);

        /** @brief Release all cached SDL_Surface objects. */
        void clear();

        std::unordered_map<char, SDL_Surface *> characters; ///< Glyph surfaces keyed by char.
    protected:
        std::unique_ptr<mx::Font> font;
    };

    enum FadeState { 
        FADE_NONE, 
        FADE_IN, 
        FADE_OUT 
    };

    class GLConsole;

    /**
     * @class Console
     * @brief Core text-console widget backed by an SDL_Surface.
     *
     * Manages an output buffer, an editable input line, command history,
     * optional fade-in/out animation, and a thread-safe message queue.
     * Rendered to an SDL_Surface by drawText(); GLConsole uploads that surface
     * to a GL texture each frame.
     */
    class Console {
    public:
        Console();
        ~Console();

        Console(const Console&) = delete;
        Console& operator=(const Console&) = delete;
        Console(Console&&) = delete;
        Console& operator=(Console&&) = delete;

        friend class GLConsole;
        mx::mxUtil *util; ///< Pointer to asset-path utilities (not owned).

        /**
         * @brief Set position and size of the console rectangle.
         * @param x,y Top-left corner in pixels.
         * @param w,h Width and height in pixels.
         */
        void create(int x, int y, int w, int h);

        /**
         * @brief Load font and rebuild the glyph cache.
         * @param fnt  Path to a TTF font file.
         * @param size Point size.
         * @param col  Glyph render colour.
         */
        void load(const std::string &fnt, int size, const SDL_Color &col);

        /** @brief Clear the output buffer and reset scroll. */
        void clear();

        /**
         * @brief Append text to the output buffer.
         * @param str String to append (may contain newlines).
         */
        void print(const std::string &str);

        /**
         * @brief Append text from any thread (uses the internal message queue).
         * @param str String to append.
         */
        void thread_safe_print(const std::string &str);

        /**
         * @brief Handle a single character key press for the input line.
         * @param c Character received from SDL_TEXTINPUT or SDL_KEYDOWN.
         */
        void keypress(char c);

        /**
         * @brief Render the console contents to an internal SDL_Surface.
         * @return Pointer to the freshly rendered surface (owned by Console).
         */
        SDL_Surface *drawText();

        /** @brief Return the current input-line text. */
        std::string getText() const;

        /** @brief Replace the input-line text. */
        void setText(const std::string& text);

        /** @return Current caret position within the input line. */
        size_t getCursorPos() const;

        /** @brief Move the caret to @p pos within the input line. */
        void setCursorPos(size_t pos);

        /**
         * @brief Tokenise and dispatch a command string.
         * @param cmd Raw command text (may include arguments).
         */
        void procCmd(const std::string &cmd);

        /** @brief Change the prompt prefix (default: "$ "). */
        void setPrompt(const std::string &text);

        /** @brief Jump the scroll view to the most recent output. */
        void scrollToBottom();

        /**
         * @brief Register a command callback invoked when the user presses Enter.
         * @param window   Parent GL window.
         * @param callback Handler receives tokenised arguments; return @c true to suppress default handling.
         */
        void setCallback(gl::GLWindow *window,
                         std::function<bool(gl::GLWindow *win, const std::vector<std::string> &)> callback);

        void moveCursorLeft();   ///< Move caret one character to the left.
        void moveCursorRight();  ///< Move caret one character to the right.
        void moveCursorHome();   ///< Move caret to start of input line.
        void moveCursorEnd();    ///< Move caret to end of input line.
        void moveHistoryUp();    ///< Recall the previous command from history.
        void moveHistoryDown();  ///< Move forward through command history.
        void updateInputScrollOffset(); ///< Recalculate horizontal scroll for long input lines.

        int getHeight() const { return console_rect.h; } ///< @return Pixel height of the console area.
        int getWidth()  const { return console_rect.w; } ///< @return Pixel width of the console area.

        std::string promptText = "$ "; ///< Prompt string displayed before the input line.
        Uint32 cursorBlinkTime = 0;    ///< SDL_GetTicks() value of the last cursor blink toggle.
        bool cursorVisible = true;     ///< Whether the cursor is currently shown.
        gl::GLWindow *window = nullptr;///< Owning GL window (weak reference).

        void startFadeIn();  ///< Begin a fade-in animation.
        void startFadeOut(); ///< Begin a fade-out animation.
        bool isFading()   const { return fadeState != FADE_NONE; } ///< @return @c true while animating.
        bool isVisible()  const { return alpha > 0; }               ///< @return @c true if at least partially visible.
        void show();  ///< Make the console fully visible immediately.
        void hide();  ///< Make the console invisible immediately.
        void reload(); ///< Reload the glyph cache with the current font settings.

        /** @return @c true if the glyph cache has been populated. */
        bool loaded() const {
            THREAD_GUARD(console_mutex);
            return c_chars.characters.size() > 0;
        }

        bool isScrollDragging() const { return scrollDragging; } ///< @return Whether the scrollbar thumb is being dragged.

        /**
         * @brief Change font size and colour without reloading the widget.
         * @param size New point size.
         * @param col  New colour.
         */
        void setTextAttrib(const int size, const SDL_Color &col);

        /**
         * @brief Access the internal output buffer directly as an ostream.
         * @return Reference to the internal ostringstream.
         */
        std::ostringstream &bufferData();

        /**
         * @brief Register a callback for raw input-line submission (Enter key).
         * @param callback Handler receives the raw input string; return value is unused.
         */
        void setInputCallback(std::function<int(gl::GLWindow *win, const std::string &)> callback);

        std::string inputBuffer; ///< Current content of the input line (mirrors internal state).
        bool needsRedraw = true; ///< Set to trigger an SDL_Surface redraw.
        bool needsReflow = true; ///< Set to trigger line-wrapping recalculation.
        FadeState fadeState = FADE_NONE; ///< Current fade animation state.

        /** @brief Process pending messages from the thread-safe print queue (call each frame). */
        void process_message_queue();

#if defined(__EMSCRIPTEN__)
        /** @brief Single-threaded command queue drain (Emscripten only). */
        void processCommandQueue();
#endif

        void scrollUp();    ///< Scroll output up by one line.
        void scrollDown();  ///< Scroll output down by one line.
        void resetScroll(); ///< Reset scroll to the bottom.
        void scrollPageUp();   ///< Scroll up by one page.
        void scrollPageDown(); ///< Scroll down by one page.

        /**
         * @brief Handle a mouse-wheel scroll event.
         * @param mouseX,mouseY Cursor position.
         * @param wheelY        Scroll delta (positive = up).
         */
        void handleMouseScroll(int mouseX, int mouseY, int wheelY);

        /**
         * @brief Test whether the mouse is over the scrollbar thumb.
         * @return @c true if the cursor is inside the scrollbar rectangle.
         */
        bool isMouseOverScrollbar(int mouseX, int mouseY) const;

        void beginScrollDrag(int mouseY);  ///< Start a scrollbar thumb drag.
        void updateScrollDrag(int mouseY); ///< Continue a scrollbar thumb drag.
        void endScrollDrag();              ///< Release the scrollbar thumb.
    protected:
        mutable THREAD_MUTEX console_mutex;
        std::queue<std::string> message_queue;
        ConsoleChars c_chars;
        bool userScrolling = false;
        std::string font;
        int font_size = 16;
        SDL_Color color = {255, 255, 255, 255};
        std::ostringstream data;
        std::string input_text;
        SDL_Surface *surface = nullptr;
        SDL_Rect console_rect;
        size_t cursorPos = 0;
        size_t stopPosition = 0;
        size_t inputCursorPos = 0;
        int cursorX = 0;        
        int cursorY = 0;        
        bool showInput = true;  
        void checkScroll();
        void updateCursorPosition();
        void checkForLineWrap();
        std::function<bool(gl::GLWindow *win, const std::vector<std::string> &)> callback = nullptr;     
        std::function<int(gl::GLWindow *window, const std::string &)> callbackEnter = nullptr;
        bool callbackSet = false;
        std::vector<std::string> commandHistory;    
        int historyIndex = -1;                      
        std::string tempBuffer;   
        unsigned char alpha = 188;                  
        Uint32 fadeStartTime = 0;
        unsigned int fadeDuration = 500;
        int inputScrollOffset = 0; 
        struct TextLine {
            std::string text;       
            size_t startPos;        
            size_t length;          
        };
        std::vector<TextLine> lines;  
        bool inMultilineMode;
        int braceCount;
        std::string multilineBuffer;
        std::string originalPrompt;
        std::string continuationPrompt; 
        void processMultilineInput(const std::string& cmd);
        bool checkBraceBalance(const std::string& text);  
        void calculateLines();
        std::unique_ptr<std::thread> worker_thread;
        CommandQueue command_queue;
        std::atomic<bool> worker_active{true};
        void worker_thread_func();
        bool enterCallbackSet = false;
        void clearText();
        
        int visibleLineCount = 0;            
        int scrollOffset = 0;                
        bool scrollDragging = false;
        int scrollDragStartY = 0;
        int scrollDragStartOffset = 0;
        SDL_Rect scrollbarRect = {0, 0, 0, 0}; 
    };

    /**
     * @class GLConsole
     * @brief OpenGL-rendered overlay console built on top of Console.
     *
     * GLConsole manages a gl::GLSprite and uploads the Console's SDL_Surface
     * to a GL texture every frame. It handles window resize, fade animation,
     * and optional full-height stretching — making it easy to drop an interactive
     * terminal into any GLWindow-based application.
     */
    class GLConsole {
    public:
        GLConsole();
        ~GLConsole();

        GLConsole(const GLConsole&) = delete;
        GLConsole& operator=(const GLConsole&) = delete;
        GLConsole(GLConsole&&) = delete;
        GLConsole& operator=(GLConsole&&) = delete;

        /** @brief Destroy the GL texture and sprite resources. */
        void release();

        friend class Console;

        /**
         * @brief Load and position the console inside a specific rectangle.
         * @param win  Parent GL window.
         * @param rc   Desired console rectangle in window-local pixels.
         * @param fnt  TTF font path.
         * @param size Font point size.
         * @param col  Text colour.
         */
        void load(gl::GLWindow *win, const SDL_Rect &rc, const std::string &fnt, int size, const SDL_Color &col);

        /**
         * @brief Load the console spanning the full window width.
         * @param win  Parent GL window.
         * @param fnt  TTF font path.
         * @param size Font point size.
         * @param col  Text colour.
         */
        void load(gl::GLWindow *win, const std::string &fnt, int size, const SDL_Color &col);

        /**
         * @brief Upload SDL_Surface and blit the GL texture each frame.
         * @param win Current GL window (provides gl context).
         */
        void draw(gl::GLWindow *win);

        /**
         * @brief Low-level SDL event handler (keyboard + mouse).
         * @param e SDL event reference.
         */
        void handleEvent(SDL_Event &e);

        /**
         * @brief High-level event handler with window context.
         * @param win Parent GL window.
         * @param e   SDL event reference.
         */
        void event(gl::GLWindow *win, SDL_Event &e);

        /** @brief Append text to the console output. */
        void print(const std::string &data);

        /** @brief Append text followed by a newline. */
        void println(const std::string &data);

        /** @brief Thread-safe variant of print(). */
        void thread_safe_print(const std::string &data);

        /**
         * @brief Handle window resize — rebuilds GL texture at new dimensions.
         * @param win Parent GL window.
         * @param w,h New viewport size in pixels.
         */
        void resize(gl::GLWindow *win, int w, int h);

        /** @brief Change the prompt prefix string. */
        void setPrompt(const std::string &prompt);

        /** @brief Register a command callback (same semantics as Console::setCallback()). */
        void setCallback(gl::GLWindow *window,
                         std::function<bool(gl::GLWindow *win, const std::vector<std::string> &)> callback);

        /** @brief Register a raw-input callback (same semantics as Console::setInputCallback()). */
        void setInputCallback(std::function<int(gl::GLWindow *window, const std::string &)> callback);

        void show(); ///< Make console visible.
        void hide(); ///< Hide console.

        /** @brief Enable/disable vertical stretch to fill the window height. */
        void setStretch(bool stretch) { stretch_value = stretch; }

        /** @brief Change font size/colour (propagates to inner Console). */
        void setTextAttrib(const int size, const SDL_Color &col);

        int getWidth()  const; ///< @return Console pixel width.
        int getHeight() const; ///< @return Console pixel height.

        /** @brief Update the inner Console's window pointer. */
        void setWindow(gl::GLWindow *win) { console.window = win; }

        /**
         * @brief Handle built-in commands (clear, quit, help, …).
         * @param cmd Tokenised command vector.
         * @return @c true if the command was consumed.
         */
        bool procDefaultCommands(const std::vector<std::string> &cmd);

        /** @brief Reset all registered callbacks to no-ops. */
        void clear_callbacks() {
            console.callback = [](gl::GLWindow *window, const std::vector<std::string> &args) -> bool {
                return false;
            };
            console.callbackEnter = [](gl::GLWindow *window, const std::string &text) -> int {
                return 0;
            };
            console.callbackSet = false;
            console.enterCallbackSet = false;
        }

        /**
         * @brief printf-style helper that formats a string and calls print().
         * @tparam Args Variadic format arguments.
         * @param format printf format string.
         * @param args   Format arguments.
         */
        template<typename... Args>
        void printf(const char *format, Args... args) {
            if (format == nullptr) return;
            int size = std::snprintf(nullptr, 0, format, args...);
            if (size < 0) {
                print("[printf formatting error]\n");
                return;
            }
            std::vector<char> buffer(static_cast<size_t>(size) + 1);
            std::snprintf(buffer.data(), buffer.size(), format, args...);
            print(std::string(buffer.data())); 
        }

        /** @return Stream reference to the inner output buffer. */
        std::ostream &bufferData() { return console.bufferData(); }

        /** @return Stream containing the last submitted input line. */
        std::istream &inputData();

        /** @brief Flush the thread-safe message queue into the output buffer (call each frame). */
        void process_message_queue();

        bool isVisible() const { return console.isVisible(); } ///< @return @c true if the console is at least partially visible.
        bool isFading()  const { return console.isFading();  }  ///< @return @c true while a fade animation is running.

        /** @brief Set an absolute height override for the stretch mode. */
        void setStretchHeight(int value);

        /** @brief Clear the entire output buffer. */
        void clearText();

        /** @brief Drain the command queue and process pending messages (call each frame). */
        void refresh() {
#if defined(__EMSCRIPTEN__)
        console.processCommandQueue();
#endif
        console.process_message_queue(); }
    protected:
        mutable std::mutex gl_mutex;
        Console console;
        bool stretch_value = true;
        SDL_Rect rc;
        int stretch_height = 0;
        std::unique_ptr<gl::GLSprite> sprite = nullptr;
        std::unique_ptr<gl::ShaderProgram> shader;
        GLuint texture = 0;
        GLuint loadTextFromSurface(SDL_Surface *surf);
        void updateTexture(GLuint tex, SDL_Surface *surf);
        std::string font;
        int font_size = 16;
        SDL_Color color = {255, 255, 255, 255};
    private:
        std::istringstream inputStream;
    };
}

#endif