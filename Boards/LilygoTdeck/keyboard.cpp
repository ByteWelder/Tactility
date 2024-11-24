#include "keyboard.h"
#include "config.h"
#include "lvgl.h"
#include "TactilityCore.h"
#include "Ui/LvglKeypad.h"
#include "Hal/I2c/I2c.h"
#include <driver/i2c.h>

#define TAG "tdeck_keyboard"

typedef struct {
    lv_indev_t* device;
} KeyboardData;

static inline bool keyboard_i2c_read(uint8_t* output) {
    return tt::hal::i2c::masterRead(TDECK_KEYBOARD_I2C_BUS_HANDLE, TDECK_KEYBOARD_SLAVE_ADDRESS, output, 1);
}

void keyboard_wait_for_response() {
    TT_LOG_I(TAG, "Waiting for keyboard response...");
    bool awake = false;
    uint8_t read_buffer = 0x00;
    do {
        awake = keyboard_i2c_read(&read_buffer);
        if (!awake) {
            tt::delay_ms(100);
        }
    } while (!awake);
    TT_LOG_I(TAG, "Keyboard responded");
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
            TT_LOG_I(TAG, "Released %d", last_buffer);
            data->key = last_buffer;
            data->state = LV_INDEV_STATE_RELEASED;
        } else if (read_buffer != 0) {
            TT_LOG_I(TAG, "Pressed %d", read_buffer);
            data->key = read_buffer;
            data->state = LV_INDEV_STATE_PRESSED;
        }
    }

    last_buffer = read_buffer;
}

Keyboard keyboard_alloc(_Nullable lv_disp_t* display) {
    auto* data = static_cast<KeyboardData*>(malloc(sizeof(KeyboardData)));

    data->device = lv_indev_create();
    lv_indev_set_type(data->device, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(data->device, &keyboard_read_callback);
    lv_indev_set_display(data->device, display);

    tt::lvgl::keypad_set_indev(data->device);

    return data;
}

void keyboard_free(Keyboard keyboard) {
    auto* data = static_cast<KeyboardData*>(keyboard);
    lv_indev_delete(data->device);
    free(data);
}
