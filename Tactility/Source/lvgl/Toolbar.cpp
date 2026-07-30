#include <Tactility/lvgl/Toolbar.h>
#include <Tactility/app/AppManifest.h>

namespace tt::lvgl {

lv_obj_t* toolbar_create(lv_obj_t* parent, const app::AppContext& app) {
    return lvgl_toolbar_create(parent, app.getManifest().appName.c_str());
}

} // namespace
