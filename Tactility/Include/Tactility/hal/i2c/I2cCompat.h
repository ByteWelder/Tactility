#pragma once

#ifdef ESP_PLATFORM

#include <hal/i2c_types.h>
#include <driver/i2c.h>

#else

#include <cstdint>

enum i2c_port_t {
    I2C_NUM_0 = 0,
    I2C_NUM_1,
    LP_I2C_NUM_0,
    I2C_NUM_MAX,
};

enum i2c_mode_t {
    I2C_MODE_MASTER,
    I2C_MODE_MAX,
};

struct i2c_config_t {
    i2c_mode_t mode;
    int sda_io_num;
    int scl_io_num;
    bool sda_pullup_en;
    bool scl_pullup_en;
    union {
        struct {
            uint32_t clk_speed;
        } master;
    };
    uint32_t clk_flags;
};

#endif
