#pragma once

#include <lvgl.h>

namespace tt::app::serialconsole {

class View {
public:
    virtual void onStop() = 0;
};

}