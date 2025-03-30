#pragma once

#include "driver/gpio.h"
#include <stdint.h>

#define Z_THRESHOLD     400
#define Z_THRESHOLD_INT 75
#define MSEC_THRESHOLD  3
#define SPI_SETTING     0  // Mode 0 (CPOL=0, CPHA=0)

class SoftSPI;  // Forward declaration

class TS_Point {
public:
    TS_Point() : x(0), y(0), z(0) {}
    TS_Point(int16_t x, int16_t y, int16_t z) : x(x), y(y), z(z) {}
    bool operator==(TS_Point p) { return ((p.x == x) && (p.y == y) && (p.z == z)); }
    bool operator!=(TS_Point p) { return ((p.x != x) || (p.y != y) || (p.z != z)); }
    int16_t x, y, z;
};

class XPT2046_TouchscreenSOFTSPI {
public:
    XPT2046_TouchscreenSOFTSPI(gpio_num_t csPin, gpio_num_t tirqPin = GPIO_NUM_NC);
    bool begin(SoftSPI* touchscreenSPI);
    TS_Point getPoint(SoftSPI* touchscreenSPI);
    bool tirqTouched();
    bool touched(SoftSPI* touchscreenSPI);
    void readData(SoftSPI* touchscreenSPI, uint16_t* x, uint16_t* y, uint8_t* z);
    void setRotation(uint8_t n) { rotation = n % 4; }

private:
    void update(SoftSPI* touchscreenSPI);
    gpio_num_t csPin, tirqPin;
    volatile bool isrWake = true;
    uint8_t rotation = 1;
    int16_t xraw = 0, yraw = 0, zraw = 0;
    uint32_t msraw = 0x80000000;
};
