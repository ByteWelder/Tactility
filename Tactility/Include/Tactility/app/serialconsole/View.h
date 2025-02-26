#pragma once

#include <lvgl.h>

namespace tt::app::serialconsole {

class View {
public:

    virtual void onStart(lv_obj_t* parent) = 0;
    virtual void onStop() = 0;
};

}