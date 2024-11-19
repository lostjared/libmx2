#ifndef ___FONT__H__
#define ___FONT__H__

#include"SDL.h"
#include"SDL_ttf.h"
#include<string>
#include"tee_stream.hpp"
#include<optional>
#include"wrapper.hpp"

namespace mx {

    class Font {
    public:
        Font() = default;
        Font(const std::string &tf);
        Font(const std::string &tf, int size);
        Font(TTF_Font *tf);       
        Font(const Font &f);
        Font(Font &&f);
        ~Font();
        Font &operator=(const Font &f);
        Font &operator=(Font && f);
        bool tryLoadFont(const std::string &fname, int size);
        void loadFont(const std::string &fname, int size);
     
        std::optional<TTF_Font *> handle() const { 
            if(the_font)
                return the_font; 
            return std::nullopt;
        }

        TTF_Font *unwrap() const {
            if(the_font) {
                return the_font;
            }
            mx::system_err << "mx: panic Invalid Font.\n";
            mx::system_err.flush();
            exit(EXIT_FAILURE);
            return 0;
        }

        Wrapper<TTF_Font *> wrapper() const {
            if(the_font)
                return the_font;
            return std::nullopt;
        }

    private:
        TTF_Font *the_font = nullptr;
    };

    class Text {
    public:
        Text() = default;
        void setRenderer(SDL_Renderer *renderer);
        void setColor(SDL_Color color);
        void printText_Solid(const Font &f, int x, int y, const std::string &text);
        void printText_Blended(const Font &f, int x, int y, const std::string &text);
    private:
        SDL_Renderer *renderer = nullptr;
        SDL_Color color_value;
    };

}

#endif