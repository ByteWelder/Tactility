#pragma once

#include "lvgl.h"

namespace tt::hal {

class Display;

class Touch {

public:
    [[nodiscard]] virtual bool start(lv_display_t* display) = 0;
    [[nodiscard]] virtual bool stop() = 0;

    [[nodiscard]] virtual lv_indev_t* _Nullable getLvglIndev() = 0;
};

}