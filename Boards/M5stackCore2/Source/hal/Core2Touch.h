#pragma once

#include "hal/Touch.h"
#include "TactilityCore.h"
#include "FT6X36.h"

class Core2Touch : public tt::hal::Touch {
private:
    lv_indev_t* _Nullable deviceHandle = nullptr;
    FT6X36 driver = FT6X36(I2C_NUM_0, GPIO_NUM_NC);
public:
    bool start(lv_display_t* display) override;
    bool stop() override;
    void read(lv_indev_data_t* data);
    lv_indev_t* _Nullable getLvglIndev() override { return deviceHandle; }
};
