#pragma once

#include <tt_hal_display.h>

class PixelBuffer {
    uint16_t width;
    uint16_t height;
    ColorFormat colorFormat;
    void* data;

public:

    PixelBuffer(uint16_t width, uint16_t height, ColorFormat colorFormat);

    ~PixelBuffer();

    uint16_t getWidth() const {
        return width;
    }

    uint16_t getHeight() const {
        return height;
    }

    ColorFormat getColorFormat() const {
        return colorFormat;
    }

    void* getData() const {
        return data;
    }

    uint32_t getDataSize() const {
        return width * height * getPixelSize();
    }

    void* getDataAtRow(uint16_t row) const {
        auto address = reinterpret_cast<uint32_t>(data) + (row * getRowSize());
        return reinterpret_cast<void*>(address);
    }

    uint16_t getRowSize() const {
        return width * getPixelSize();
    }

    uint8_t getPixelSize() const;

    void clear() const;
};