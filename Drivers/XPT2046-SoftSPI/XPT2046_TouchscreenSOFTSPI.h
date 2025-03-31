#pragma once

#include "driver/gpio.h"
#include "esp_attr.h"  // For IRAM_ATTR
#include <stdint.h>

#define Z_THRESHOLD     400
#define Z_THRESHOLD_INT 75
#define MSEC_THRESHOLD  3
#define SPI_SETTING     0  // Mode 0 (CPOL=0, CPHA=0)

template<gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode>
class SoftSPI;  // Forward declaration

class TS_Point {
public:
    TS_Point() : x(0), y(0), z(0) {}
    TS_Point(int16_t x, int16_t y, int16_t z) : x(x), y(y), z(z) {}
    bool operator==(TS_Point p) { return ((p.x == x) && (p.y == y) && (p.z == z)); }
    bool operator!=(TS_Point p) { return ((p.x != x) || (p.y != y) || (p.z != z)); }
    int16_t x, y, z;
};

template<gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode = 0>
class XPT2046_TouchscreenSOFTSPI {
public:
    XPT2046_TouchscreenSOFTSPI(gpio_num_t csPin, gpio_num_t tirqPin = GPIO_NUM_NC);
    bool begin();
    TS_Point getPoint();
    bool tirqTouched();
    bool touched();
    void readData(uint16_t* x, uint16_t* y, uint8_t* z);
    void setRotation(uint8_t n) { rotation = n % 4; }

private:
    IRAM_ATTR static void isrPin(void* arg);  // ISR declaration
    void update();
    gpio_num_t csPin, tirqPin;
    volatile bool isrWake = true;
    uint8_t rotation = 1;
    int16_t xraw = 0, yraw = 0, zraw = 0;
    uint32_t msraw = 0x80000000;
    SoftSPI<MisoPin, MosiPin, SckPin, Mode> touchscreenSPI;
};
