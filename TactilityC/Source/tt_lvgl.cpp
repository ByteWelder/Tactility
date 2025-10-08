#include <tt_kernel.h>
#include <Tactility/lvgl/Lvgl.h>
#include <Tactility/lvgl/LvglSync.h>

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

void tt_lvgl_lock(TickType timeout) {
    tt::lvgl::getSyncLock()->lock(timeout);
}

void tt_lvgl_unlock() {
    tt::lvgl::getSyncLock()->unlock();
}

}