#pragma once

#include "app/AppInstance.h"
#include "lvgl.h"

namespace tt::service::gui {

// TODO: Move internally, use handle publicly

typedef struct ViewPort {
    app::AppInstance& app;

    ViewPort(app::AppInstance& app) : app(app) {}
} ViewPort;

/** ViewPort allocator
 * always returns view_port or stops system if not enough memory.
 * @param app
 * @return ViewPort instance
 */
ViewPort* view_port_alloc(app::AppInstance& app);

/** ViewPort destruction
 * Ensure that view_port was unregistered in GUI system before use.
 * @param viewPort ViewPort instance
 */
void view_port_free(ViewPort* viewPort);

} // namespace
