#include "XPT2046_TouchscreenSOFTSPI.h"
#include "SoftSPI.h"
#include "../../Boards/CYD-2432S028R/Source/hal/YellowDisplayConstants.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include <inttypes.h>

static const char* TAG = "XPT2046_SoftSPI";
#define CMD_X_READ  0x90  // X position
#define CMD_Y_READ  0xD0  // Y position
#define READ_COUNT  30    // Number of readings to average
#define MSEC_THRESHOLD 3  // Debounce threshold (ms)

template<gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode>
XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>::XPT2046_TouchscreenSOFTSPI(gpio_num_t csPin, gpio_num_t tirqPin)
    : csPin(csPin), tirqPin(tirqPin) {}

template<gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode>
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
        .pull_up_en = mode ? GPIO_PULLUP_DISABLE : GPIO_PULLUP_ENABLE,  // Pull-up for IRQ
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
        fastPinMode(tirqPin, false);  // Input with pull-up
        gpio_install_isr_service(0);
        gpio_isr_handler_add(tirqPin, XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>::isrPin, this);
    }
    touchscreenSPI.begin();
    ESP_LOGI(TAG, "Initialized with CS=%" PRId32 ", IRQ=%" PRId32, (int32_t)csPin, (int32_t)tirqPin);
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
void XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>::calibrate(float xfac_new, float yfac_new, int16_t xoff_new, int16_t yoff_new) {
    xfac = xfac_new;
    yfac = yfac_new;
    xoff = xoff_new;
    yoff = yoff_new;
    ESP_LOGI(TAG, "Calibration set: xfac=%f, yfac=%f, xoff=%d, yoff=%d", xfac, yfac, xoff, yoff);
}

template<gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode>
uint16_t XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>::readXOY(uint8_t cmd) {
    uint16_t buf[READ_COUNT], temp;
    uint32_t sum = 0;
    const uint8_t LOST_VAL = 1;  // Discard 1 high and 1 low

    fastDigitalWrite(csPin, 0);  // Select
    for (uint8_t i = 0; i < READ_COUNT; i++) {
        touchscreenSPI.transfer(cmd);
        buf[i] = touchscreenSPI.transfer16(0x00) >> 3;  // 12-bit value
    }
    fastDigitalWrite(csPin, 1);  // Deselect

    // Sort (bubble sort)
    for (uint8_t i = 0; i < READ_COUNT - 1; i++) {
        for (uint8_t j = i + 1; j < READ_COUNT; j++) {
            if (buf[i] > buf[j]) {
                temp = buf[i];
                buf[i] = buf[j];
                buf[j] = temp;
            }
        }
    }

    // Average middle values
    for (uint8_t i = LOST_VAL; i < READ_COUNT - LOST_VAL; i++) {
        sum += buf[i];
    }
    return sum / (READ_COUNT - 2 * LOST_VAL);
}

template<gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode>
void XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>::update() {
    if (tirqPin != GPIO_NUM_NC && !fastDigitalRead(tirqPin)) {
        isrWake = true;
    }
    if (!isrWake) {
        ESP_LOGD(TAG, "No IRQ, skipping update");
        zraw = 0;
        return;
    }

    uint32_t now = esp_timer_get_time() / 1000;
    if (now - msraw < MSEC_THRESHOLD) return;

    uint16_t ux = readXOY(CMD_X_READ);
    uint16_t uy = readXOY(CMD_Y_READ);

    ESP_LOGI(TAG, "SPI raw: x=%" PRIu16 ", y=%" PRIu16, ux, uy);

    // Updated scaling based on logs
    int16_t x = (ux - 229) * 239 / (489 - 229);  // Map 229-489 to 0-239
    int16_t y = uy * 319 / 496;                   // Map 0-496 to 0-319

    if (x < 0) x = 0;
    if (x > 239) x = 239;
    if (y < 0) y = 0;
    if (y > 319) y = 319;

    ESP_LOGI(TAG, "Pre-rotation: x=%d, y=%d", x, y);

    int16_t swap_tmp;
    switch (rotation) {
        case 0:  // Portrait (try this first)
            swap_tmp = x;
            x = y;
            y = 320 - swap_tmp;
            break;
        case 1:  // Landscape (current)
            x = 240 - x;
            y = 320 - y;
            break;
        case 2:  // Portrait inverted
            swap_tmp = x;
            x = y;
            y = swap_tmp;
            x = 240 - x;
            break;
        case 3:  // Landscape inverted
            break;
    }

    xraw = x;
    yraw = y;
    zraw = 1;
    msraw = now;

    isrWake = false;
    ESP_LOGI(TAG, "Touch raw: x=%d, y=%d", xraw, yraw);
}

template class XPT2046_TouchscreenSOFTSPI<CYD_TOUCH_MISO_PIN, CYD_TOUCH_MOSI_PIN, CYD_TOUCH_SCK_PIN, 0>;
