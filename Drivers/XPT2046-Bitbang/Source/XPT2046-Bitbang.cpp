#include "XPT2046-Bitbang.h"
#include <algorithm>

XPT2046_Bitbang* XPT2046_Bitbang::instance = nullptr;

// -------------------- Configuration --------------------
XPT2046_Bitbang::Configuration::Configuration(
    gpio_num_t mosiPin,
    gpio_num_t misoPin,
    gpio_num_t clkPin,
    gpio_num_t csPin,
    gpio_num_t irqPin,
    uint16_t xMax,
    uint16_t yMax,
    bool swapXy,
    bool mirrorX,
    bool mirrorY
) : mosiPin(mosiPin),
    misoPin(misoPin),
    clkPin(clkPin),
    csPin(csPin),
    irqPin(irqPin),
    xMax(xMax),
    yMax(yMax),
    swapXy(swapXy),
    mirrorX(mirrorX),
    mirrorY(mirrorY) {}

// -------------------- Constructor --------------------
XPT2046_Bitbang::XPT2046_Bitbang(std::unique_ptr<Configuration> cfg)
    : configuration(std::move(cfg)) {}

// -------------------- SPI Bitbang --------------------
int XPT2046_Bitbang::readSPI(uint8_t cmd) {
    int value = 0;

    gpio_set_level(configuration->csPin, 0);
    ets_delay_us(2);

    for (int i = 7; i >= 0; i--) {
        gpio_set_level(configuration->mosiPin, (cmd >> i) & 1);
        gpio_set_level(configuration->clkPin, 1);
        ets_delay_us(2);
        gpio_set_level(configuration->clkPin, 0);
        ets_delay_us(2);
    }

    for (int i = 15; i >= 0; i--) {
        gpio_set_level(configuration->clkPin, 1);
        ets_delay_us(2);
        if (gpio_get_level(configuration->misoPin)) {
            value |= (1 << i);
        }
        gpio_set_level(configuration->clkPin, 0);
        ets_delay_us(2);
    }

    gpio_set_level(configuration->csPin, 1);
    return (value >> 4) & 0x0FFF;
}

// -------------------- LVGL Callback --------------------
void XPT2046_Bitbang::touchReadCallback(lv_indev_t* indev, lv_indev_data_t* data) {
    XPT2046_Bitbang* touch = static_cast<XPT2046_Bitbang*>(lv_indev_get_user_data(indev));
    if (!touch) return;

    if (touch->isTouched()) {
        Point p = touch->getTouch();
        data->point.x = p.x;
        data->point.y = p.y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

// -------------------- Start / Stop --------------------
bool XPT2046_Bitbang::start(lv_display_t* display) {
    gpio_config_t io_conf = {};
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask = (1ULL << configuration->mosiPin) |
                           (1ULL << configuration->clkPin) |
                           (1ULL << configuration->csPin);
    gpio_config(&io_conf);

    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << configuration->misoPin) |
                           (1ULL << configuration->irqPin);
    gpio_config(&io_conf);

    gpio_set_level(configuration->csPin, 1);
    gpio_set_level(configuration->clkPin, 0);
    gpio_set_level(configuration->mosiPin, 0);

    deviceHandle = lv_indev_create();
    lv_indev_set_type(deviceHandle, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(deviceHandle, touchReadCallback);
    lv_indev_set_user_data(deviceHandle, this);

    instance = this;
    return true;
}

bool XPT2046_Bitbang::stop() {
    cleanup();
    instance = nullptr;
    return true;
}

lv_indev_t* XPT2046_Bitbang::getLvglIndev() { return deviceHandle; }

// -------------------- Touch --------------------
bool XPT2046_Bitbang::isTouched() {
    return gpio_get_level(configuration->irqPin) == 0;
}

Point XPT2046_Bitbang::getTouch() {
    int x = readSPI(0x90); // X
    int y = readSPI(0xD0); // Y

    if (configuration->swapXy) std::swap(x, y);
    if (configuration->mirrorX) x = configuration->xMax - x;
    if (configuration->mirrorY) y = configuration->yMax - y;

    x = std::clamp(x, 0, (int)configuration->xMax);
    y = std::clamp(y, 0, (int)configuration->yMax);

    return Point{x, y};
}

// -------------------- Empty / Minimal --------------------
void XPT2046_Bitbang::calibrate() {}
void XPT2046_Bitbang::setCalibration(int, int, int, int) {}

void XPT2046_Bitbang::cleanup() {
    if (deviceHandle) {
        lv_indev_delete(deviceHandle);
        deviceHandle = nullptr;
    }
}

