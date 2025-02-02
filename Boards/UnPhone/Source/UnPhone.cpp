#include "UnPhoneFeatures.h"
#include "hal/UnPhoneDisplay.h"
#include "hal/UnPhonePower.h"
#include "hal/UnPhoneSdCard.h"
#include <Tactility/hal/Configuration.h>

bool unPhoneInitPower();
bool unPhoneInitHardware();
bool unPhoneInitLvgl();

extern const tt::hal::Configuration unPhone = {
    .initBoot = unPhoneInitPower,
    .initHardware = unPhoneInitHardware,
    .initLvgl = unPhoneInitLvgl,
    .createDisplay = createDisplay,
    .sdcard = createUnPhoneSdCard(),
    .power = unPhoneGetPower,
    .i2c = {
        tt::hal::i2c::Configuration {
            .name = "Internal",
            .port = I2C_NUM_0,
            .initMode = tt::hal::i2c::InitMode::ByTactility,
            .canReinit = false,
            .hasMutableConfiguration = false,
            .config = (i2c_config_t) {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = GPIO_NUM_3,
                .scl_io_num = GPIO_NUM_4,
                .sda_pullup_en = true,
                .scl_pullup_en = true,
                .master = {
                    .clk_speed = 400000
                },
                .clk_flags = 0
            }
        },
        tt::hal::i2c::Configuration {
            .name = "Unused",
            .port = I2C_NUM_1,
            .initMode = tt::hal::i2c::InitMode::Disabled,
            .canReinit = true,
            .hasMutableConfiguration = true,
            .config = (i2c_config_t) {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = GPIO_NUM_NC,
                .scl_io_num = GPIO_NUM_NC,
                .sda_pullup_en = false,
                .scl_pullup_en = false,
                .master = {
                    .clk_speed = 400000
                },
                .clk_flags = 0
            }
        }
    }
};
