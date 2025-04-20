#ifndef __CONSOLE__H__MX2
#define __CONSOLE__H__MX2

#include<iostream>
#include<string>
#include<sstream>
#include<unordered_map>
#include<memory>
#include"mx.hpp"
#include<vector>
#include<functional>

namespace gl {
    class GLWindow;
    class GLSprite;
    class ShaderProgram;
}

#ifndef GL_UNSIGNED_INT
typedef unsigned int GLuint;
#endif


namespace console {

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

    class Console {
    public:
        Console() = default;
        ~Console();
        mx::mxUtil *util;
        void create(int x, int y, int w, int h);
        void load(const std::string &fnt, int size, const SDL_Color &col);
        void clear();
        void print(const std::string &str);
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
        bool loaded() const { return c_chars.characters.size() > 0; }
        void setTextAttrib(const int size, const SDL_Color &col);
    protected:
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
        std::string inputBuffer;
        bool showInput = true;  
        void checkScroll();
        void updateCursorPosition();
        void checkForLineWrap();
        std::function<bool(gl::GLWindow *win, const std::vector<std::string> &)> callback = nullptr;     
        bool callbackSet = false;
        std::vector<std::string> commandHistory;    
        int historyIndex = -1;                      
        std::string tempBuffer;   
        unsigned char alpha = 188;                  
        enum FadeState { 
            FADE_NONE, 
            FADE_IN, 
            FADE_OUT 
        };
        FadeState fadeState = FADE_NONE;
        Uint32 fadeStartTime = 0;
        unsigned int fadeDuration = 300; 
    };

    class GLConsole {
    public:
        GLConsole();
        ~GLConsole();
        void load(gl::GLWindow *win, const SDL_Rect &rc, const std::string &fnt, int size, const SDL_Color &col);
        void load(gl::GLWindow *win, const std::string &fnt, int size, const SDL_Color &col);
        void draw(gl::GLWindow *win);
        void event(gl::GLWindow *win, SDL_Event &e);
        void print(const std::string &data);
        void println(const std::string &data);
        void resize(gl::GLWindow *win, int w, int h);
        void setPrompt(const std::string &prompt);
        void setCallback(gl::GLWindow *window, std::function<bool(gl::GLWindow *win, const std::vector<std::string> &)> callback);
        void show();
        void hide();
        void setStretch(bool stretch) { stretch_value = stretch; }
        void setTextAttrib(const int size, const SDL_Color &col);
        std::ostringstream textval;
        int getWidth() const;
        int getHeight() const;
        void printf(const char *s) {
            if(s == nullptr) return;
            while(*s) {
              textval << *s++;
            }
            print(textval.str());
            textval.str("");
        }
    
        template<typename T, typename... Args>
        void printf(const char *s, T value, Args... args) {
            while(s && *s) {
                if(*s == '%' && *++s!='%') {
                    textval << value;
                    return printf(++s, args...);
                }
                textval << *s++;
            }
            print(textval.str());
            textval.str("");
        }
        bool isVisible() const { return console.isVisible(); }
        bool isFading() const { return console.isFading(); }
        void setStretchHeight(int value);
        protected:
        Console console;
        bool stretch_value;
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
    };

}

#endif