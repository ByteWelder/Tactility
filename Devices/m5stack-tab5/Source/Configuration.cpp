#include "devices/Display.h"
#include "devices/SdCard.h"

#include <Tactility/hal/Configuration.h>

using namespace tt::hal;

static const auto LOGGER = tt::Logger("Tab5");

static DeviceVector createDevices() {
    return {
        createDisplay(),
        createSdCard(),
    };
}

static bool initBoot() {
    // From https://github.com/m5stack/M5GFX/blob/03565ccc96cb0b73c8b157f5ec3fbde439b034ad/src/M5GFX.cpp
    static constexpr uint8_t reg_data_io1_1[] = {
        0x03, 0b01111111, 0,   // PI4IO_REG_IO_DIR
        0x05, 0b01000110, 0,   // PI4IO_REG_OUT_SET (bit4=LCD Reset,bit5=GT911 TouchReset  LOW)
        0x07, 0b00000000, 0,   // PI4IO_REG_OUT_H_IM
        0x0D, 0b01111111, 0,   // PI4IO_REG_PULL_SEL
        0x0B, 0b01111111, 0,   // PI4IO_REG_PULL_EN
        0xFF,0xFF,0xFF,
    };

    // From https://github.com/m5stack/M5GFX/blob/03565ccc96cb0b73c8b157f5ec3fbde439b034ad/src/M5GFX.cpp
    static constexpr uint8_t reg_data_io1_2[] = {
        0x05, 0b01110110, 0,   // PI4IO_REG_OUT_SET (bit4=LCD Reset,bit5=GT911 TouchReset  HIGH)
        0xFF,0xFF,0xFF,
    };

    // From https://github.com/m5stack/M5GFX/blob/03565ccc96cb0b73c8b157f5ec3fbde439b034ad/src/M5GFX.cpp
    static constexpr uint8_t reg_data_io2[] = {
        0x03, 0b10111001, 0,   // PI4IO_REG_IO_DIR
        0x07, 0b00000110, 0,   // PI4IO_REG_OUT_H_IM
        0x0D, 0b10111001, 0,   // PI4IO_REG_PULL_SEL
        0x0B, 0b11111001, 0,   // PI4IO_REG_PULL_EN
        0x09, 0b01000000, 0,   // PI4IO_REG_IN_DEF_STA
        0x11, 0b10111111, 0,   // PI4IO_REG_INT_MASK
        0x05, 0b10001001, 0,   // PI4IO_REG_OUT_SET
        0xFF,0xFF,0xFF,
    };

    // constexpr auto pi4io1_i2c_addr = 0x43;
    // if (i2c::masterWrite(I2C_NUM_0, pi4io1_i2c_addr, reg_data_io1_1, sizeof(reg_data_io1_1))) {
    //     LOGGER.error("I2C init of PI4IO1 failed");
    // }
    //
    // if (i2c::masterWrite(I2C_NUM_0, pi4io1_i2c_addr, reg_data_io2, sizeof(reg_data_io2))) {
    //     LOGGER.error("I2C init of PI4IO1 failed");
    // }
    //
    // tt::kernel::delayTicks(10);
    // if (i2c::masterWrite(I2C_NUM_0, pi4io1_i2c_addr, reg_data_io1_2, sizeof(reg_data_io1_2))) {
    //     LOGGER.error("I2C init of PI4IO1 failed");
    // }

    return true;
}

extern const Configuration hardwareConfiguration = {
    .initBoot = initBoot,
    .createDevices = createDevices,
    .i2c = {
        i2c::Configuration {
            .name = "Internal",
            .port = I2C_NUM_0,
            .initMode = i2c::InitMode::ByTactility,
            .isMutable = false,
            .config = (i2c_config_t) {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = GPIO_NUM_31,
                .scl_io_num = GPIO_NUM_32,
                .sda_pullup_en = true,
                .scl_pullup_en = true,
                .master = {
                    .clk_speed = 400000
                },
                .clk_flags = 0
            }
        },
        // i2c::Configuration {
        //     .name = "Port A",
        //     .port = I2C_NUM_1,
        //     .initMode = i2c::InitMode::ByTactility,
        //     .isMutable = false,
        //     .config = (i2c_config_t) {
        //         .mode = I2C_MODE_MASTER,
        //         .sda_io_num = GPIO_NUM_53,
        //         .scl_io_num = GPIO_NUM_54,
        //         .sda_pullup_en = true,
        //         .scl_pullup_en = true,
        //         .master = {
        //             .clk_speed = 400000
        //         },
        //         .clk_flags = 0
        //     }
        // }
    }
};
