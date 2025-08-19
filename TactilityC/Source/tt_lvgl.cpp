#include <Tactility/lvgl/Lvgl.h>

extern "C" {

bool tt_lvgl_is_started() {
    return tt::lvgl::isStarted();
}

void tt_lvgl_start() {
    tt::lvgl::start();
}

void tt_lvgl_stop() {
    tt::lvgl::stop();
}

}
