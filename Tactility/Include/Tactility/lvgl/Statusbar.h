#pragma once

#include <Tactility/app/AppContext.h>

#include <lvgl.h>

namespace tt::lvgl {

constexpr auto STATUSBAR_ICON_LIMIT = 8;
constexpr auto STATUSBAR_ICON_SIZE = 20;
constexpr auto STATUSBAR_HEIGHT = STATUSBAR_ICON_SIZE + 2;

lv_obj_t* statusbar_create(lv_obj_t* parent);
int8_t statusbar_icon_add(const std::string& image);
int8_t statusbar_icon_add();
void statusbar_icon_remove(int8_t id);
void statusbar_icon_set_image(int8_t id, const std::string& image);
void statusbar_icon_set_visibility(int8_t id, bool visible);

} // namespace
