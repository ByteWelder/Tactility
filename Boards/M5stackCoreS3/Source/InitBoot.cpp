#include <driver/i2c.h>
#include <driver/spi_master.h>
#include <intr_types.h>
#include "Log.h"
#include "hal/CoreS3DisplayConstants.h"
#include "hal/i2c/I2c.h"
#include "CoreS3Constants.h"
#include "kernel/Kernel.h"

#define TAG "core2"

#define CORES3_SPI2_PIN_SCLK GPIO_NUM_36
#define CORES3_SPI2_PIN_MOSI GPIO_NUM_37
#define CORES3_SPI2_PIN_MISO GPIO_NUM_35

/**
 * For details see https://github.com/espressif/esp-bsp/blob/master/bsp/m5stack_core_s3/m5stack_core_s3.c
 */
static bool initSpi3() {
    TT_LOG_I(TAG, LOG_MESSAGE_SPI_INIT_START_FMT, SPI3_HOST);
    const spi_bus_config_t bus_config = {
        .mosi_io_num = CORES3_SPI2_PIN_MOSI,
        .miso_io_num = CORES3_SPI2_PIN_MISO,
        .sclk_io_num = CORES3_SPI2_PIN_SCLK,
        .data2_io_num = GPIO_NUM_NC,
        .data3_io_num = GPIO_NUM_NC,
        .data4_io_num = GPIO_NUM_NC,
        .data5_io_num = GPIO_NUM_NC,
        .data6_io_num = GPIO_NUM_NC,
        .data7_io_num = GPIO_NUM_NC,
        .max_transfer_sz = CORES3_LCD_DRAW_BUFFER_SIZE,
        .flags = 0,
        .isr_cpu_id = INTR_CPU_ID_AUTO,
        .intr_flags = 0
    };

    if (spi_bus_initialize(SPI3_HOST, &bus_config, SPI_DMA_CH_AUTO) != ESP_OK) {
        TT_LOG_E(TAG, LOG_MESSAGE_SPI_INIT_FAILED_FMT, SPI3_HOST);
        return false;
    }

    return true;
}

/**
 * For details see https://github.com/espressif/esp-bsp/blob/master/bsp/m5stack_core_s3/m5stack_core_s3.c
 */
bool initGpioExpander() {
    TT_LOG_I(TAG, "Init AW9523 GPIO expander");
    uint8_t aw9523_P0 = 0b10U;
    uint8_t aw9523_P1 = 0b10100000U;

    // Enable LCD
    aw9523_P1 |= (1 << 1);

    // Enable touch
    aw9523_P0 |= (1);

    // SD card
    aw9523_P0 |= (1 << 4);

    if (!tt::hal::i2c::masterWriteRegister(I2C_NUM_0, AW9523_ADDRESS, 0x02U, &aw9523_P0, 1U, 1000UL)) {
        TT_LOG_E(TAG, "Failed to enable LCD");
        return false;
    }

    if (!tt::hal::i2c::masterWriteRegister(I2C_NUM_0, AW9523_ADDRESS, 0x03U, &aw9523_P1, 1U, 1000UL)) {
        TT_LOG_E(TAG, "Failed to enable touch");
        return false;
    }

    return true;
}

static constexpr const uint32_t POWER_OUTPUT_MASK_BUS = 0b00000010U; // BUS EN
static constexpr const uint32_t POWER_OUTPUT_MASK_USB = 0b00100000U; // USB OTG EN

