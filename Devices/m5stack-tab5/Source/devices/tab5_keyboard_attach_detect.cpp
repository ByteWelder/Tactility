#include "tab5_keyboard_attach_detect.h"

#include "tab5_keyboard.h"

#include <tactility/device.h>
#include <tactility/log.h>

#include <lvgl/lvgl.h>
#include <lvgl.h>

#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>

constexpr auto* TAG = "Tab5";

// Hot-plug attach-state check interval. I2C probes can false-positive on a floating/half-connected
// bus (e.g. mid-unplug), so a state change is only acted on once it's seen on two consecutive
// checks in a row.
constexpr auto ATTACH_CHECK_INTERVAL_MS = 1000;

static TimerHandle_t attach_detect_timer = nullptr;

static bool was_attached = false;
static bool pending_attach_state = false;
static uint8_t pending_attach_confirm_count = 0;

// Tracks LVGL's own readiness so a restart (lvgl_is_running() going from false back to true -
// e.g. an app that took over the display for direct rendering, stopping and letting LVGL rebind)
// can be told apart from the keyboard itself attaching/detaching. See apply_state()'s comment for
// why that distinction matters.
static bool was_lvgl_ready = false;

static lv_display_rotation_t saved_rotation = LV_DISPLAY_ROTATION_0;
static bool rotation_override_active = false;

// Applies the current attach state to LVGL (landscape rotation while attached, restoring the
// prior rotation on detach unless the user changed it manually since attaching) and to the
// keyboard device's own register state (reinit on attach - RGB mode/interrupt config are volatile
// across an unplug/replug on this chip). Ported as-is from the deprecated HAL's
// Tab5Keyboard::applyAutoRotation() / the pre-refactor tab5_keyboard.cpp driver logic.
// @return true once handled; false to be retried on the next tick (e.g. LVGL lock busy).
static bool apply_state(Device* keyboard_device, bool attached) {
    if (!lvgl_try_lock(pdMS_TO_TICKS(100))) {
        return false; // retry next tick
    }

    // Resolved inside the lock, not before: the default display can start/stop between an
    // unlocked probe and actually taking the lock, and lv_display_get_default() itself isn't
    // safe to call without holding it (unlike lvgl_is_running(), used for the readiness check in
    // attach_detect_callback()).
    auto* display = lv_display_get_default();
    if (display == nullptr) {
        lvgl_unlock();
        return false; // LVGL not ready yet - retry next tick
    }

    if (attached) {
        tab5_keyboard_reinit(keyboard_device);

        if (lv_display_get_rotation(display) != LV_DISPLAY_ROTATION_90) {
            saved_rotation = lv_display_get_rotation(display);
            rotation_override_active = true;
            lv_display_set_rotation(display, LV_DISPLAY_ROTATION_90);
        }
    } else {
        // Only restore if rotation is still what we set it to - if the user manually changed it
        // since attaching, respect their choice instead.
        if (rotation_override_active && lv_display_get_rotation(display) == LV_DISPLAY_ROTATION_90) {
            lv_display_set_rotation(display, saved_rotation);
        }
        rotation_override_active = false;
    }

    lvgl_unlock();
    return true;
}

static void attach_detect_callback(TimerHandle_t /*timer*/) {
    Device* keyboard_device = nullptr;
    if (device_get_by_name("keyboard0", &keyboard_device) != ERROR_NONE) {
        return; // Not constructed yet - will retry on next tick
    }

    // LVGL restarting is a distinct event from the keyboard physically attaching/detaching: the
    // accessory may never have moved, but whatever apply_state() last set (rotation) may have
    // been reset in the meantime by the restart. Forcing was_attached false makes the block below
    // see a fresh "attached" transition (still going through the normal 2-check debounce) so
    // apply_state() re-announces the current state instead of staying silent forever, waiting for
    // an edge that will never come because the keyboard was never actually unplugged.
    // lvgl_is_running() is safe to call unlocked (unlike lv_display_get_default(), resolved inside
    // the lock in apply_state() instead).
    const bool lvgl_ready = lvgl_is_running();
    if (lvgl_ready && !was_lvgl_ready) {
        was_attached = false;
        pending_attach_confirm_count = 0;
    }
    was_lvgl_ready = lvgl_ready;

    const bool attached = tab5_keyboard_is_attached(keyboard_device);
    if (attached != was_attached) {
        // Require the new state to be confirmed on a second consecutive check before acting - a
        // single probe on a floating/half-connected bus (e.g. mid-unplug) can false-positive.
        if (attached != pending_attach_state || pending_attach_confirm_count == 0) {
            pending_attach_state = attached;
            pending_attach_confirm_count = 1;
        } else {
            pending_attach_confirm_count = 0;
            if (apply_state(keyboard_device, attached)) {
                was_attached = attached;
            }
            // else: not handled yet (e.g. LVGL lock busy) - retry on the next confirmed check
        }
    } else {
        pending_attach_confirm_count = 0;
    }

    device_put(keyboard_device);
}

void tab5_keyboard_attach_detect_start() {
    if (attach_detect_timer != nullptr) {
        LOG_W(TAG, "keyboard attach-detect timer already running");
        return;
    }

    was_attached = false;
    pending_attach_confirm_count = 0;
    was_lvgl_ready = false;
    rotation_override_active = false;

    attach_detect_timer = xTimerCreate("kb_attach_detect", pdMS_TO_TICKS(ATTACH_CHECK_INTERVAL_MS), pdTRUE, nullptr, attach_detect_callback);
    if (!attach_detect_timer) {
        LOG_E(TAG, "Failed to create keyboard attach-detect timer");
        return;
    }
    if (xTimerStart(attach_detect_timer, pdMS_TO_TICKS(100)) != pdPASS) {
        LOG_E(TAG, "Failed to start keyboard attach-detect timer");
        xTimerDelete(attach_detect_timer, pdMS_TO_TICKS(100));
        attach_detect_timer = nullptr;
    }
}

void tab5_keyboard_attach_detect_stop() {
    if (attach_detect_timer == nullptr) {
        return;
    }

    if (xTimerStop(attach_detect_timer, pdMS_TO_TICKS(100)) != pdPASS) {
        LOG_W(TAG, "Failed to stop keyboard attach-detect timer");
    }
    if (xTimerDelete(attach_detect_timer, pdMS_TO_TICKS(100)) != pdPASS) {
        LOG_E(TAG, "Failed to delete keyboard attach-detect timer");
    }
    // Always clear the handle - stale non-null handle is worse than a resource leak, as it would
    // cause tab5_keyboard_attach_detect_start() to silently skip re-creating the timer.
    attach_detect_timer = nullptr;
}
