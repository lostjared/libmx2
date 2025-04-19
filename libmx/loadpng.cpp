#include"loadpng.hpp"
#include<png.h>
#include"SDL.h"
#include<cstdio>
#include<cstdlib>
#include<cctype>
#include<string>
#include<iostream>
#include"tee_stream.hpp"

#if defined(_MSC_VER)
    #if _MSC_VER >= 1930
    #define SAFE_FUNC
    #endif
#endif

namespace png {
    SDL_Surface* LoadPNG(const char* file) {
        auto chkString = [](const std::string &filename) -> bool {
            std::string lwr;
            for(size_t i =  0; i < filename.length(); ++i) 
                lwr += tolower(filename[i]);
            if(lwr.find(".png") == std::string::npos)
                return false;

            return true;
        };

        if(chkString(file) == false)
            return NULL;

        #ifdef SAFE_FUNC
        FILE* fp = NULL;
        errno_t err = fopen_s(&fp, file, "rb");
        if (!(err == 0 && file != NULL)) {
            mx::system_err << "Failed to open png file: " << file << "\n";
            return nullptr;
        }
        #else
        FILE* fp = fopen(file, "rb");
        if (!fp) {
            mx::system_err << "Failed to open file: " << file << "\n";
            return nullptr;
        }
        #endif
        png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
        if (!png) {
            fclose(fp);
            return nullptr;
        }

        png_infop info = png_create_info_struct(png);
        if (!info) {
            png_destroy_read_struct(&png, nullptr, nullptr);
            fclose(fp);
            return nullptr;
        }

        if (setjmp(png_jmpbuf(png))) {
            png_destroy_read_struct(&png, &info, nullptr);
            fclose(fp);
            return nullptr;
        }

        png_init_io(png, fp);
        png_read_info(png, info);

        int width = png_get_image_width(png, info);
        int height = png_get_image_height(png, info);
        png_byte color_type = png_get_color_type(png, info);
        png_byte bit_depth = png_get_bit_depth(png, info);

        if (bit_depth == 16) {
            png_set_strip_16(png);
        }

        if (color_type == PNG_COLOR_TYPE_PALETTE) {
            png_set_palette_to_rgb(png);
        }

        if (png_get_valid(png, info, PNG_INFO_tRNS)) {
            png_set_tRNS_to_alpha(png);
        }

        if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
            png_set_gray_to_rgb(png);
        }

        if (color_type == PNG_COLOR_TYPE_RGB) {
            png_set_add_alpha(png, 0xFF, PNG_FILLER_AFTER);
        }

        png_read_update_info(png, info);

        int pitch = png_get_rowbytes(png, info);
        Uint32 rmask, gmask, bmask, amask;

    #if SDL_BYTEORDER == SDL_BIG_ENDIAN
        rmask = 0xFF000000;
        gmask = 0x00FF0000;
        bmask = 0x0000FF00;
        amask = 0x000000FF;
    #else
        rmask = 0x000000FF;
        gmask = 0x0000FF00;
        bmask = 0x00FF0000;
        amask = 0xFF000000;
    #endif

        SDL_Surface* surface = SDL_CreateRGBSurface(0, width, height, 32, rmask, gmask, bmask, amask);
        if (!surface) {
            png_destroy_read_struct(&png, &info, nullptr);
            fclose(fp);
            return nullptr;
        }

        png_bytep* row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
        if(row_pointers == NULL) {
            fprintf(stderr, "Error memory allocation error.\n");
            if (surface)
                SDL_FreeSurface(surface);
            if (png && info) 
                png_destroy_read_struct(&png, &info, nullptr);
            if (fp) 
                fclose(fp);
            return NULL;
        }
        Uint8* pixels = (Uint8*) surface->pixels;

        for (int y = 0; y < height; ++y) {
            row_pointers[y] = pixels + y * pitch;
        }

        png_read_image(png, row_pointers);
        png_read_end(png, nullptr);

