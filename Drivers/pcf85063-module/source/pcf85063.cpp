// SPDX-License-Identifier: Apache-2.0
#include <drivers/pcf85063.h>
#include <pcf85063_module.h>
#include <tactility/device.h>
#include <tactility/drivers/i2c_controller.h>
#include <tactility/log.h>

#define TAG "PCF85063"

static constexpr uint8_t REG_CTRL1   = 0x00; // Control_1
static constexpr uint8_t CTRL1_STOP  = 0x20; // STOP: 1 = oscillator stopped
static constexpr uint8_t REG_SECONDS = 0x04; // Seconds BCD, bit 7 = OS (clock integrity)
// Registers 0x04–0x0A: seconds, minutes, hours, days, weekdays, months, years

static constexpr TickType_t I2C_TIMEOUT_TICKS = pdMS_TO_TICKS(50);

#define GET_CONFIG(device) (static_cast<const Pcf85063Config*>((device)->config))

// region Helpers

static uint8_t bcd_to_dec(uint8_t bcd) { return static_cast<uint8_t>((bcd >> 4) * 10 + (bcd & 0x0F)); }
static uint8_t dec_to_bcd(uint8_t dec) { return static_cast<uint8_t>(((dec / 10) << 4) | (dec % 10)); }

// endregion

// region Driver lifecycle

static error_t start(Device* device) {
    auto* i2c_controller = device_get_parent(device);
    if (device_get_type(i2c_controller) != &I2C_CONTROLLER_TYPE) {
        LOG_E(TAG, "Parent is not an I2C controller");
        return ERROR_RESOURCE;
    }

    auto address = GET_CONFIG(device)->address;

    // Clear the STOP bit only, so the oscillator runs while the other Control_1 settings
    // (e.g. the crystal load-capacitance select) keep whatever the board already configured.
    if (i2c_controller_register8_reset_bits(i2c_controller, address, REG_CTRL1, CTRL1_STOP, I2C_TIMEOUT_TICKS) != ERROR_NONE) {
        LOG_E(TAG, "Failed to clear STOP bit at 0x%02X", address);
        return ERROR_RESOURCE;
    }

    return ERROR_NONE;
}

static error_t stop(Device* device) {
    // The RTC keeps running on its backup supply; nothing to do.
    return ERROR_NONE;
}

// endregion

extern "C" {

error_t pcf85063_get_datetime(Device* device, RtcDateTime* dt) {
    auto* i2c_controller = device_get_parent(device);
    auto address = GET_CONFIG(device)->address;

    // Burst-read 7 registers starting at 0x04:
    // [0]=seconds [1]=minutes [2]=hours [3]=days [4]=weekdays [5]=months [6]=years
    uint8_t buf[7] = {};
    error_t error = i2c_controller_read_register(i2c_controller, address, REG_SECONDS, buf, sizeof(buf), I2C_TIMEOUT_TICKS);
    if (error != ERROR_NONE) return error;

    if (buf[0] & 0x80u) {
        LOG_E(TAG, "Clock integrity compromised (OS flag set) — data unreliable");
        return ERROR_INVALID_STATE;
    }
    dt->second = bcd_to_dec(buf[0] & 0x7Fu); // mask OS flag
    dt->minute = bcd_to_dec(buf[1] & 0x7Fu);
    dt->hour   = bcd_to_dec(buf[2] & 0x3Fu);
    dt->day    = bcd_to_dec(buf[3] & 0x3Fu);
    // buf[4] = weekday — ignored
    dt->month  = bcd_to_dec(buf[5] & 0x1Fu);
    bool century = (buf[5] & 0x80u) != 0;
    dt->year   = static_cast<uint16_t>(2000 + bcd_to_dec(buf[6]) + (century ? 100 : 0));

    return ERROR_NONE;
}

error_t pcf85063_set_datetime(Device* device, const RtcDateTime* dt) {
    if (dt->year < 2000 || dt->year > 2199 ||
        dt->month < 1 || dt->month > 12 ||
        dt->day < 1 || dt->day > 31 ||
        dt->hour > 23 || dt->minute > 59 || dt->second > 59) {
        return ERROR_INVALID_ARGUMENT;
    }

    auto* i2c_controller = device_get_parent(device);
    auto address = GET_CONFIG(device)->address;

    bool century = (dt->year >= 2100);
    uint8_t y = static_cast<uint8_t>(century ? dt->year - 2100 : dt->year - 2000);

    uint8_t buf[7] = {};
    buf[0] = dec_to_bcd(dt->second);
    buf[1] = dec_to_bcd(dt->minute);
    buf[2] = dec_to_bcd(dt->hour);
    buf[3] = dec_to_bcd(dt->day);
    buf[4] = 0; // weekday — leave as Sunday (unused)
    buf[5] = static_cast<uint8_t>(dec_to_bcd(dt->month) | (century ? 0x80u : 0x00u));
    buf[6] = dec_to_bcd(y);

    return i2c_controller_write_register(i2c_controller, address, REG_SECONDS, buf, sizeof(buf), I2C_TIMEOUT_TICKS);
}

RtcApi pcf85063_rtc_api = {
    .get_time = pcf85063_get_datetime,
    .set_time = pcf85063_set_datetime
};

Driver pcf85063_driver = {
    .name = "pcf85063",
    .compatible = (const char*[]) { "nxp,pcf85063a", nullptr },
    .start_device = start,
    .stop_device = stop,
    .probe = nullptr,
    .api = &pcf85063_rtc_api,
    .device_type = &RTC_TYPE,
    .owner = &pcf85063_module,
    .internal = nullptr
};

} // extern "C"
