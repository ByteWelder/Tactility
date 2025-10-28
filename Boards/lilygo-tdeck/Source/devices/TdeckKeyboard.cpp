#include "TdeckKeyboard.h"
#include <Tactility/hal/i2c/I2c.h>
#include <driver/i2c.h>

constexpr auto* TAG = "TdeckKeyboard";
constexpr auto TDECK_KEYBOARD_I2C_BUS_HANDLE = I2C_NUM_0;
constexpr auto TDECK_KEYBOARD_SLAVE_ADDRESS = 0x55;

static bool keyboard_i2c_read(uint8_t* output) {
    return tt::hal::i2c::masterRead(TDECK_KEYBOARD_I2C_BUS_HANDLE, TDECK_KEYBOARD_SLAVE_ADDRESS, output, 1, 100 / portTICK_PERIOD_MS);
}

/**
 * The callback simulates press and release events, because the T-Deck
 * keyboard only publishes press events on I2C.
 * LVGL currently works without those extra release events, but they
 * are implemented for correctness and future compatibility.
 *
 * @param indev_drv
 * @param data
 */
static void keyboard_read_callback(TT_UNUSED lv_indev_t* indev, lv_indev_data_t* data) {
    static uint8_t last_buffer = 0x00;
    uint8_t read_buffer = 0x00;

    // Defaults
    data->key = 0;
    data->state = LV_INDEV_STATE_RELEASED;

    if (keyboard_i2c_read(&read_buffer)) {
        if (read_buffer == 0 && read_buffer != last_buffer) {
            TT_LOG_D(TAG, "Released %d", last_buffer);
            data->key = last_buffer;
            data->state = LV_INDEV_STATE_RELEASED;
        } else if (read_buffer != 0) {
            TT_LOG_D(TAG, "Pressed %d", read_buffer);
            data->key = read_buffer;
            data->state = LV_INDEV_STATE_PRESSED;
        }
    }

    last_buffer = read_buffer;
}

bool TdeckKeyboard::startLvgl(lv_display_t* display) {
    deviceHandle = lv_indev_create();
    lv_indev_set_type(deviceHandle, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(deviceHandle, &keyboard_read_callback);
    lv_indev_set_display(deviceHandle, display);
    lv_indev_set_user_data(deviceHandle, this);
    return true;
}

bool TdeckKeyboard::stopLvgl() {
    lv_indev_delete(deviceHandle);
    deviceHandle = nullptr;
    return true;
}

bool TdeckKeyboard::isAttached() const {
    return tt::hal::i2c::masterHasDeviceAtAddress(TDECK_KEYBOARD_I2C_BUS_HANDLE, TDECK_KEYBOARD_SLAVE_ADDRESS, 100);
}