        free(row_pointers);
        png_destroy_read_struct(&png, &info, nullptr);
        fclose(fp);
        return surface;
    }


    bool SavePNG(SDL_Texture* texture, SDL_Renderer* renderer, const char* filename) {
        int width, height;
        SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);
        SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA32);
        if (!surface) {
            mx::system_err << "mx: Failed to create SDL_Surface: " << SDL_GetError() << "\n";
            return false;
        }

        SDL_Texture *old = SDL_GetRenderTarget(renderer);

        SDL_SetRenderTarget(renderer, texture);
        if (SDL_RenderReadPixels(renderer, nullptr, surface->format->format, surface->pixels, surface->pitch) != 0) {
            std::cerr << "mx: Failed to read pixels from renderer: " << SDL_GetError() << "\n";
            SDL_FreeSurface(surface);
            return false;
        }
        SDL_SetRenderTarget(renderer, nullptr); 
        FILE* fp = fopen(filename, "wb");
        if (!fp) {
            mx::system_err << "mx: Failed to open file for writing: " << filename << "\n";
            SDL_FreeSurface(surface);
            return false;
        }

        png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
        if (!png) {
            mx::system_err << "mx: Failed to create PNG write struct." << "\n";
            fclose(fp);
            SDL_FreeSurface(surface);
            return false;
        }

        png_infop info = png_create_info_struct(png);
        if (!info) {
            mx::system_err << "mx: Failed to create PNG info struct." << "\n";
            png_destroy_write_struct(&png, nullptr);
            fclose(fp);
            SDL_FreeSurface(surface);
            return false;
        }

        if (setjmp(png_jmpbuf(png))) {
            mx::system_err << "mx: Error during PNG creation." << "\n";
            png_destroy_write_struct(&png, &info);
            fclose(fp);
            SDL_FreeSurface(surface);
            return false;
        }

        png_init_io(png, fp);
        png_set_IHDR(png, info, surface->w, surface->h, 8, PNG_COLOR_TYPE_RGBA,
                    PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
        png_write_info(png, info);

        uint8_t* pixels = static_cast<uint8_t*>(surface->pixels);
        png_bytep* row_pointers = new png_bytep[surface->h];
        for (int y = 0; y < surface->h; ++y) {
            row_pointers[y] = pixels + y * surface->pitch;
        }

        png_write_image(png, row_pointers);
        png_write_end(png, nullptr);

        delete[] row_pointers;
        png_destroy_write_struct(&png, &info);
        fclose(fp);
        SDL_FreeSurface(surface);
        SDL_SetRenderTarget(renderer, old);
        return true;
    }

    bool SavePNG_RGBA(const char *filename, void *buffer, int w, int h) {
        int pitch = w * 4;
        FILE* fp = fopen(filename, "wb");
        if (!fp) {
            mx::system_err << "mx: Failed to open file for writing: " << filename << "\n";
            return false;
        }
        png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
        if (!png) {
            mx::system_err << "mx: Failed to create PNG write struct." << "\n";
            fclose(fp);
            return false;
        }
        png_infop info = png_create_info_struct(png);
        if (!info) {
            mx::system_err << "mx: Failed to create PNG info struct." << "\n";
            png_destroy_write_struct(&png, nullptr);
            fclose(fp);
            return false;
        }
        if (setjmp(png_jmpbuf(png))) {
            mx::system_err << "mx: Error during PNG creation." << "\n";
            png_destroy_write_struct(&png, &info);
            fclose(fp);
            return false;
        }
        png_init_io(png, fp);
        png_set_IHDR(png, info, w, h, 8, PNG_COLOR_TYPE_RGBA,
                    PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
        png_write_info(png, info);

        uint8_t* pixels = static_cast<uint8_t*>(buffer);
        png_bytep* row_pointers = new png_bytep[h];
        for (int y = 0; y < h; ++y) {
            row_pointers[y] = pixels + y * pitch;
        }

        png_write_image(png, row_pointers);
        png_write_end(png, nullptr);

        delete[] row_pointers;
        png_destroy_write_struct(&png, &info);
        fclose(fp);
        return true;
    }
}