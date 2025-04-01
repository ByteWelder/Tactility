#pragma once

#include "driver/gpio.h"
#include "esp_attr.h"
#include "SoftSPI.h"
#include <stdint.h>

#define SPI_SETTING 0  // Mode 0 (CPOL=0, CPHA=0)

class TS_Point {
public:
    TS_Point() : x(0), y(0), z(0) {}
    TS_Point(int16_t x, int16_t y, int16_t z) : x(x), y(y), z(z) {}
    bool operator==(TS_Point p) { return ((p.x == x) && (p.y == y) && (p.z == z)); }
    bool operator!=(TS_Point p) { return ((p.x != x) || (p.y != y) || (p.z != z)); }
    int16_t x, y, z;
};

template<gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode = SPI_SETTING>
class XPT2046_TouchscreenSOFTSPI {
public:
    XPT2046_TouchscreenSOFTSPI(gpio_num_t csPin, gpio_num_t tirqPin = GPIO_NUM_NC);
    bool begin();
    TS_Point getPoint();
    bool tirqTouched();
    bool touched();
    void readData(uint16_t* x, uint16_t* y, uint16_t* z);
    void setRotation(uint8_t n) { rotation = n % 4; }
    void calibrate(float xfac, float yfac, int16_t xoff, int16_t yoff);
    // Added for calibration app
    void getRawTouch(uint16_t& rawX, uint16_t& rawY);

private:
    IRAM_ATTR static void isrPin(void* arg);
    void update();
    uint16_t readXOY(uint8_t cmd);
    gpio_num_t csPin, tirqPin;
    volatile bool isrWake = false;
    uint8_t rotation = 1;
    int16_t xraw = 0, yraw = 0, zraw = 0;
    uint32_t msraw = 0x80000000;
    float xfac = 0, yfac = 0;
    int16_t xoff = 0, yoff = 0;
    SoftSPI<MisoPin, MosiPin, SckPin, Mode> touchscreenSPI;
};

// Declare the global instance
extern XPT2046_TouchscreenSOFTSPI<CYD_TOUCH_MISO_PIN, CYD_TOUCH_MOSI_PIN, CYD_TOUCH_SCK_PIN, 0> touch;
