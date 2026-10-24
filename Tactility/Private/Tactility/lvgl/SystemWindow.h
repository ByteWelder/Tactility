#pragma once

#include <Tactility/TactilityPrivate.h>

#include <app/instance.h>

#include <lvgl_window_manager/window_manager.h>

#include <stdint.h>

namespace tt::lvgl {

/** A window id as an opaque event context, and back. Window ids never reach a pointer's size. */
inline void* toWindowContext(WindowId window) {
    return reinterpret_cast<void*>(static_cast<uintptr_t>(window));
}

inline WindowId fromWindowContext(const void* context) {
    return static_cast<WindowId>(reinterpret_cast<uintptr_t>(context));
}

/**
 * Calls @a action for every window, bottom-most first, without copying the whole stack: the stack
 * is unbounded, and these run on the LVGL task.
 * @param[in] action called with each window; return false to stop
 */
template <typename Action>
void forEachWindow(Action&& action) {
    constexpr size_t CHUNK_SIZE = 8;
    WindowInfo windows[CHUNK_SIZE];
    const size_t total = window_manager_get_windows(nullptr, 0, 0);
    for (size_t offset = 0; offset < total; offset += CHUNK_SIZE) {
        const size_t count = window_manager_get_windows(windows, offset, CHUNK_SIZE);
        for (size_t index = 0; index < count && index < CHUNK_SIZE; index++) {
            if (!action(windows[index])) {
                return;
            }
        }
    }
}

/** @return the topmost window, or a window with id WINDOW_ID_NONE when no window exists */
inline WindowInfo getTopWindow() {
    WindowInfo window = { WINDOW_ID_NONE, 0 };
    const size_t total = window_manager_get_windows(nullptr, 0, 0);
    if (total != 0) {
        window_manager_get_windows(&window, total - 1, 1);
    }
    return window;
}

/**
 * Raises a window off the LVGL task.
 * @warning window_manager_raise() takes the window manager's lifecycle mutex, which is held while
 * windows are built; calling it from an LVGL event callback can deadlock against a concurrent
 * create/remove that is waiting for the LVGL lock. Every raise goes through here for that reason.
 */
inline void raiseWindow(WindowId window) {
    dispatchOnMainDispatcher([](void* context) {
        window_manager_raise(fromWindowContext(context));
    }, toWindowContext(window));
}

}
