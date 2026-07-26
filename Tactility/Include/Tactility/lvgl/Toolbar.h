#pragma once

#include "../app/AppContext.h"

#include <lvgl/widgets/toolbar.h>

namespace tt::lvgl {

/** Create a toolbar widget that shows the app name as title */
lv_obj_t* toolbar_create(lv_obj_t* parent, const app::AppContext& app);

} // namespace
