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
}
#endif