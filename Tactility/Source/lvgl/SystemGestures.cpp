#include <Tactility/lvgl/SystemGestures.h>

#ifdef ESP_PLATFORM
#include <sdkconfig.h>
#endif

#if defined(CONFIG_TT_SYSTEM_GESTURES)

#include <Tactility/TactilityPrivate.h>
#include <Tactility/lvgl/SystemBars.h>
#include <Tactility/lvgl/SystemRecents.h>

#include <app/event.h>
#include <app/manager.h>
#include <app/manifest.h>

#include <lvgl/devices/indev.h>
#include <lvgl/lvgl.h>

#include <lvgl_window_manager/window_manager.h>

#include <cstdint>
#include <cstring>

namespace tt::lvgl {

namespace {

constexpr auto* LAUNCHER_APP_ID = "tactility.launcher";

/** How long a bottom-edge swipe has to stay down before the recents switcher opens instead of going home. */
constexpr uint32_t HOLD_TIME_MS = 500;
// How often the hold timer checks whether the swipe turned into a hold. It only exists while such a
// press is down.
constexpr uint32_t HOLD_POLL_MS = 50;

// The most windows the home gesture looks through for the launcher. Bounded, so the stack isn't
// copied to the heap from an LVGL callback.
constexpr size_t WINDOW_SCAN_LIMIT = 16;

// One press at a time: a second finger is ignored, which is what a single-touch panel reports anyway
struct GestureState {
    lv_indev_t* indev = nullptr;
    SystemGestureEdge edge = SystemGestureEdge::None;
    lv_point_t press = { 0, 0 };
    uint32_t press_tick = 0;
    bool hold_triggered = false;
    lv_timer_t* hold_timer = nullptr;
};

GestureState state;
bool attached = false;

void raise_on_main_dispatcher(void* context) {
    window_manager_raise(static_cast<WindowId>(reinterpret_cast<uintptr_t>(context)));
}

/** Defers to the main dispatcher: window_manager_raise() must not run on the LVGL task, see its warning. */
void raiseWindow(WindowId window) {
    dispatchOnMainDispatcher(raise_on_main_dispatcher, reinterpret_cast<void*>(static_cast<uintptr_t>(window)));
}

WindowId findLauncherWindow() {
    WindowInfo windows[WINDOW_SCAN_LIMIT];
    size_t count = window_manager_get_windows(windows, 0, WINDOW_SCAN_LIMIT);
    if (count > WINDOW_SCAN_LIMIT) {
        count = WINDOW_SCAN_LIMIT;
    }

    char app_id[APP_MANIFEST_ID_LENGTH + 1];
    for (size_t i = 0; i < count; i++) {
        if (app_manager_get_instance_app_id(windows[i].app_instance_id, app_id, sizeof(app_id)) != ERROR_NONE) {
            continue;
        }
        if (strcmp(app_id, LAUNCHER_APP_ID) == 0) {
            return windows[i].id;
        }
    }
    return 0;
}

/** Home: shows the launcher again, keeping the apps above it in the stack. */
void showHome() {
    const WindowId launcher = findLauncherWindow();
    if (launcher != 0) {
        raiseWindow(launcher);
    }
}

/** Back: closes the topmost app, which brings back the window below it. No-op on the launcher. */
void closeTopmostApp() {
    char app_id[APP_MANIFEST_ID_LENGTH + 1];
    if (app_manager_get_topmost_app_id(app_id, sizeof(app_id)) != ERROR_NONE) {
        return;
    }
    if (strcmp(app_id, LAUNCHER_APP_ID) == 0) {
        return;
    }

    AppInstanceId topmost = 0;
    if (app_manager_get_topmost_instance_id(&topmost) == ERROR_NONE) {
        app_event_emit_close(topmost);
    }
}

void stopHoldTimer() {
    if (state.hold_timer != nullptr) {
        lv_timer_delete(state.hold_timer);
        state.hold_timer = nullptr;
    }
}

// Keeps checking a bottom-edge press: a swipe up that keeps being held opens the recents switcher
void onHoldTimer(lv_timer_t* timer) {
    if (state.indev == nullptr) {
        lv_timer_pause(timer);
        return;
    }
    if (lv_tick_elaps(state.press_tick) < HOLD_TIME_MS) {
        return;
    }

    lv_point_t point;
    lv_indev_get_point(state.indev, &point);
    if (!systemGestureIsSwipeUp(state.press.x, state.press.y, point.x, point.y)) {
        // Still down, but not swiped up far enough yet
        return;
    }

    // The finger is still down: the release must not click whatever is under it
    state.hold_triggered = true;
    lv_timer_pause(timer);
    lv_indev_wait_release(state.indev);
    systemRecentsShow();
}

void clearPress() {
    stopHoldTimer();
    state.indev = nullptr;
    state.edge = SystemGestureEdge::None;
    state.hold_triggered = false;
}

lv_display_t* getIndevDisplay(lv_indev_t* indev) {
    lv_display_t* display = lv_indev_get_display(indev);
    return display != nullptr ? display : lv_display_get_default();
}

void onIndevPressed(lv_event_t* event) {
    auto* indev = static_cast<lv_indev_t*>(lv_event_get_current_target(event));
    if (state.indev != nullptr) {
        return;
    }
    // A shown overlay (system menu, recents) has its own gestures
    if (window_manager_get_active_layer() != WINDOW_LAYER_APP) {
        return;
    }
    lv_display_t* display = getIndevDisplay(indev);
    if (display == nullptr) {
        return;
    }

    lv_point_t point;
    lv_indev_get_point(indev, &point);
    const SystemGestureEdge edge = systemGestureEdgeAt(
        point.x,
        point.y,
        lv_display_get_horizontal_resolution(display),
        lv_display_get_vertical_resolution(display)
    );
    if (edge == SystemGestureEdge::None) {
        return;
    }
    // In the side layout, the statusbar strip already owns the left edge
    if (edge == SystemGestureEdge::Left && systemBarsGetLayout() == settings::launcher::SystemBarsLayout::Side) {
        return;
    }

    state.indev = indev;
    state.edge = edge;
    state.press = point;
    state.press_tick = lv_tick_get();
    state.hold_triggered = false;

    if (edge == SystemGestureEdge::Bottom) {
        state.hold_timer = lv_timer_create(onHoldTimer, HOLD_POLL_MS, nullptr);
    }
}

void onIndevReleased(lv_event_t* event) {
    auto* indev = static_cast<lv_indev_t*>(lv_event_get_current_target(event));
    if (state.indev == nullptr || indev != state.indev) {
        return;
    }

    const SystemGestureEdge edge = state.edge;
    const lv_point_t press = state.press;
    const bool hold_triggered = state.hold_triggered;
    clearPress();

    if (hold_triggered) {
        return;
    }

    lv_point_t point;
    lv_indev_get_point(indev, &point);

    if (edge == SystemGestureEdge::Bottom) {
        if (systemGestureIsSwipeUp(press.x, press.y, point.x, point.y)) {
            // The release must not click the widget below the pointer
            lv_indev_reset(indev, nullptr);
            showHome();
        }
    } else if (edge == SystemGestureEdge::Left) {
        if (systemGestureIsSwipeRight(press.x, press.y, point.x, point.y)) {
            lv_indev_reset(indev, nullptr);
            closeTopmostApp();
        }
    }
}

void onIndevPressLost(lv_event_t* /*event*/) {
    clearPress();
}

void forEachPointer(void (*action)(lv_indev_t* indev)) {
    for (lv_indev_t* indev = lv_indev_get_next(nullptr); indev != nullptr; indev = lv_indev_get_next(indev)) {
        if (lv_indev_get_type(indev) == LV_INDEV_TYPE_POINTER) {
            action(indev);
        }
    }
}

void removePointerCallbacks(lv_indev_t* indev) {
    lv_indev_remove_event_cb_with_user_data(indev, onIndevPressed, nullptr);
    lv_indev_remove_event_cb_with_user_data(indev, onIndevReleased, nullptr);
    lv_indev_remove_event_cb_with_user_data(indev, onIndevPressLost, nullptr);
}

// The pointer devices are removed by LVGL itself on shutdown, so detaching only has to undo the
// callbacks for the ones that are still there.
void addPointerCallbacks(lv_indev_t* indev) {
    removePointerCallbacks(indev);
    lv_indev_add_event_cb(indev, onIndevPressed, LV_EVENT_PRESSED, nullptr);
    lv_indev_add_event_cb(indev, onIndevReleased, LV_EVENT_RELEASED, nullptr);
    lv_indev_add_event_cb(indev, onIndevPressLost, LV_EVENT_PRESS_LOST, nullptr);
}

}

void systemGesturesAttach() {
    if (attached) {
        return;
    }
    // Keypad/encoder-only devices keep behaving exactly as before
    if (!lvgl_indev_exists(LV_INDEV_TYPE_POINTER)) {
        return;
    }
    // shortcut: pointer devices added after boot (a USB HID mouse) don't get the system gestures.
    // Covers every touch panel, which exists before this runs.
    forEachPointer(addPointerCallbacks);
    attached = true;
}

void systemGesturesDetach() {
    if (!attached) {
        return;
    }
    clearPress();
    forEachPointer(removePointerCallbacks);
    attached = false;
}

}

#else

namespace tt::lvgl {

void systemGesturesAttach() {}
void systemGesturesDetach() {}

}

#endif
