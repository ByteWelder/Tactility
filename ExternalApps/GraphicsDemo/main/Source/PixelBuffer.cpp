#include "PixelBuffer.h"

#include <cstdlib>
#include <cstring>



PixelBuffer::PixelBuffer(uint16_t width, uint16_t height, ColorFormat colorFormat) :
    width(width),
    height(height),
    colorFormat(colorFormat) {
    data = malloc(width * height * getPixelSize());
    assert(data != nullptr);
}

PixelBuffer::~PixelBuffer() {
    free(data);
}

void PixelBuffer::clear() const {
    memset(data, 0, getDataSize());
}

uint8_t PixelBuffer::getPixelSize() const {
    switch (colorFormat) {
        case COLOR_FORMAT_MONOCHROME:
            return 1;
        case COLOR_FORMAT_BGR565:
            return 2;
        case COLOR_FORMAT_RGB565:
            return 2;
        case COLOR_FORMAT_RGB888:
            return 3;
        default:
            return 0;
    }
}
