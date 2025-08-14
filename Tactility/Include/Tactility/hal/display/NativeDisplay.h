#pragma once

namespace tt::hal::display {

enum class ColorFormat {
    Monochrome, // 1 bpp
    BGR565,
    RGB565,
    RGB888
};

class NativeDisplay {

public:

    virtual ~NativeDisplay() = default;

    virtual ColorFormat getColorFormat() const = 0;
    virtual uint16_t getPixelWidth() const = 0;
    virtual uint16_t getPixelHeight() const = 0;
    virtual bool drawBitmap(int xStart, int yStart, int xEnd, int yEnd, const void* pixelData) = 0;
};

}