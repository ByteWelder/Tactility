#pragma once

#include <esp_log.h>
#include "drivers/Colors.h"

#include <cstring>
#include <tt_hal_display.h>

class PixelBuffer {
    uint16_t pixelWidth;
    uint16_t pixelHeight;
    ColorFormat colorFormat;
    uint8_t* data;

public:

    PixelBuffer(uint16_t pixelWidth, uint16_t pixelHeight, ColorFormat colorFormat) :
        pixelWidth(pixelWidth),
        pixelHeight(pixelHeight),
        colorFormat(colorFormat)
    {
        data = static_cast<uint8_t*>(malloc(pixelWidth * pixelHeight * getPixelSize()));
        assert(data != nullptr);
    }

    ~PixelBuffer() {
        free(data);
    }

    uint16_t getPixelWidth() const {
        return pixelWidth;
    }

    uint16_t getPixelHeight() const {
        return pixelHeight;
    }

    ColorFormat getColorFormat() const {
        return colorFormat;
    }

    void* getData() const {
        return data;
    }

    uint32_t getDataSize() const {
        return pixelWidth * pixelHeight * getPixelSize();
    }

    void* getDataAtRow(uint16_t row) const {
        auto address = reinterpret_cast<uint32_t>(data) + (row * getRowDataSize());
        return reinterpret_cast<void*>(address);
    }

    uint16_t getRowDataSize() const {
        return pixelWidth * getPixelSize();
    }

    uint8_t getPixelSize() const {
        switch (colorFormat) {
            case COLOR_FORMAT_MONOCHROME:
                return 1;
            case COLOR_FORMAT_BGR565:
            case COLOR_FORMAT_BGR565_SWAPPED:
            case COLOR_FORMAT_RGB565:
            case COLOR_FORMAT_RGB565_SWAPPED:
                return 2;
            case COLOR_FORMAT_RGB888:
                return 3;
            default:
                // TODO: Crash with error
                return 0;
        }
    }

    uint8_t* getPixelAddress(uint16_t x, uint16_t y) const {
        uint32_t offset = ((y * getPixelWidth()) + x) * getPixelSize();
        uint32_t address = reinterpret_cast<uint32_t>(data) + offset;
        return reinterpret_cast<uint8_t*>(address);
    }

    void setPixel(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b) const {
        auto address = getPixelAddress(x, y);
        switch (colorFormat) {
            case COLOR_FORMAT_MONOCHROME:
                *address = (uint8_t)((uint16_t)r + (uint16_t)g + (uint16_t)b / 3);
                break;
            case COLOR_FORMAT_BGR565:
                Colors::rgb888ToBgr565(r, g, b, reinterpret_cast<uint16_t*>(address));
                break;
            case COLOR_FORMAT_BGR565_SWAPPED: {
                // TODO: Make proper conversion function
                Colors::rgb888ToBgr565(r, g, b, reinterpret_cast<uint16_t*>(address));
                uint8_t temp = *address;
                *address = *(address + 1);
                *(address + 1) = temp;
                break;
            }
            case COLOR_FORMAT_RGB565: {
                Colors::rgb888ToRgb565(r, g, b, reinterpret_cast<uint16_t*>(address));
                break;
            }
            case COLOR_FORMAT_RGB565_SWAPPED: {
                // TODO: Make proper conversion function
                Colors::rgb888ToRgb565(r, g, b, reinterpret_cast<uint16_t*>(address));
                uint8_t temp = *address;
                *address = *(address + 1);
                *(address + 1) = temp;
                break;
            }
            case COLOR_FORMAT_RGB888: {
                uint8_t pixel[3] = { r, g, b };
                memcpy(address, pixel, 3);
                break;
            }
            default:
                // NO-OP
                break;
        }
    }

    void clear(int value = 0) const {
        memset(data, value, getDataSize());
    }
};