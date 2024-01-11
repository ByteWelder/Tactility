#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "context.h"

/** ViewPort Draw callback
 * @warning    called from GUI thread
 */
typedef void (*ViewPortShowCallback)(Context* context, lv_obj_t* parent);
typedef void (*ViewPortHideCallback)(Context* context);

// TODO: Move internally, use handle publicly

typedef struct {
    Context* context;
    ViewPortShowCallback on_show;
    ViewPortHideCallback on_hide;
    bool app_toolbar;
} ViewPort;

/** ViewPort allocator
 *
 * always returns view_port or stops system if not enough memory.
 * @param context app Context
 * @param on_show Called to create LVGL widgets
 * @param on_hide Called before clearing the LVGL widget parent
 *
 * @return     ViewPort instance
 */
ViewPort* view_port_alloc(
    Context* context,
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

#ifdef __cplusplus
}
#endif
