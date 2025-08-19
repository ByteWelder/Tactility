#pragma once

class Colors {

public:

    static void rgb888ToRgb565(uint8_t red, uint8_t green, uint8_t blue, uint16_t* rgb565) {
        uint16_t _rgb565 = (red >> 3);
        _rgb565 = (_rgb565 << 6) | (green >> 2);
        _rgb565 = (_rgb565 << 5) | (blue >> 3);
        *rgb565 = _rgb565;
    }

    static void rgb888ToBgr565(uint8_t red, uint8_t green, uint8_t blue, uint16_t* bgr565) {
        uint16_t _bgr565 = (blue >> 3);
        _bgr565 = (_bgr565 << 6) | (green >> 2);
        _bgr565 = (_bgr565 << 5) | (red >> 3);
        *bgr565 = _bgr565;
    }

    static void rgb565ToRgb888(uint16_t rgb565, uint32_t* rgb888) {
        uint32_t _rgb565 = rgb565;
        uint8_t b = (_rgb565 >> 8) & 0xF8;
        uint8_t g = (_rgb565 >> 3) & 0xFC;
        uint8_t r = (_rgb565 << 3) & 0xF8;

        uint8_t* r8p = reinterpret_cast<uint8_t*>(rgb888);
        uint8_t* g8p = r8p + 1;
        uint8_t* b8p = r8p + 2;

        *r8p = r | ((r >> 3) & 0x7);
        *g8p = g | ((g >> 2) & 0x3);
        *b8p = b | ((b >> 3) & 0x7);
    }
};