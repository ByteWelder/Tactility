#include <driver/i2c.h>
#include <driver/spi_master.h>
#include <esp_intr_types.h>
#include "Log.h"
#include "hal/CoreS3DisplayConstants.h"
#include "kernel/Kernel.h"
#include "Axp2101/Axp2101.h"
#include "Aw9523/Aw9523.h"

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
        .data_io_default_level = false,
        .max_transfer_sz = CORES3_LCD_DRAW_BUFFER_SIZE,
        .flags = 0,
        .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
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
 * and schematic: https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/datasheet/core/K128%20CoreS3/Sch_M5_CoreS3_v1.0.pdf
 */
bool initGpioExpander() {
    TT_LOG_I(TAG, "AW9523 init");

    /**
     * P0 pins:
     * 0: Touch reset
     * 1: Bus out enable
     * 2: AW88298 reset (I2S)
     * 3: ES7210 interrupt (Audio ADC)
     * 4: SD Card SW(itch on?)
     * 5: USB OTG enable
     * 6: /
     * 7: /
     */

    /**
     * P1 pins:
     * 0: Cam reset
     * 1: LCD reset
     * 2: Touch interrupt
     * 3: AW88298 interrupt (I2S)
     * 4: /
     * 5: /
     * 6: /
     * 7: Boost enable
     */

    uint8_t p0_state = 0U;
    uint8_t p1_state = 0U;

    // Enable touch
    p0_state |= (1U);
    // Bus out enable
    p0_state |= (1U << 1U);
    // I2S
    p0_state |= (1U << 2U);
    // SD card
    p0_state |= (1U << 4U);

    // Enable LCD
    p1_state |= (1U << 1U);
    // Boost enable
    p1_state |= (1U << 7U);

    Aw9523 aw(I2C_NUM_0);

    if (!aw.writeP0(p0_state)) {
        TT_LOG_E(TAG, "AW9523: Failed to set P0");
        return false;
    }

    if (!aw.writeP1(p1_state)) {
        TT_LOG_E(TAG, "AW9523: Failed to set P1");
        return false;
    }

    Axp2101 axp(I2C_NUM_0);
    if (axp.isVBus()) {
        float voltage = 0.0f;
        axp.getVBusVoltage(voltage);
        TT_LOG_I(TAG, "AXP2101: VBus at %.2f", voltage);
    } else {
        TT_LOG_W(TAG, "AXP2101: VBus disabled");
    }

    return true;
}

bool initPowerControl() {
    TT_LOG_I(TAG, "Init power control (AXP2101)");

    Aw9523 aw(I2C_NUM_0);
    // Source: https://github.com/m5stack/M5Unified/blob/b8cfec7fed046242da7f7b8024a4e92004a51ff7/src/utility/Power_Class.cpp#L61
    aw.bitOnP1(0b10000000); // SY7088 boost enable

    /** AXP2101 usage
    Source: https://github.com/m5stack/M5Unified/blob/b8cfec7fed046242da7f7b8024a4e92004a51ff7/README.md?plain=1#L223
    |           |M5Stack<BR>CoreS3<BR>CoreS3SE| |
    |:---------:|:-----------------:|:---------:|
    | ALDO1     |VDD 1v8            | ALDO1     |
    | ALDO2     |VDDA 3v3           | ALDO2     |
    | ALDO3     |CAM 3v3            | ALDO3     |
    | ALDO4     |TF 3v3             | ALDO4     |
    | BLDO1     |AVDD               | BLDO1     |
    | BLDO2     |DVDD               | BLDO2     |
    | DLDO1/DC1 |LCD BL             | DLDO1/DC1 |
    | DLDO2/DC2 | ---               | DLDO2/DC2 |
    | BACKUP    |RTC BAT            | BACKUP    |
    */

    /**
     * 0x92 = ALD01
     * 0x93 = ALD02
     * 0x94 = ALD03
     * 0x95 = ALD04
     * 0x96 = BLD01
     * 0x97 = BLD02
     *
     * DCDC1 : 0.7-3.5V，  25mV/step  1200mA
     * DCDC2 : 0.7-2.275V，25mV/step  1600mA
     * DCDC3 : 0.7-3.5V，  25mV/step   700mA

     * LDOio0: 1.8-3.3V,  100mV/step    50mA
     * LDO1  :                          30mA always on
     * LDO2  : 1.8-3.3V， 100mV/step   200mA
     * LDO3  : 1.8-3.3V， 100mV/step   200mA
     */
    // Source: https://github.com/m5stack/M5Unified/blob/b8cfec7fed046242da7f7b8024a4e92004a51ff7/src/utility/Power_Class.cpp#L64
    static constexpr uint8_t reg_data_array[] = {
        0x90U, 0xBFU,  // LDOS ON/OFF control 0 (backlight)
        0x92U, 18U -5U, // ALDO1 set to 1.8v // for AW88298
        0x93U, 33U -5U, // ALDO2 set to 3.3v // for ES7210
        0x94U, 33U -5U, // ALDO3 set to 3.3v // for camera
        0x95U, 33U -5U, // ALDO3 set to 3.3v // for TF card slot
        0x27, 0x00, // PowerKey Hold=1sec / PowerOff=4sec
        0x69, 0x11, // CHGLED setting
        0x10, 0x30, // PMU common config
        0x30, 0x0F // ADC enabled (for voltage measurement)
    };

    Axp2101 axp(I2C_NUM_0);
    if (axp.setRegisters((uint8_t*)reg_data_array, sizeof(reg_data_array))) {
        TT_LOG_I(TAG, "AXP2101 initialized with %d registers", sizeof(reg_data_array) / 2);
        return true;
    } else {
        TT_LOG_E(TAG, "AXP2101: Failed to set registers");
        return false;
    }
}

bool initBoot() {
    TT_LOG_I(TAG, "initBoot()");
    return initPowerControl() &&
        initGpioExpander() &&
        initSpi3();
}