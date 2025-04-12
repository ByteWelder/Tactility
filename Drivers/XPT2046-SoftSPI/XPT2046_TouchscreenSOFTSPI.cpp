#include "XPT2046_TouchscreenSOFTSPI.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include <algorithm>

static const char* TAG = "XPT2046_SoftSPI";
#define CMD_X_READ  0xD0  // X position
#define CMD_Y_READ  0x90  // Y position
#define CMD_Z1_READ 0xB0  // Z1 (pressure)
#define CMD_Z2_READ 0xC0  // Z2 (pressure)
#define READ_COUNT  10    // Number of readings (reduced for speed)
#define MSEC_THRESHOLD 20 // Debounce (ms)
#define VARIANCE_THRESHOLD 100 // Max allowed sample spread
#define ADC_MIN 50        // Min valid ADC value
#define ADC_MAX 4045      // Max valid ADC value (4096 - 50)
#define Z_THRESHOLD 100   // Min Z for valid touch

template<gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode>
XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>::XPT2046_TouchscreenSOFTSPI(gpio_num_t csPin, gpio_num_t tirqPin)
    : csPin(csPin), tirqPin(tirqPin) {}

template<gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode>
IRAM_ATTR void XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>::isrPin(void* arg) {
    auto* o = static_cast<XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>*>(arg);
    o->isrWake = true;
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

template<gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode>
bool XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>::begin() {
    fastPinMode(csPin, true);
    fastDigitalWrite(csPin, 1);
    if (tirqPin != GPIO_NUM_NC) {
        fastPinMode(tirqPin, false);
        gpio_install_isr_service(0);
        gpio_isr_handler_add(tirqPin, isrPin, this);
    }
    touchscreenSPI.begin();
    ESP_LOGI(TAG, "Initialized with CS=%d, IRQ=%d", csPin, tirqPin);
    return true;
}

template<gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode>
bool XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>::tirqTouched() {
    return tirqPin != GPIO_NUM_NC && !fastDigitalRead(tirqPin);
}

template<gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode>
bool XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>::touched() {
    update();
    return zraw > 0;
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

template<gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode>
uint16_t XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>::readXOY(uint8_t cmd) {
    uint16_t buf[READ_COUNT], temp;
    uint32_t sum = 0;
    const uint8_t LOST_VAL = 1;  // Discard top/bottom 10% (1/10)

    fastDigitalWrite(csPin, 0);
    for (uint8_t i = 0; i < READ_COUNT; i++) {
        touchscreenSPI.transfer(cmd);
        buf[i] = touchscreenSPI.transfer16(0x00) >> 3;  // 12-bit
    }
    fastDigitalWrite(csPin, 1);

    // Sort samples
    for (uint8_t i = 0; i < READ_COUNT - 1; i++) {
        for (uint8_t j = i + 1; j < READ_COUNT; j++) {
            if (buf[i] > buf[j]) {
                temp = buf[i];
                buf[i] = buf[j];
                buf[j] = temp;
            }
        }
    }

    uint16_t min_val = buf[LOST_VAL];
    uint16_t max_val = buf[READ_COUNT - LOST_VAL - 1];
    if (max_val - min_val > VARIANCE_THRESHOLD || min_val < ADC_MIN || max_val > ADC_MAX) {
        ESP_LOGW(TAG, "readXOY: cmd=0x%02x, invalid: variance=%u, min=%u, max=%u",
                 cmd, max_val - min_val, min_val, max_val);
        return 0;
    }

    for (uint8_t i = LOST_VAL; i < READ_COUNT - LOST_VAL; i++) {
        sum += buf[i];
    }
    uint16_t avg = sum / (READ_COUNT - 2 * LOST_VAL);
    ESP_LOGI(TAG, "readXOY: cmd=0x%02x, avg=%u, min=%u, max=%u, variance=%u",
             cmd, avg, min_val, max_val, max_val - min_val);
    return avg;
}

template<gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode>
void XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>::getRawTouch(uint16_t& rawX, uint16_t& rawY) {
    uint16_t discard_buf = 0;
    // Discard first X read (unreliable, per esp_lcd_touch_xpt2046)
    fastDigitalWrite(csPin, 0);
    touchscreenSPI.transfer(CMD_X_READ);
    discard_buf = touchscreenSPI.transfer16(0x00) >> 3;
    fastDigitalWrite(csPin, 1);

    rawX = readXOY(CMD_X_READ);
    rawY = readXOY(CMD_Y_READ);
    if (rawX != 0 && rawY != 0) {
        ESP_LOGI(TAG, "Raw touch read: x=%u, y=%u", rawX, rawY);
    }
}

template<gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode>
void XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>::update() {
    bool irqState = tirqPin != GPIO_NUM_NC && !fastDigitalRead(tirqPin);
    if (!irqState && !isrWake) {
        zraw = 0;
        return;
    }

    uint32_t now = esp_timer_get_time() / 1000;
    if (now - msraw < MSEC_THRESHOLD) return;

    // Read Z for pressure detection
    uint16_t z1 = readXOY(CMD_Z1_READ);
    uint16_t z2 = readXOY(CMD_Z2_READ);
    uint16_t z = z1 && z2 ? (z1 >> 3) + (4096 - (z2 >> 3)) : 0;

    if (z < Z_THRESHOLD) {
        zraw = 0;
        isrWake = false;
        return;
    }

    uint16_t x, y;
    getRawTouch(x, y);

    if (x == 0 || y == 0) {
        zraw = 0;
        isrWake = false;
        return;
    }

    int16_t swap_tmp;
    switch (rotation) {
        case 0: break;
        case 1: swap_tmp = x; x = y; y = 240 - swap_tmp; break;
        case 2: x = 240 - x; y = 320 - y; break;
        case 3: swap_tmp = x; x = 320 - y; y = swap_tmp; break;
    }

    xraw = x;
    yraw = y;
    zraw = z;
    msraw = now;
    isrWake = false;

    if (xraw != 0 || yraw != 0) {
        ESP_LOGI(TAG, "Touch raw (rotated): x=%d, y=%d, z=%d", xraw, yraw, zraw);
    }
}

XPT2046_TouchscreenSOFTSPI<CYD_TOUCH_MISO_PIN, CYD_TOUCH_MOSI_PIN, CYD_TOUCH_SCK_PIN, 0> touch(CYD_TOUCH_CS_PIN, CYD_TOUCH_IRQ_PIN);

template class XPT2046_TouchscreenSOFTSPI<CYD_TOUCH_MISO_PIN, CYD_TOUCH_MOSI_PIN, CYD_TOUCH_SCK_PIN, 0>;
