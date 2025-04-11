#ifndef __CONSOLE__H__MX2
#define __CONSOLE__H__MX2

#include<iostream>
#include<string>
#include<sstream>
#include<unordered_map>
#include<memory>
#include"mx.hpp"
#include"gl.hpp"

namespace console {

    class ConsoleChars {
    public:
        ConsoleChars() = default;
        ~ConsoleChars();

        void load(const std::string &fnt, int size);
        void clear();

        std::unordered_map<char, SDL_Surface *> characters;
    protected:
        mx::Font font;
    };

    class Console {
    public:
        Console() = default;
        ~Console();

        void create(int x, int y, int w, int h);
        void load(const std::string &fnt, int size);
        void clear();
        void print(const std::string &str);
        void keypress(char c);
        SDL_Surface *drawText();
        std::string getText() const;
        void setText(const std::string& text);        
        size_t getCursorPos() const;
        void setCursorPos(size_t pos);
        void procCmd(const std::string &cmd);
    protected:
        ConsoleChars c_chars;
        std::ostringstream data;
        std::string input_text;
        SDL_Surface *surface = nullptr;
        SDL_Rect console_rect;
        
        size_t cursorPos = 0;   
        int cursorX = 0;        
        int cursorY = 0;        
        bool cursorVisible = true;
        Uint32 cursorBlinkTime = 0;

        void checkScroll();
        void updateCursorPosition();
        void checkForLineWrap();
    };

    class GLConsole {
    public:
        GLConsole() = default;
        ~GLConsole();
        
        void load(gl::GLWindow *win);
        void draw(gl::GLWindow *win);
        void event(gl::GLWindow *win, SDL_Event &e);
        void print(const std::string &data);
        void resize(gl::GLWindow *win, int w, int h);
    protected:
        Console console;
        std::unique_ptr<gl::GLSprite> sprite = nullptr;
        gl::ShaderProgram shader;
        GLuint texture = 0;
        GLuint loadTextFromSurface(SDL_Surface *surf);
        void updateTexture(GLuint tex, SDL_Surface *surf);

    };

}

#endif