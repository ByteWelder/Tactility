// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <lvgl.h>

#include <tactility/error.h>
#include <tactility/freertos/freertos.h>
#include <tactility/freertos/task.h>

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t WindowId;

enum WindowState {
    /** id is the current topmost window and has live widgets. */
    WINDOW_STATE_GRANTED,
    /** id is not currently topmost - either buried under a newer window (its widgets don't
     * exist right now, but it may resurface and get rebuilt if everything above it is removed)
     * or it no longer exists at all (removed). */
    WINDOW_STATE_REVOKED,
};

/**
 * Called once by window_manager_start(), given the real root widget (a raw, full-size
 * container created directly under the default display's active screen). May add extra chrome
 * (e.g. a statusbar) as children of @a root_widget.
 * @param[in] root_widget the real root widget; owned by this module, deleted automatically
 * (along with everything added under it) by window_manager_stop()
 * @return the widget windows should actually be placed into - @a root_widget itself, or a
 * child of it. Returning NULL falls back to @a root_widget.
 * @warning Called on the LVGL task with the LVGL lock already held.
 */
typedef lv_obj_t* (*WindowManagerScreenInitFn)(lv_obj_t* root_widget);

/**
 * Configures the screen-init callback window_manager_start() invokes to build the root/content
 * widgets. Pass NULL to restore the default (no chrome - the raw root widget is used directly).
 * @warning Must be called before window_manager_start(); has no effect once already started.
 */
void window_manager_configure(WindowManagerScreenInitFn screen_init);

/**
 * Creates the root widget (under the default display's active screen) and, via the configured
 * screen-init callback, whatever chrome/content widget it wants around it. Idempotent - a
 * second call while already started is a no-op.
 * @retval ERROR_RESOURCE no default display is active (lv_screen_active() returned NULL)
 * @retval ERROR_NONE on success (including if already started)
 */
error_t window_manager_start(void);

/**
 * Deletes the root widget created by window_manager_start() (and everything under it - any
 * chrome plus whatever the topmost window had drawn), removing it from the display, and drops
 * every tracked window. Idempotent - a second call while already stopped is a no-op.
 */
error_t window_manager_stop(void);

/**
 * Called to populate a window's widgets: once by window_manager_create() when the window is
 * first created, and again later by window_manager_remove() if this window resurfaces as the
 * new topmost after whatever was above it is removed. Only the current topmost window ever has
 * live widgets - everything below it in the stack exists as tracked state only.
 * @param[in] root a fresh, full-size container created directly under the content widget for
 * this window; deleted automatically once this window stops being topmost
 * @param[in] user_data whatever was passed to window_manager_create() for this window
 * @warning Called on the LVGL task with the LVGL lock already held.
 * @warning May run on a different kernel thread than the one that called window_manager_create()
 * for this window - the rebuild-on-remove path runs on whichever thread called
 * window_manager_remove() for the window that used to be on top (e.g. a dialog's own thread as
 * it closes). Do NOT rely on thread_local state set by this window's own app thread; use
 * @a user_data instead.
 */
typedef void (*WindowCreateWidgetsFn)(lv_obj_t* root, void* user_data);

/**
 * Creates a new window on top of the stack (last created = topmost). Deletes the previously
 * topmost window's widgets (if any) and builds this window's widgets immediately via
 * @a create_widgets - only the topmost window ever has live widgets.
 * @param[in] user_data opaque; passed back to @a create_widgets on every call, including a
 * later rebuild triggered by window_manager_remove() - see its @warning about which thread that
 * can run on. Typically the calling app's own Context*.
 * @return the new window's id, or 0 if window_manager_start() hasn't been called
 */
WindowId window_manager_create(uint32_t app_instance_id, WindowCreateWidgetsFn create_widgets, void* user_data);

/**
 * Removes a window, wherever it is in the stack - not necessarily the topmost one. If it was
 * topmost, its widgets are deleted and whichever window is now on top (if any) has its
 * create_widgets called again to rebuild its widgets.
 */
void window_manager_remove(WindowId id);

/** @return the current state of @a id; WINDOW_STATE_REVOKED if @a id is buried or doesn't exist. */
enum WindowState window_manager_get_state(WindowId id);

/**
 * Blocks the calling task until @a id's state changes away from WINDOW_STATE_GRANTED, or
 * @a timeout elapses. Returns immediately with WINDOW_STATE_REVOKED if @a id isn't currently
 * topmost (nothing to wait for).
 * @return the state after waking (or immediately, if there was nothing to wait for)
 */
enum WindowState window_manager_await_state_change(WindowId id, TickType_t timeout);

#ifdef __cplusplus
}
#endif
