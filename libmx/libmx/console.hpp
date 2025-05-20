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
    
    class CommandQueue {
    public:
        struct Command {
            std::string text;
            std::function<void(const std::string&, std::ostream&)> callback;
        };
        
        void push(const Command& cmd);
        bool pop(Command& cmd);
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

    class ConsoleChars {
    public:
        ConsoleChars();
        ~ConsoleChars();

        void load(const std::string &fnt, int size, const SDL_Color &c);
        void clear();

        std::unordered_map<char, SDL_Surface *> characters;
    protected:
        std::unique_ptr<mx::Font> font;
    };

    enum FadeState { 
        FADE_NONE, 
        FADE_IN, 
        FADE_OUT 
    };

    class GLConsole;

    class Console {
    public:
        Console();
        ~Console();
        friend class GLConsole;
        mx::mxUtil *util;
        void create(int x, int y, int w, int h);
        void load(const std::string &fnt, int size, const SDL_Color &col);
        void clear();
        void print(const std::string &str);
        void thread_safe_print(const std::string &str);
        void keypress(char c);
        SDL_Surface *drawText();
        std::string getText() const;
        void setText(const std::string& text);        
        size_t getCursorPos() const;
        void setCursorPos(size_t pos);
        void procCmd(const std::string &cmd);
        void setPrompt(const std::string &text);
        void scrollToBottom();
        void setCallback(gl::GLWindow *window, std::function<bool(gl::GLWindow *win, const std::vector<std::string> &)> callback);        
        void moveCursorLeft();
        void moveCursorRight();
        void moveHistoryUp();    
        void moveHistoryDown();
        int getHeight() const { return console_rect.h; }
        int getWidth() const { return console_rect.w; }
        std::string promptText = "$ "; 
        Uint32 cursorBlinkTime = 0;
        bool cursorVisible = true;
        gl::GLWindow *window = nullptr;
        void startFadeIn();
        void startFadeOut();
        bool isFading() const { return fadeState != FADE_NONE; }
        bool isVisible() const { return alpha > 0; }
        void show();
        void hide();
        void reload();
        bool loaded() const { 
            THREAD_GUARD(console_mutex);
            return c_chars.characters.size() > 0; 
        }
        void setTextAttrib(const int size, const SDL_Color &col);
        std::ostringstream &bufferData();
        void setInputCallback(std::function<int(gl::GLWindow *win, const std::string &)> callback);
        std::string inputBuffer;
        bool needsRedraw = true;
        bool needsReflow = true;
        FadeState fadeState = FADE_NONE;
        void process_message_queue();     
#if defined(__EMSCRIPTEN__)
        void processCommandQueue();  // Manually process commands for Emscripten
#endif
    protected:
        mutable THREAD_MUTEX console_mutex;
        std::queue<std::string> message_queue;
        ConsoleChars c_chars;
        std::string font;
        int font_size = 16;
        SDL_Color color = {255, 255, 255, 255};
        std::ostringstream data;
        std::string input_text;
        SDL_Surface *surface = nullptr;
        SDL_Rect console_rect;
        size_t cursorPos = 0;
        size_t stopPosition = 0;
        bool promptWouldWrap = false;
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
        struct TextLine {
            std::string text;       
            size_t startPos;        
            size_t length;          
        };
        std::vector<TextLine> lines;  
        bool inMultilineMode = false;
        std::string multilineBuffer;
        int braceCount = 0;
        std::string originalPrompt;      
        void calculateLines();
        std::unique_ptr<std::thread> worker_thread;
        CommandQueue command_queue;
        std::atomic<bool> worker_active{true};
        void worker_thread_func();
        bool enterCallbackSet = false;
    };

    class GLConsole {
    public:
        GLConsole();
        ~GLConsole();
        friend class Console;
        void load(gl::GLWindow *win, const SDL_Rect &rc, const std::string &fnt, int size, const SDL_Color &col);
        void load(gl::GLWindow *win, const std::string &fnt, int size, const SDL_Color &col);
        void draw(gl::GLWindow *win);
        void handleEvent(SDL_Event &e);
        void event(gl::GLWindow *win, SDL_Event &e);
        void print(const std::string &data);
        void println(const std::string &data);
        void thread_safe_print(const std::string &data);
        void resize(gl::GLWindow *win, int w, int h);
        void setPrompt(const std::string &prompt);
        void setCallback(gl::GLWindow *window, std::function<bool(gl::GLWindow *win, const std::vector<std::string> &)> callback);
        void setInputCallback(std::function<int(gl::GLWindow *window, const std::string &)> callback);
        void show();
        void hide();
        void setStretch(bool stretch) { stretch_value = stretch; }
        void setTextAttrib(const int size, const SDL_Color &col);
        int getWidth() const;
        int getHeight() const;
        void setWindow(gl::GLWindow *win) { console.window = win; } 
        bool procDefaultCommands(const std::vector<std::string> &cmd);
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

        std::ostream &bufferData() { return console.bufferData(); }
        std::istream &inputData();
        void process_message_queue();
        bool isVisible() const { return console.isVisible(); }
        bool isFading() const { return console.isFading(); }
        void setStretchHeight(int value);
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