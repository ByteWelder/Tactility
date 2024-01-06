#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "context.h"

typedef struct ViewPort ViewPort;

typedef enum {
    ViewPortOrientationHorizontal,
    ViewPortOrientationHorizontalFlip,
    ViewPortOrientationVertical,
    ViewPortOrientationVerticalFlip,
    ViewPortOrientationMAX, /**< Special value, don't use it */
} ViewPortOrientation;

/** ViewPort Draw callback
 * @warning    called from GUI thread
 */
typedef void (*ViewPortShowCallback)(Context* context, lv_obj_t* parent);
typedef void (*ViewPortHideCallback)(Context* context);

/** ViewPort allocator
 *
 * always returns view_port or stops system if not enough memory.
 *
 * @return     ViewPort instance
 */
ViewPort* view_port_alloc();

/** ViewPort deallocator
 *
 * Ensure that view_port was unregistered in GUI system before use.
 *
 * @param      view_port  ViewPort instance
 */
void view_port_free(ViewPort* view_port);

/** Enable or disable view_port rendering.
 *
 * @param      view_port  ViewPort instance
 * @param      enabled    Indicates if enabled
 * @warning    automatically dispatches update event
 */
void view_port_enabled_set(ViewPort* view_port, bool enabled);
bool view_port_is_enabled(const ViewPort* view_port);

/** ViewPort event callbacks
 *
 * @param      view_port  ViewPort instance
 * @param      on_show    Called to create LVGL widgets
 * @param      on_hide    Called before clearing the LVGL widget parent
 * @param      context    context to pass to callback
 */
void view_port_draw_callback_set(
    ViewPort* view_port,
    ViewPortShowCallback on_show,
    ViewPortHideCallback on_hide,
    Context* context
);
/** Emit update signal to GUI system.
 *
 * Rendering will happen later after GUI system process signal.
 *
 * @param      view_port  ViewPort instance
 */
void view_port_update(ViewPort* view_port);

#ifdef __cplusplus
}
#endif
