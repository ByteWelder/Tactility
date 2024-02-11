#include "save_png.h"
#include "src/extra/libs/png/lodepng.h"

bool lv_screenshot_save_png_file(const uint8_t* image, uint32_t w, uint32_t h, uint32_t bpp, const char* filename) {
    if (bpp == 32) {
        return lodepng_encode32_file(filename, image, w, h);
    } else if (bpp == 24) {
        return lodepng_encode24_file(filename, image, w, h);
    }
    return false;
}
