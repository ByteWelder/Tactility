#pragma once

#include "../Device.h"
#include "NativeTouch.h"

#include <lvgl.h>

namespace tt::hal::touch {

class Display;

class TouchDevice : public Device {

public:

    Type getType() const override { return Type::Touch; }

    virtual bool start() = 0;
    virtual bool stop() = 0;

    virtual bool supportsLvgl() const { return false; }
    virtual bool startLvgl(lv_display_t* display) = 0;
    virtual bool stopLvgl() = 0;

    virtual lv_indev_t* _Nullable getLvglIndev() = 0;

    virtual std::shared_ptr<NativeTouch> _Nullable getNativeTouch() = 0;
};

}
