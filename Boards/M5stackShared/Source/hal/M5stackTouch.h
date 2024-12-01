#pragma once

#include "hal/Touch.h"
#include "TactilityCore.h"

class M5stackTouch : public tt::hal::Touch {
private:
    lv_indev_t* _Nullable deviceHandle = nullptr;
public:
    bool start(lv_display_t* display) override;
    bool stop() override;
    lv_indev_t* _Nullable getLvglIndev() override { return deviceHandle; }
};
