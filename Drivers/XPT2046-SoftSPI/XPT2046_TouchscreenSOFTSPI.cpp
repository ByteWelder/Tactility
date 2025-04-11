#include "XPT2046_TouchscreenSOFTSPI.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include <inttypes.h>

static const char* TAG = "XPT2046_SoftSPI";
#define CMD_X_READ  0x90
#define CMD_Y_READ  0xD0
#define READ_COUNT  30
#define MSEC_THRESHOLD 3

template <gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode>
XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>::XPT2046_TouchscreenSOFTSPI(gpio_num_t csPin, gpio_num_t tirqPin)
    : csPin(csPin), tirqPin(tirqPin) {}

template <gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode>
IRAM_ATTR void XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>::isrPin(void* arg) {
    auto* o = static_cast<XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>*>(arg);
    o->isrWake = true;
    ESP_LOGD(TAG, "IRQ triggered, state=%d", gpio_get_level(o->tirqPin));
}

static inline void fastDigitalWrite(gpio_num_t pin, bool level) {
    gpio_set_level(pin, level);
}

static inline bool fastDigitalRead(gpio_num_t pin) {
    return gpio_get_level(pin) != 0;
}

static inline void fastPinMode(gpio_num_t pin, bool mode) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << pin),
        .mode = mode ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT,
        .pull_up_en = mode ? GPIO_PULLUP_DISABLE : GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = mode ? GPIO_INTR_DISABLE : GPIO_INTR_NEGEDGE
    };
    gpio_config(&io_conf);
}

template <gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode>
bool XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>::begin() {
    fastPinMode(csPin, true);
    fastDigitalWrite(csPin, 1);
    if (tirqPin != GPIO_NUM_NC) {
        fastPinMode(tirqPin, false);
        gpio_install_isr_service(0);
        gpio_isr_handler_add(tirqPin, XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>::isrPin, this);
    }
    touchscreenSPI.begin();
    touchscreenSPI.setBitOrder(SoftSPI<MisoPin, MosiPin, SckPin, Mode>::MSBFIRST);
    touchscreenSPI.setDataMode(Mode);
    touchscreenSPI.setClockDivider(16);
    ESP_LOGI(TAG, "Initialized with CS=%" PRId32 ", IRQ=%" PRId32 ", Mode=%d", (int32_t)csPin, (int32_t)tirqPin, Mode);
    return true;
}

template <gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode>
bool XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>::tirqTouched() {
    return tirqPin != GPIO_NUM_NC && !fastDigitalRead(tirqPin);
}

template <gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode>
bool XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>::touched() {
    update();
    return zraw > 0;
}

template <gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode>
TS_Point XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>::getPoint() {
    update();
    return TS_Point(xraw, yraw, zraw);
}

template <gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode>
void XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>::readData(uint16_t* x, uint16_t* y, uint16_t* z) {
    update();
    *x = xraw;
    *y = yraw;
    *z = zraw;
}

template <gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode>
uint16_t XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>::readXOY(uint8_t cmd) {
    uint16_t buf[READ_COUNT], temp;
    uint32_t sum = 0;

    fastDigitalWrite(csPin, 0);
    touchscreenSPI.transfer(cmd);
    for (uint8_t i = 0; i < READ_COUNT; ++i)
        buf[i] = touchscreenSPI.transfer16(0x00) >> 3;

    fastDigitalWrite(csPin, 1);

    // Insertion sort
    for (uint8_t i = 0; i < READ_COUNT - 1; ++i) {
        for (uint8_t j = i + 1; j < READ_COUNT; ++j) {
            if (buf[i] > buf[j]) {
                temp = buf[i];
                buf[i] = buf[j];
                buf[j] = temp;
            }
        }
    }

    for (uint8_t i = 1; i < READ_COUNT - 1; ++i)
        sum += buf[i];

    return sum / (READ_COUNT - 2);
}

template <gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode>
void XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>::getRawTouch(uint16_t& rawX, uint16_t& rawY) {
    rawX = readXOY(CMD_X_READ);
    rawY = readXOY(CMD_Y_READ);
}

template <gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode>
void XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>::update() {
    bool irqState = tirqPin != GPIO_NUM_NC && !fastDigitalRead(tirqPin);
    uint32_t now = esp_timer_get_time() / 1000;

    if (now - msraw < MSEC_THRESHOLD && !irqState && !isrWake)
        return;

    int16_t x = readXOY(CMD_X_READ);
    int16_t y = readXOY(CMD_Y_READ);

    if (x == 0 && y == 0 && !irqState && !isrWake) {
        zraw = 0;
        isrWake = false;
        return;
    }

    int16_t tmp;
    switch (rotation) {
        case 1: tmp = x; x = y; y = 240 - tmp; break;
        case 2: x = 240 - x; y = 320 - y; break;
        case 3: tmp = x; x = 320 - y; y = tmp; break;
    }

    xraw = x;
    yraw = y;
    zraw = (x > 0 || y > 0) ? 1 : 0;
    msraw = now;
    isrWake = false;
}

// Explicit instantiation and global object
template class XPT2046_TouchscreenSOFTSPI<CYD_TOUCH_MISO_PIN, CYD_TOUCH_MOSI_PIN, CYD_TOUCH_SCK_PIN, SPI_SETTING>;
XPT2046_TouchscreenSOFTSPI<CYD_TOUCH_MISO_PIN, CYD_TOUCH_MOSI_PIN, CYD_TOUCH_SCK_PIN, SPI_SETTING> touch(CYD_TOUCH_CS_PIN, CYD_TOUCH_IRQ_PIN);
