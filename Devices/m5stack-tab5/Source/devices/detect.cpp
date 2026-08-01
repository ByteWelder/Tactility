#include "detect.h"

#include <tactility/device.h>
#include <tactility/drivers/i2c_controller.h>
#include <tactility/log.h>

#include <esp_lcd_touch_gt911.h>
#include <esp_lcd_touch_st7123.h>

#include <freertos/FreeRTOS.h>

constexpr auto* TAG = "Tab5";

static Tab5Variant detected_variant = Tab5Variant::Unknown;

Tab5Variant tab5_get_variant() {
    return detected_variant;
}

void tab5_set_variant(Tab5Variant variant) {
    detected_variant = variant;
}

// The ST7123 (V2) and ST7121 (V3) touch controllers share the same fixed I2C address, so presence
// alone doesn't distinguish them. Both expose a firmware-version byte at register 0x0000 (a 16-bit
// register address, per ESP_LCD_TOUCH_IO_I2C_ST7123_CONFIG's lcd_cmd_bits=16 - see the M5Tab5
// UserDemo's bsp_detect_display_type()): fw_version 1 means ST7121/V3, fw_version 3 means
// ST7123/V2. Returns false (leaving *out_variant untouched) if the register read fails or reports
// an unrecognized value, so the caller's outer attempt loop can retry rather than the touch IC's
// transient not-finished-booting state permanently misdetecting V3 hardware as V2.
static bool probe_st7123_or_st7121_variant(Device* i2c0, TickType_t timeout, Tab5Variant* out_variant) {
    const uint8_t reg_addr[2] = {0x00, 0x00};
    uint8_t fw_version = 0;
    if (i2c_controller_write_read(i2c0, ESP_LCD_TOUCH_IO_I2C_ST7123_ADDRESS, reg_addr, sizeof(reg_addr), &fw_version, 1, timeout) != ERROR_NONE) {
        LOG_W(TAG, "display_detect: failed to read touch FW version, retrying");
        return false;
    }
    if (fw_version == 1) {
        LOG_I(TAG, "display_detect: detected ST7121 touch (FW version 1) — using variant V3");
        *out_variant = Tab5Variant::V3;
        return true;
    }
    if (fw_version == 3) {
        LOG_I(TAG, "display_detect: detected ST7123 touch (FW version 3) — using variant V2");
        *out_variant = Tab5Variant::V2;
        return true;
    }
    LOG_W(TAG, "display_detect: touch at ST7123 address reported unknown FW version %u, retrying", fw_version);
    return false;
}

Tab5Variant tab5_probe_variant(Device* i2c0) {
    // Allow time for the touch IC to fully boot after the reset pulse above: 100ms is enough for
    // I2C ACK (probe) but cold power-on needs ~300ms before register reads succeed reliably.
    vTaskDelay(pdMS_TO_TICKS(300));

    constexpr auto PROBE_TIMEOUT = pdMS_TO_TICKS(50);

    for (int attempt = 0; attempt < 3; ++attempt) {
        if (i2c_controller_has_device_at_address(i2c0, ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS, PROBE_TIMEOUT) == ERROR_NONE ||
            i2c_controller_has_device_at_address(i2c0, ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP, PROBE_TIMEOUT) == ERROR_NONE) {
            LOG_I(TAG, "display_detect: detected GT911 touch — using variant V1 ");
            return Tab5Variant::V1;
        }

        if (i2c_controller_has_device_at_address(i2c0, ESP_LCD_TOUCH_IO_I2C_ST7123_ADDRESS, PROBE_TIMEOUT) == ERROR_NONE) {
            Tab5Variant variant;
            if (probe_st7123_or_st7121_variant(i2c0, PROBE_TIMEOUT, &variant)) {
                return variant;
            }
            // FW-version read failed/unrecognized - fall through to the retry delay below instead
            // of giving up immediately, same as an address-probe miss.
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }

    LOG_W(TAG, "display_detect: no known touch controller detected, defaulting to V2");
    return Tab5Variant::V2;
}
