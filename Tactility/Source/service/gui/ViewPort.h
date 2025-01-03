#pragma once

#include "app/AppContext.h"
#include "lvgl.h"

namespace tt::service::gui {

/** ViewPort Draw callback
 * @warning    called from GUI thread
 */
typedef void (*ViewPortShowCallback)(app::AppContext& app, lv_obj_t* parent);
typedef void (*ViewPortHideCallback)(app::AppContext& app);

// TODO: Move internally, use handle publicly

typedef struct ViewPort {
    app::AppContext& app;
    ViewPortShowCallback onShow;
    ViewPortHideCallback _Nullable onHide;

    ViewPort(
        app::AppContext& app,
        ViewPortShowCallback on_show,
        ViewPortHideCallback _Nullable on_hide
    ) : app(app), onShow(on_show), onHide(on_hide) {}
} ViewPort;

/** ViewPort allocator
 *
 * always returns view_port or stops system if not enough memory.
 * @param app
 * @param on_show Called to create LVGL widgets
 * @param on_hide Called before clearing the LVGL widget parent
 *
 * @return     ViewPort instance
 */
ViewPort* view_port_alloc(
    app::AppContext& app,
    ViewPortShowCallback on_show,
    ViewPortHideCallback on_hide
);

/** ViewPort deallocator
 *
 * Ensure that view_port was unregistered in GUI system before use.
 *
 * @param      view_port  ViewPort instance
 */
void view_port_free(ViewPort* view_port);

} // namespace
