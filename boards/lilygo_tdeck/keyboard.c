#include "keyboard.h"
#include "config.h"
#include "hal/lv_hal.h"
#include "tactility_core.h"
#include "ui/lvgl_keypad.h"
#include <driver/i2c.h>

#define TAG "tdeck_keyboard"
#define KEYBOARD_SLAVE_ADDRESS 0x55

typedef struct {
    lv_indev_drv_t* driver;
    lv_indev_t* device;
} KeyboardData;

static inline esp_err_t keyboard_i2c_read(uint8_t* output) {
    return i2c_master_read_from_device(
        TDECK_I2C_BUS_HANDLE,
        KEYBOARD_SLAVE_ADDRESS,
        output,
        1,
        configTICK_RATE_HZ / 10
    );
}

void keyboard_wait_for_response() {
    TT_LOG_I(TAG, "wake await...");
    bool awake = false;
    uint8_t read_buffer = 0x00;
    do {
        awake = keyboard_i2c_read(&read_buffer) == ESP_OK;
        if (!awake) {
            tt_delay_ms(100);
        }
    } while (!awake);
    TT_LOG_I(TAG, "awake");
}

static void keyboard_read_callback(struct _lv_indev_drv_t* indev_drv, lv_indev_data_t* data) {
    static uint8_t last_buffer = 0x00;
    uint8_t read_buffer = 0x00;

    // Defaults
    data->key = 0;
    data->state = LV_INDEV_STATE_RELEASED;

    if (keyboard_i2c_read(&read_buffer) == ESP_OK) {
        if (read_buffer == 0 && read_buffer != last_buffer) {
            TT_LOG_I(TAG, "released %d", last_buffer);
            data->key = last_buffer;
            data->state = LV_INDEV_STATE_RELEASED;
        } else if (read_buffer != 0) {
            TT_LOG_I(TAG, "pressed %d", read_buffer);
            data->key = read_buffer;
            data->state = LV_INDEV_STATE_PRESSED;
        }
    }

    last_buffer = read_buffer;
}

Keyboard keyboard_alloc(_Nullable lv_disp_t* display) {
    KeyboardData* data = malloc(sizeof(KeyboardData));

    data->driver = malloc(sizeof(lv_indev_drv_t));
    memset(data->driver, 0, sizeof(lv_indev_drv_t));
    lv_indev_drv_init(data->driver);

    data->driver->type = LV_INDEV_TYPE_KEYPAD;
    data->driver->read_cb = &keyboard_read_callback;
    data->driver->disp = display;

    data->device = lv_indev_drv_register(data->driver);
    tt_check(data->device != NULL);

    tt_lvgl_keypad_set_indev(data->device);

    return data;
}

void keyboard_free(Keyboard keyboard) {
    KeyboardData* data = (KeyboardData*)keyboard;
    lv_indev_delete(data->device);
    free(data->driver);
    free(data);
}
