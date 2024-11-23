#ifdef WITH_JPEG
#include <stdio.h>
#include <jpeglib.h>
#include <setjmp.h>
#include "SDL.h"

namespace jpeg {

    struct my_error_mgr {
        struct jpeg_error_mgr pub;
        jmp_buf setjmp_buffer;
    };

    typedef struct my_error_mgr *my_error_ptr;

    void my_error_exit(j_common_ptr cinfo) {
        my_error_ptr myerr = (my_error_ptr)cinfo->err;
        (*cinfo->err->output_message)(cinfo);
        longjmp(myerr->setjmp_buffer, 1);
    }

    SDL_Surface *LoadJPEG(const char *filename) {
        FILE *file = fopen(filename, "rb");
        if (!file) return NULL;

        struct jpeg_decompress_struct cinfo;
        struct my_error_mgr jerr;

        cinfo.err = jpeg_std_error(&jerr.pub);
        jerr.pub.error_exit = my_error_exit;

        if (setjmp(jerr.setjmp_buffer)) {
            jpeg_destroy_decompress(&cinfo);
            fclose(file);
            return NULL;
        }

        jpeg_create_decompress(&cinfo);
        jpeg_stdio_src(&cinfo, file);
        jpeg_read_header(&cinfo, TRUE);
        jpeg_start_decompress(&cinfo);

        int width = cinfo.output_width;
        int height = cinfo.output_height;
        int pixel_size = cinfo.output_components;
        Uint32 pixel_format = (pixel_size == 3) ? SDL_PIXELFORMAT_RGB24 : SDL_PIXELFORMAT_RGBA32;

        SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 8 * pixel_size, pixel_format);
        if (!surface) {
            jpeg_finish_decompress(&cinfo);
            jpeg_destroy_decompress(&cinfo);
            fclose(file);
            return NULL;
        }

        unsigned char *row_pointer = (unsigned char *)surface->pixels;
        while (cinfo.output_scanline < cinfo.output_height) {
            jpeg_read_scanlines(&cinfo, &row_pointer, 1);
            row_pointer += surface->pitch;
        }

        jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);
        fclose(file);

        return surface;
    }

    bool SaveJPEG(SDL_Renderer *renderer, SDL_Texture *texture, const char *filename, int quality) {
        int width = 0, height = 0;
        Uint32 format = 0;
        int access = 0;

        if (SDL_QueryTexture(texture, &format, &access, &width, &height) != 0) {
            return false;
        }

        SDL_Texture *old = SDL_GetRenderTarget(renderer);
        SDL_SetRenderTarget(renderer, texture);
      

        SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 24, SDL_PIXELFORMAT_RGB24);
        if (!surface) {
            return false;
        }

        if (SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGB24, surface->pixels, surface->pitch) != 0) {
            SDL_FreeSurface(surface);
            return false;
        }

        struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr jerr;
        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_compress(&cinfo);

        FILE* outfile = fopen(filename, "wb");
        if (!outfile) {
            SDL_FreeSurface(surface);
            return false;
        }

        jpeg_stdio_dest(&cinfo, outfile);

        cinfo.image_width = width;
        cinfo.image_height = height;
        cinfo.input_components = 3;
        cinfo.in_color_space = JCS_RGB;

        jpeg_set_defaults(&cinfo);
        jpeg_set_quality(&cinfo, quality, TRUE);

        jpeg_start_compress(&cinfo, TRUE);

        JSAMPROW row_pointer;
        while (cinfo.next_scanline < cinfo.image_height) {
            row_pointer = (JSAMPROW)((Uint8*)surface->pixels + cinfo.next_scanline * surface->pitch);
            jpeg_write_scanlines(&cinfo, &row_pointer, 1);
        }
        jpeg_finish_compress(&cinfo);
        jpeg_destroy_compress(&cinfo);
        fclose(outfile);
        SDL_FreeSurface(surface);
        SDL_SetRenderTarget(renderer, old);
        return true;
    }
}
#endif