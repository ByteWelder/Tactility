#include "XPT2046_TouchscreenSOFTSPI.h"
#include "SoftSPI.h"
#include "../../Boards/CYD-2432S028R/Source/hal/YellowDisplayConstants.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <inttypes.h>

static const char* TAG = "XPT2046_SoftSPI";
#define Z_THRESHOLD 400
#define Z_THRESHOLD_INT 75
#define MSEC_THRESHOLD 3

template<gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode>
XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>::XPT2046_TouchscreenSOFTSPI(gpio_num_t csPin, gpio_num_t tirqPin)
    : csPin(csPin), tirqPin(tirqPin) {}

template<gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode>
IRAM_ATTR void XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>::isrPin(void* arg) {
    auto* o = static_cast<XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>*>(arg);
    o->isrWake = true;
    ESP_LOGD(TAG, "IRQ triggered");
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
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = mode ? GPIO_INTR_DISABLE : GPIO_INTR_NEGEDGE
    };
    gpio_config(&io_conf);
}

template<gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode>
bool XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>::begin() {
    fastPinMode(csPin, true);
    fastDigitalWrite(csPin, 1);  // HIGH
    if (tirqPin != GPIO_NUM_NC) {
        fastPinMode(tirqPin, false);
        gpio_install_isr_service(0);
        gpio_isr_handler_add(tirqPin, XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>::isrPin, this);
    }
    touchscreenSPI.begin();
    ESP_LOGI(TAG, "Initialized with CS=%" PRId32 ", IRQ=%" PRId32, (int32_t)csPin, (int32_t)tirqPin);
    return true;
}

template<gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode>
bool XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>::tirqTouched() {
    return isrWake;
}

template<gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode>
bool XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>::touched() {
    update();
    return zraw >= Z_THRESHOLD;
}

template<gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode>
TS_Point XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>::getPoint() {
    update();
    return TS_Point(xraw, yraw, zraw);
}

template<gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode>
void XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>::readData(uint16_t* x, uint16_t* y, uint16_t* z) {
    update();
    *x = xraw;
    *y = yraw;
    *z = zraw;
}

static int16_t besttwoavg(int16_t x, int16_t y, int16_t z) {
    int16_t da = abs(x - y), db = abs(x - z), dc = abs(z - y);
    if (da <= db && da <= dc) return (x + y) >> 1;
    if (db <= da && db <= dc) return (x + z) >> 1;
    return (y + z) >> 1;
}

template<gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode>
void XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>::update() {
    if (!isrWake) return;
    uint32_t now = esp_timer_get_time() / 1000;  // Milliseconds
    if (now - msraw < MSEC_THRESHOLD) return;

    fastDigitalWrite(csPin, 0);  // LOW
    ets_delay_us(10);  // Stabilize

    // Z measurement
    touchscreenSPI.transfer(0xB1);  // Z1
    uint16_t z1_raw = touchscreenSPI.transfer16(0xC1);  // Z2
    int16_t z1 = z1_raw >> 3;
    int z = z1 + 4095;
    uint16_t z2_raw = touchscreenSPI.transfer16(0x91);  // Dummy X
    int16_t z2 = z2_raw >> 3;
    z -= z2;

    int16_t data[6];
    if (z >= Z_THRESHOLD) {
        touchscreenSPI.transfer16(0x91);  // Dummy X
        data[0] = touchscreenSPI.transfer16(0xD1) >> 3;  // Y
        data[1] = touchscreenSPI.transfer16(0x91) >> 3;  // X
        data[2] = touchscreenSPI.transfer16(0xD1) >> 3;  // Y
        data[3] = touchscreenSPI.transfer16(0x91) >> 3;  // X
    } else {
        data[0] = data[1] = data[2] = data[3] = 0;
    }
    data[4] = touchscreenSPI.transfer16(0xD0) >> 3;  // Y (power down)
    data[5] = touchscreenSPI.transfer16(0x00) >> 3;

    fastDigitalWrite(csPin, 1);  // HIGH

    ESP_LOGI(TAG, "SPI raw: z1=%" PRId16 " (raw=%" PRIu16 "), z2=%" PRId16 " (raw=%" PRIu16 "), z=%d, data=[%" PRId16 ", %" PRId16 ", %" PRId16 ", %" PRId16 ", %" PRId16 ", %" PRId16 "]",
             z1, z1_raw, z2, z2_raw, z, data[0], data[1], data[2], data[3], data[4], data[5]);

    if (z < 0) z = 0;
    if (z < Z_THRESHOLD) {
        zraw = 0;
        if (z < Z_THRESHOLD_INT && tirqPin != GPIO_NUM_NC) isrWake = false;
        return;
    }
    zraw = z;

    int16_t x = besttwoavg(data[0], data[2], data[4]);
    int16_t y = besttwoavg(data[1], data[3], data[5]);

    if (z >= Z_THRESHOLD) {
        msraw = now;
        switch (rotation) {
            case 0: xraw = 4095 - y; yraw = x; break;
            case 1: xraw = x; yraw = y; break;
            case 2: xraw = y; yraw = 4095 - x; break;
            default: xraw = 4095 - x; yraw = 4095 - y; break;
        }
    }
}

// Explicit instantiation
template class XPT2046_TouchscreenSOFTSPI<CYD_TOUCH_MISO_PIN, CYD_TOUCH_MOSI_PIN, CYD_TOUCH_SCK_PIN, 0>;
