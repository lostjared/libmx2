#include"font.hpp"
#include<iostream>
namespace mx {
        Font::Font(const std::string &tf) {
            loadFont(tf, 14);
        }

        Font::~Font() {
            if(the_font)
                TTF_CloseFont(the_font);
        }

        Font::Font(const std::string &tf, int size) {
            loadFont(tf, size);
        }
        
        Font::Font(TTF_Font *tf) : the_font{tf} {}
        Font::Font(const Font &f) : the_font(f.the_font) {}
        Font::Font(Font &&f) : the_font{std::move(f.the_font)} {}
        
        Font &Font::operator=(const Font &f) {
            the_font = f.the_font;
            return *this;
        }
            
        Font &Font::operator=(Font && f) {
            the_font = std::move(f.the_font);
            return *this;
        }

        bool Font::tryLoadFont(const std::string &fname, int size) {
            the_font = TTF_OpenFont(fname.c_str(), size);
            if(!the_font) {
                std::cerr << "mx: Error opening font: " << TTF_GetError() << "\n";
                return false;
            }
            return true;
        }
        
        void Font::loadFont(const std::string &fname, int size) {
            if(!tryLoadFont(fname, size))
                exit(EXIT_FAILURE);

        }

        void Text::setRenderer(SDL_Renderer *renderer) {
            this->renderer = renderer;
        }
        void Text::setColor(SDL_Color color) {
            color_value = color;
        }
        
        void Text::printText_Solid(const Font &f, int x, int y, const std::string &text) {
                SDL_Surface *surf = TTF_RenderText_Solid(f.handle(),text.c_str(), color_value);
                if(!surf) {
                    std::cerr << "mx: Error rendering text...\n";
                    return;
                }
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
                if(!tex) {
                    std::cerr << "mx: Error creating texture..\n";
                    SDL_FreeSurface(surf);
                    return;
                }
                SDL_Rect rc = {x,y,surf->w,surf->h};
                SDL_RenderCopy(renderer, tex, nullptr, &rc);
                SDL_FreeSurface(surf);
                SDL_DestroyTexture(tex);
        }

        void Text::printText_Blended(const Font &f, int x, int y, const std::string &text) {
                SDL_Surface *surf = TTF_RenderText_Blended(f.handle(),text.c_str(), color_value);
                if(!surf) {
                    std::cerr << "mx: Error rendering text...\n";
                    return;
                }
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
                if(!tex) {
                    std::cerr << "mx: Error creating texture..\n";
                    SDL_FreeSurface(surf);
                    return;
                }
                SDL_Rect rc = {x,y,surf->w,surf->h};
                SDL_RenderCopy(renderer, tex, nullptr, &rc);
                SDL_FreeSurface(surf);
                SDL_DestroyTexture(tex);

        }


}