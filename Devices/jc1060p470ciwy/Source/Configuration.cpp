#include "devices/Display.h"
#include "devices/Power.h"
#include "devices/SdCard.h"

#include <Tactility/hal/Configuration.h>

using namespace tt::hal;

static DeviceVector createDevices() {
    return {
        createDisplay(),
        createSdCard(),
        createPower()
    };
}

extern const Configuration hardwareConfiguration = {
    .createDevices = createDevices,
    .i2c = {
        // I2C for GT911 touch controller
        i2c::Configuration {
            .name = "Touch",
            .port = I2C_NUM_0,
            .initMode = i2c::InitMode::ByTactility,
            .isMutable = false,
            .config = (i2c_config_t) {
                .mode = I2C_MODE_MASTER,
                .sda_io_num = GPIO_NUM_7,
                .scl_io_num = GPIO_NUM_8,
                .sda_pullup_en = true,
                .scl_pullup_en = true,
                .master = {
                    .clk_speed = 400000
                },
                .clk_flags = 0
            }
        }
    }
};
