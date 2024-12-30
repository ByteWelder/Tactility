#pragma once

#include "hal/Touch.h"
#include "TactilityCore.h"
#include "ft6x36/FT6X36.h"

class Core2Touch : public tt::hal::Touch {

private:

    lv_indev_t* _Nullable deviceHandle = nullptr;
    FT6X36 driver = FT6X36(I2C_NUM_0, GPIO_NUM_39);
    tt::Thread driverThread;
    bool interruptDriverThread = false;
    tt::Mutex mutex;

    lv_point_t lastPoint = { .x = 0, .y = 0 };
    lv_indev_state_t lastState = LV_INDEV_STATE_RELEASED;

    bool shouldInterruptDriverThread();

public:

    Core2Touch();

    bool start(lv_display_t* display) override;
    bool stop() override;

    void readLast(lv_indev_data_t* data);
    lv_indev_t* _Nullable getLvglIndev() override { return deviceHandle; }
    void driverThreadMain();
};
