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
typedef void (*ViewPortDrawCallback)(Context* context, lv_obj_t* parent);

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
 * @param      callback   appropriate callback function
 * @param      context    context to pass to callback
 */
void view_port_draw_callback_set(ViewPort* view_port, ViewPortDrawCallback callback, Context* context);
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
