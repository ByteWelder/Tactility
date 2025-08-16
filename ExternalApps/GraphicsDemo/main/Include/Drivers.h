#pragma once

#include <tt_hal_display.h>
#include <tt_hal_touch.h>

class Drivers {

    DeviceId displayId = 0;
    DeviceId touchId = 0;

public:

    DisplayDriverHandle display = nullptr;
    TouchDriverHandle touch = nullptr;

    bool validateSupport();

    bool start();

    void stop();
};