// Ported from https://github.com/m5stack/M5Unified/blob/b8cfec7fed046242da7f7b8024a4e92004a51ff7/src/utility/Power_Class.cpp#L427
// Also see https://github.com/m5stack/M5CoreS3/blob/fdbb037bf9ea65569542c37be23607ecc2a57cd8/src/AXP2101.cpp#L298
// and https://github.com/m5stack/M5Unified/blob/b8cfec7fed046242da7f7b8024a4e92004a51ff7/src/utility/Power_Class.cpp#L60
// Datasheet: https://www.alldatasheet.com/datasheet-pdf/download/1148542/AWINIC/AW9523B.html
// Schematic: https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/core/K128%20CoreS3/Sch_M5_CoreS3_v1.0.pdf
static bool setOutput(uint8_t mask, bool enable) {
    static constexpr const uint8_t port0_reg = 0x02U;
    static constexpr const uint8_t port1_reg = 0x03U;
    static constexpr const uint8_t port1_bitmask_boost = 0b10000000U; // BOOST_EN

    uint8_t port0_buf = 0U, port1_buf = 0U;
    if (
        tt::hal::i2c::masterReadRegister(I2C_NUM_0, AW9523_ADDRESS, port0_reg, &port0_buf, 1U, 1000UL) &&
        tt::hal::i2c::masterReadRegister(I2C_NUM_0, AW9523_ADDRESS, port1_reg, &port1_buf, 1U, 1000UL)
    ) {
        TT_LOG_I(TAG, "Reading %02x %02x", port0_buf, port1_buf);
        uint8_t p0 = port0_buf | mask;
        uint8_t p1 = port1_buf | port1_bitmask_boost;

        if (!enable) {
            p0 = port0_buf & ~mask;
            if (0U == (p0 & POWER_OUTPUT_MASK_BUS)) {
                p1 &= ~port1_bitmask_boost;
            }
        }
        port0_buf = p0;
        port1_buf = p1;
        TT_LOG_I(TAG, "Writing %02x %02x", port0_buf, port1_buf);
        return tt::hal::i2c::masterWriteRegister(I2C_NUM_0, AW9523_ADDRESS, port0_reg, &port0_buf, 1U, 1000UL) &&
            tt::hal::i2c::masterWriteRegister(I2C_NUM_0, AW9523_ADDRESS, port1_reg, &port1_buf, 1U, 1000UL);
    } else {
        return false;
    }
}

bool initPowerControl() {
    TT_LOG_I(TAG, "Init power control");

    uint8_t boost_bit = 0b10000000U;
    if (!tt::hal::i2c::masterWriteRegister(I2C_NUM_0, AXP2101_ADDRESS, 0x03U, &boost_bit, 1U, 1000UL)) {
        TT_LOG_E(TAG, "Failed to set boost bit");
        return false;
    }

    static constexpr std::uint8_t reg_data_array[] = {
        0x90U, 0xBFU,  // LDOS ON/OFF control 0 (backlight)
        0x92U, 18U -5U, // ALDO1 set to 1.8v // for AW88298
        0x93U, 33U -5U, // ALDO2 set to 3.3v // for ES7210
        0x94U, 33U -5U, // ALDO3 set to 3.3v // for camera
        0x95U, 33U -5U, // ALDO3 set to 3.3v // for TF card slot
        0x27U, 0x00U, // PowerKey Hold=1sec / PowerOff=4sec
        0x10U, 0x30U, // PMU common config
        0x30U, 0x0FU, // ADC enabled (for voltage measurement)
        0x12U, 0x00U, // BATFET disable
        0x68U, 0x01U, // Battery detection enabled.
        0x69U, 0x13U, // CHGLED setting (automatic?) - the other setting in M5Unified is 0x11
        0x99U, 0x00U // DLDO1 set 0.5v (vibration motor)
    };

    if (!tt::hal::i2c::masterWriteRegisterArray(I2C_NUM_0, AXP2101_ADDRESS, reg_data_array, sizeof(reg_data_array), 1000UL)) {
        TT_LOG_E(TAG, "Failed to init AXP2101");
    } else {
        TT_LOG_I(TAG, "Inited AXP2101 at %d registers", sizeof(reg_data_array) / 2);
    }

    return true;
}

bool initBoot() {
    TT_LOG_I(TAG, "initBoot");
    return initPowerControl() &&
        // TODO: Make the BUS power output work
        setOutput(POWER_OUTPUT_MASK_USB, false) &&
        setOutput(POWER_OUTPUT_MASK_BUS, true) &&
        initGpioExpander() &&
        initSpi3();
}