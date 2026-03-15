/**
 * @file font.hpp
 * @brief SDL_ttf font and 2D text rendering wrappers.
 */
#ifndef ___FONT__H__
#define ___FONT__H__

#include"SDL.h"
#include"SDL_ttf.h"
#include<string>
#include"tee_stream.hpp"
#include<optional>
#include"wrapper.hpp"

namespace mx {

/**
 * @class Font
 * @brief RAII wrapper around a TTF_Font handle.
 *
 * Loads a TrueType font from disk and manages its lifetime.  The
 * underlying TTF_Font pointer is accessible via handle(), unwrap(),
 * or wrapper() for safe optional-style access.
 */
    class Font {
    public:
        /** @brief Default constructor — font is not loaded. */
        Font() = default;

        /**
         * @brief Load a font without specifying a size (size determined later).
         * @param tf Path to the TTF/OTF file.
         */
        Font(const std::string &tf);

        /**
         * @brief Load a font at the given point size.
         * @param tf   Path to the TTF/OTF file.
         * @param size Point size to load.
         */
        Font(const std::string &tf, int size);

        /**
         * @brief Wrap an already-open TTF_Font pointer (takes ownership).
         * @param tf Existing TTF_Font handle.
         */
        Font(TTF_Font *tf);

        Font(const Font &f) = delete;
        /** @brief Move constructor — transfers ownership of the TTF handle. */
        Font(Font &&f);
        /** @brief Destructor — closes the TTF_Font if loaded. */
        ~Font();
        Font &operator=(const Font &f) = delete;
        /** @brief Move-assign — transfers ownership of the TTF handle. */
        Font &operator=(Font && f);

        /**
         * @brief Attempt to load a font, returning false on failure.
         * @param fname Path to the font file.
         * @param size  Desired point size.
         * @return @c true if loaded successfully, @c false otherwise.
         */
        bool tryLoadFont(const std::string &fname, int size);

        /**
         * @brief Load a font, throwing mx::Exception on failure.
         * @param fname Path to the font file.
         * @param size  Desired point size.
         */
        void loadFont(const std::string &fname, int size);

        /**
         * @brief Return the underlying TTF_Font as an optional.
         * @return std::optional containing the handle if loaded, else std::nullopt.
         */
        std::optional<TTF_Font *> handle() const { 
            if(the_font)
                return the_font; 
            return std::nullopt;
        }

        /**
         * @brief Unwrap the TTF_Font pointer, throwing on null.
         * @return Raw TTF_Font pointer.
         * @throws mx::Exception if the font is not loaded.
         */
        TTF_Font *unwrap() const {
            if(the_font) {
                return the_font;
            }
            throw Exception("unwrap: Invalid Font");
            return 0;
        }

        /**
         * @brief Return a Wrapper around the TTF_Font handle.
         * @return Wrapper<TTF_Font*> that holds the font pointer or nullopt.
         */
        Wrapper<TTF_Font *> wrapper() const {
            if(the_font)
                return the_font;
            return std::nullopt;
        }

    private:
        TTF_Font *the_font = nullptr; ///< Underlying SDL_ttf font handle.
    };

/**
 * @class Text
 * @brief Helper for rendering text strings with SDL_Renderer.
 *
 * Renders text glyphs onto the active SDL_Renderer using either solid
 * (fast) or blended (anti-aliased) rendering modes.
 */
    class Text {
    public:
        /** @brief Default constructor. */
        Text() = default;

        /**
         * @brief Set the SDL renderer used for all draw calls.
         * @param renderer Active SDL_Renderer.
         */
        void setRenderer(SDL_Renderer *renderer);

        /**
         * @brief Set the colour used when rendering text.
         * @param color Desired text colour.
         */
        void setColor(SDL_Color color);

        /**
         * @brief Render text with solid (no anti-aliasing) blending.
         * @param f    Font to use.
         * @param x    X position in renderer coordinates.
         * @param y    Y position in renderer coordinates.
         * @param text String to render.
         */
        void printText_Solid(const Font &f, int x, int y, const std::string &text);

        /**
         * @brief Render text with blended (anti-aliased) quality.
         * @param f    Font to use.
         * @param x    X position in renderer coordinates.
         * @param y    Y position in renderer coordinates.
         * @param text String to render.
         */
        void printText_Blended(const Font &f, int x, int y, const std::string &text);
    private:
        SDL_Renderer *renderer = nullptr; ///< Active renderer.
        SDL_Color color_value;            ///< Current text colour.
    };

}

#endif