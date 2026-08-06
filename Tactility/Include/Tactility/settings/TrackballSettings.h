#pragma once

#include <lvgl/devices/trackball.h>

namespace tt::settings::trackball {

bool load(LvglTrackballSettings& settings);

LvglTrackballSettings loadOrGetDefault();

LvglTrackballSettings getDefault();

bool save(const LvglTrackballSettings& settings);

}
