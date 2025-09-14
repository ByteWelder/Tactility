#include "CardputerEncoder.h"

void CardputerEncoder::readCallback(lv_indev_t* indev, lv_indev_data_t* data) {
    CardputerEncoder* self = static_cast<CardputerEncoder*>(lv_indev_get_user_data(indev));

    self->keyboard.updateKeyList();

    data->state = LV_INDEV_STATE_RELEASED;

    if (self->keyboard.keyList().size() != self->lastKeyNum) {
        // If key pressed
        if (self->keyboard.keyList().size() != 0) {
            // Update states and values
            self->keyboard.updateKeysState();
            if (self->keyboard.keysState().fn) {
                if (self->keyboard.keysState().enter) {
                    data->key = LV_KEY_ENTER;
                    data->state = LV_INDEV_STATE_PRESSED;
                } else {
                    for (auto& i : self->keyboard.keysState().values) {
                        if (i == ';') { // Up
                            data->enc_diff = -1;
                        } else if (i == '.') { // Down
                            data->enc_diff = 1;
                        }
                        break; // We only care about the first value
                    }
                }
            }
            self->lastKeyNum = self->keyboard.keyList().size();
        } else {
            self->lastKeyNum = 0;
        }
    }
}

bool CardputerEncoder::startLvgl(lv_display_t* display) {
    keyboard.init();

    lvglDevice = lv_indev_create();
    lv_indev_set_type(lvglDevice, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_read_cb(lvglDevice, &readCallback);
    lv_indev_set_display(lvglDevice, display);
    lv_indev_set_user_data(lvglDevice, this);

    return true;
}

bool CardputerEncoder::stopLvgl() {
    lv_indev_delete(lvglDevice);
    lvglDevice = nullptr;

    return true;
}
