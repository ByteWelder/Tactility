#include "XPT2046_TouchscreenSOFTSPI.h"
#include "SoftSPI.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "nvs_flash.h"
#include <inttypes.h>

static const char* TAG = "XPT2046_SoftSPI";
#define CMD_X_READ  0x90  // X position
#define CMD_Y_READ  0xD0  // Y position
#define READ_COUNT  30    // Number of readings to average
#define MSEC_THRESHOLD 10 // Debounce threshold (ms), was 3

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
        .pull_up_en = mode ? GPIO_PULLUP_DISABLE : GPIO_PULLUP_ENABLE,
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

    // Load calibration from NVS
    nvs_handle_t nvs;
    esp_err_t err = nvs_open("touch_cal", NVS_READONLY, &nvs);
    if (err == ESP_OK) {
        size_t size = sizeof(CalibrationData);
        err = nvs_get_blob(nvs, "cal_data", &calData, &size);
        if (err == ESP_OK && calData.valid) {
            ESP_LOGI(TAG, "Loaded calibration: xScale=%.3f, xOffset=%.3f, yScale=%.3f, yOffset=%.3f",
                     calData.xScale, calData.xOffset, calData.yScale, calData.yOffset);
        } else {
            calData.valid = false;
            ESP_LOGW(TAG, "No valid calibration data found");
        }
        nvs_close(nvs);
    }
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
    int16_t x = xraw, y = yraw;
    if (calData.valid) {
        x = (int16_t)(xraw * calData.xScale + calData.xOffset);
        y = (int16_t)(yraw * calData.yScale + calData.yOffset);
        x = (x < 0) ? 0 : (x > 240 ? 240 : x);
        y = (y < 0) ? 0 : (y > 320 ? 320 : y);
    }
    return TS_Point(x, y, zraw);
}

template<gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode>
void XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>::readData(uint16_t* x, uint16_t* y, uint16_t* z) {
    update();
    *x = calData.valid ? (uint16_t)(xraw * calData.xScale + calData.xOffset) : xraw;
    *y = calData.valid ? (uint16_t)(yraw * calData.yScale + calData.yOffset) : yraw;
    *z = zraw;
    if (calData.valid) {
        *x = (*x < 0) ? 0 : (*x > 240 ? 240 : *x);
        *y = (*y < 0) ? 0 : (*y > 320 ? 320 : *y);
    }
}

template<gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode>
uint16_t XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>::readXOY(uint8_t cmd) {
    uint16_t buf[READ_COUNT], temp;
    uint32_t sum = 0;
    const uint8_t LOST_VAL = 1;

    fastDigitalWrite(csPin, 0);  // Select
    for (uint8_t i = 0; i < READ_COUNT; i++) {
        touchscreenSPI.transfer(cmd);
        buf[i] = touchscreenSPI.transfer16(0x00) >> 3;  // 12-bit value
        ESP_LOGD(TAG, "readXOY: cmd=0x%02x, sample[%d]=%u", cmd, i, buf[i]);
    }
    fastDigitalWrite(csPin, 1);  // Deselect

    for (uint8_t i = 0; i < READ_COUNT - 1; i++) {
        for (uint8_t j = i + 1; j < READ_COUNT; j++) {
            if (buf[i] > buf[j]) {
                temp = buf[i];
                buf[i] = buf[j];
                buf[j] = temp;
            }
        }
    }

    for (uint8_t i = LOST_VAL; i < READ_COUNT - LOST_VAL; i++) {
        sum += buf[i];
    }
    uint16_t avg = sum / (READ_COUNT - 2 * LOST_VAL);
    ESP_LOGI(TAG, "readXOY: cmd=0x%02x, avg=%u", cmd, avg);
    return avg;
}

template<gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode>
void XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>::getRawTouch(uint16_t& rawX, uint16_t& rawY) {
    rawX = readXOY(CMD_X_READ);
    rawY = readXOY(CMD_Y_READ);
    if (rawX != 0 || rawY != 0) {
        ESP_LOGI(TAG, "Raw touch read: x=%" PRIu16 ", y=%" PRIu16, rawX, rawY);
    }
}

template<gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode>
CalibrationData XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>::getCalibration() {
    return calData;
}

template<gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode>
void XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>::setCalibration(const CalibrationData& cal) {
    calData = cal;
    if (calData.valid) {
        nvs_handle_t nvs;
        esp_err_t err = nvs_open("touch_cal", NVS_READWRITE, &nvs);
        if (err == ESP_OK) {
            err = nvs_set_blob(nvs, "cal_data", &calData, sizeof(CalibrationData));
            if (err == ESP_OK) {
                nvs_commit(nvs);
                ESP_LOGI(TAG, "Saved calibration: xScale=%.3f, xOffset=%.3f, yScale=%.3f, yOffset=%.3f",
                         calData.xScale, calData.xOffset, calData.yScale, calData.yOffset);
            }
            nvs_close(nvs);
        }
    }
}

template<gpio_num_t MisoPin, gpio_num_t MosiPin, gpio_num_t SckPin, uint8_t Mode>
void XPT2046_TouchscreenSOFTSPI<MisoPin, MosiPin, SckPin, Mode>::update() {
    bool irqState = tirqPin != GPIO_NUM_NC && !fastDigitalRead(tirqPin);
    ESP_LOGD(TAG, "IRQ state: %d, isrWake: %d", irqState, isrWake);
    if (!irqState && !isrWake) {
        zraw = 0;
        return;
    }

    uint32_t now = esp_timer_get_time() / 1000;
    if (now - msraw < MSEC_THRESHOLD) return;

    int16_t x = readXOY(CMD_X_READ);
    int16_t y = readXOY(CMD_Y_READ);

    if (x == 0 && y == 0) {  // Ignore invalid reads
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
    zraw = 1;
    msraw = now;

    isrWake = false;
    if (xraw != 0 || yraw != 0 || zraw != 0) {
        ESP_LOGI(TAG, "Touch raw (rotated): x=%d, y=%d, z=%d", xraw, yraw, zraw);
    }
}

XPT2046_TouchscreenSOFTSPI<CYD_TOUCH_MISO_PIN, CYD_TOUCH_MOSI_PIN, CYD_TOUCH_SCK_PIN, 0> touch(CYD_TOUCH_CS_PIN, CYD_TOUCH_IRQ_PIN);

template class XPT2046_TouchscreenSOFTSPI<CYD_TOUCH_MISO_PIN, CYD_TOUCH_MOSI_PIN, CYD_TOUCH_SCK_PIN, 0>;
