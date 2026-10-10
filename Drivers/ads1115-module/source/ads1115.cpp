// SPDX-License-Identifier: Apache-2.0
#include <drivers/ads1115.h>

#include <tactility/check.h>
#include <tactility/delay.h>
#include <tactility/device.h>
#include <tactility/driver.h>
#include <tactility/drivers/adc_controller.h>
#include <tactility/drivers/i2c_controller.h>
#include <tactility/log.h>

constexpr auto* TAG = "ADS1115";
#define GET_CONFIG(device) (static_cast<const Ads1115Config*>((device)->config))

static constexpr uint8_t REG_CONVERSION = 0x00;
static constexpr uint8_t REG_CONFIG = 0x01;

static constexpr uint16_t CONFIG_OS = 1U << 15;
static constexpr uint16_t CONFIG_MUX_SINGLE_AIN0 = 0b100U << 12;
static constexpr uint16_t CONFIG_PGA_SHIFT = 9;
static constexpr uint16_t CONFIG_MODE_SINGLE_SHOT = 1U << 8;
static constexpr uint16_t CONFIG_DR_860_SPS = 0b111U << 5;
static constexpr uint16_t CONFIG_COMP_QUE_DISABLE = 0b11U;

static constexpr uint8_t CHANNEL_COUNT = 4;
// A conversion at 860 SPS takes about 1.2 ms
static constexpr int CONVERSION_POLL_LIMIT = 10;

extern "C" {

static bool pga_bits_for(uint16_t full_scale_mv, uint16_t* out_bits) {
    static constexpr uint16_t FULL_SCALES_MV[] = { 6144, 4096, 2048, 1024, 512, 256 };
    for (uint16_t i = 0; i < sizeof(FULL_SCALES_MV) / sizeof(FULL_SCALES_MV[0]); i++) {
        if (FULL_SCALES_MV[i] == full_scale_mv) {
            *out_bits = i;
            return true;
        }
    }
    return false;
}

static error_t convert(Device* device, uint8_t channel, int16_t* out_code, TickType_t timeout) {
    const auto* config = GET_CONFIG(device);
    auto* i2c = device_get_parent(device);

    uint16_t pga_bits = 0;
    check(pga_bits_for(config->full_scale_mv, &pga_bits));

    uint16_t start = CONFIG_OS | (CONFIG_MUX_SINGLE_AIN0 + (static_cast<uint16_t>(channel) << 12)) | (pga_bits << CONFIG_PGA_SHIFT) |
        CONFIG_MODE_SINGLE_SHOT | CONFIG_DR_860_SPS | CONFIG_COMP_QUE_DISABLE;
    error_t error = i2c_controller_register16be_set(i2c, config->address, REG_CONFIG, start, timeout);
    if (error != ERROR_NONE) {
        return error;
    }

    // OS reads back as 1 once the conversion is done
    for (int i = 0; i < CONVERSION_POLL_LIMIT; i++) {
        delay_millis(1);
        uint16_t status;
        error = i2c_controller_register16be_get(i2c, config->address, REG_CONFIG, &status, timeout);
        if (error != ERROR_NONE) {
            return error;
        }
        if ((status & CONFIG_OS) != 0) {
            uint16_t code;
            error = i2c_controller_register16be_get(i2c, config->address, REG_CONVERSION, &code, timeout);
            if (error == ERROR_NONE) {
                *out_code = static_cast<int16_t>(code);
            }
            return error;
        }
    }

    return ERROR_TIMEOUT;
}

static error_t read_raw(Device* device, uint8_t channel, int* out_raw, TickType_t timeout) {
    if (channel >= CHANNEL_COUNT) {
        return ERROR_OUT_OF_RANGE;
    }

    // A conversion is a sequence of transfers that must not interleave with another read
    device_lock(device);
    int16_t code = 0;
    error_t error = convert(device, channel, &code, timeout);
    device_unlock(device);

    if (error != ERROR_NONE) {
        return error;
    }

    // 15 bits of positive range scaled to 12 bits
    *out_raw = code < 0 ? 0 : code >> 3;
    return ERROR_NONE;
}

static constexpr AdcControllerApi ADS1115_API = {
    .read_raw = read_raw
};

static error_t start(Device* device) {
    check(device_get_type(device_get_parent(device)) == &I2C_CONTROLLER_TYPE);
    const auto* config = GET_CONFIG(device);

    uint16_t pga_bits;
    if (!pga_bits_for(config->full_scale_mv, &pga_bits)) {
        LOG_E(TAG, "Unsupported full-scale-mv %u", config->full_scale_mv);
        return ERROR_INVALID_ARGUMENT;
    }

    uint16_t value;
    error_t error = i2c_controller_register16be_get(device_get_parent(device), config->address, REG_CONFIG, &value, pdMS_TO_TICKS(50));
    if (error != ERROR_NONE) {
        LOG_E(TAG, "Not responding at 0x%02X", config->address);
        return error;
    }

    return ERROR_NONE;
}

static error_t stop(Device*) {
    return ERROR_NONE;
}

Driver ads1115_driver = {
    .name = "ads1115",
    .compatible = (const char*[]) { "ti,ads1115", nullptr },
    .start_device = start,
    .stop_device = stop,
    .probe = nullptr,
    .api = &ADS1115_API,
    .device_type = &ADC_CONTROLLER_TYPE,
    .internal = nullptr
};

}
