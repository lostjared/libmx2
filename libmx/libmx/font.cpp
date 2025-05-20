#include"font.hpp"
#include<iostream>
#include<filesystem>
namespace mx {
        Font::Font(const std::string &tf) {
            loadFont(tf, 14);
        }

        Font::~Font() {
            if(the_font != nullptr)
                TTF_CloseFont(the_font);

            the_font = nullptr;
        }

        Font::Font(const std::string &tf, int size) {
            the_font = nullptr;
            loadFont(tf, size);
        }
        
        Font::Font(TTF_Font *tf) : the_font{tf} {}
        //Font::Font(const Font &f) : the_font(f.the_font) {}
        Font::Font(Font &&f) : the_font{f.the_font} {
            f.the_font = nullptr;
        }
        
        /*Font &Font::operator=(const Font &f) {
            the_font = f.the_font;
            return *this;
        }*/
            
        Font &Font::operator=(Font && f) {
            if(this != &f) {
                if(the_font != nullptr) {
                    TTF_CloseFont(the_font);
                    the_font = nullptr;
                }
                the_font = f.the_font;
                f.the_font = nullptr;
            }
            return *this;
        }

        bool Font::tryLoadFont(const std::string &fname, int size) {
            if(the_font != nullptr) {
                TTF_CloseFont(the_font);
                the_font = nullptr;
            }

            if(!std::filesystem::exists(fname)) {
                mx::system_err << "mx: Error loading font: " << fname << "\n";
                throw mx::Exception("Error loading font: " + fname);
                return false;
            }
            
            the_font = TTF_OpenFont(fname.c_str(), size);
            if(!the_font) {
                mx::system_err << "mx: Error opening font: " << TTF_GetError() << "\n";
                throw mx::Exception("Error opening font");
                return false;
            }
            return true;
        }
        
        void Font::loadFont(const std::string &fname, int size) {
            if(!tryLoadFont(fname, size))
                throw mx::Exception("Error loading font..");

        }

        void Text::setRenderer(SDL_Renderer *renderer) {
            this->renderer = renderer;
        }
        void Text::setColor(SDL_Color color) {
            color_value = color;
        }
        
        void Text::printText_Solid(const Font &f, int x, int y, const std::string &text) {
                SDL_Surface *surf = TTF_RenderText_Solid(f.wrapper().unwrap(),text.c_str(), color_value);
                if(!surf) {
                    mx::system_err << "mx: Error rendering text...\n";
                    return;
                }
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
                if(!tex) {
                    mx::system_err << "mx: Error creating texture..\n";
                    SDL_FreeSurface(surf);
                    return;
                }
                SDL_Rect rc = {x,y,surf->w,surf->h};
                SDL_RenderCopy(renderer, tex, nullptr, &rc);
                SDL_FreeSurface(surf);
                SDL_DestroyTexture(tex);
        }

        void Text::printText_Blended(const Font &f, int x, int y, const std::string &text) {
                SDL_Surface *surf = TTF_RenderText_Blended(f.wrapper().unwrap(),text.c_str(), color_value);
                if(!surf) {
                    mx::system_err << "mx: Error rendering text...\n";
                    return;
                }
                SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
                if(!tex) {
                    mx::system_err << "mx: Error creating texture..\n";
                    SDL_FreeSurface(surf);
                    return;
                }
                SDL_Rect rc = {x,y,surf->w,surf->h};
                SDL_RenderCopy(renderer, tex, nullptr, &rc);
                SDL_FreeSurface(surf);
                SDL_DestroyTexture(tex);

        }


}