// SPDX-License-Identifier: Apache-2.0
#include <lvgl/devices/trackball.h>
#include <lvgl/devices/device_context.h>
#include <lvgl/devices/keyboard.h>
#include <lvgl/lvgl.h>

#include <tactility/drivers/trackball.h>

#include <cstdint>

constexpr auto* TAG = "lvgl_trackball";

struct LvglTrackballCtx {
    LvglTrackballSettings settings;
    int32_t cursor_x;
    int32_t cursor_y;
    lv_obj_t* cursor;
    const void* cursor_image_src;
};

static inline int32_t clamp(int32_t value, int32_t min_value, int32_t max_value) {
    if (value < min_value) return min_value;
    if (value > max_value) return max_value;
    return value;
}

static void recenter_cursor(LvglTrackballCtx* ctx, lv_indev_t* indev) {
    lv_display_t* display = lv_indev_get_display(indev);
    // lv_display_get_original_*_resolution() is the native (LV_DISPLAY_ROTATION_0) size,
    // unaffected by the display's current rotation, matching lvgl/devices/pointer.c's approach.
    ctx->cursor_x = display != nullptr ? lv_display_get_original_horizontal_resolution(display) / 2 : 0;
    ctx->cursor_y = display != nullptr ? lv_display_get_original_vertical_resolution(display) / 2 : 0;
}

// Creates the cursor object on first use and binds it to indev via lv_indev_set_cursor() exactly
// once - LVGL's lv_indev_set_cursor() unconditionally reparents whatever it's given (even NULL:
// it does not special-case that, see hide_cursor() below), so it must only ever be called here,
// with a freshly-created, non-null object whose parent already matches (lv_layer_sys()), making
// lv_obj_set_parent()'s "parent == obj->parent" check a safe no-op. Once bound, later image/
// enabled changes update the existing object in place instead of re-binding.
static void show_cursor(LvglTrackballCtx* ctx, lv_indev_t* indev) {
    if (ctx->cursor_image_src == nullptr) {
        return;
    }

    if (ctx->cursor == nullptr) {
        ctx->cursor = lv_image_create(lv_layer_sys());
        if (ctx->cursor == nullptr) {
            return;
        }
        lv_obj_remove_flag(ctx->cursor, LV_OBJ_FLAG_CLICKABLE);
        lv_indev_set_cursor(indev, ctx->cursor);
    }

    lv_image_set_src(ctx->cursor, ctx->cursor_image_src);
    if (ctx->settings.enabled) {
        lv_obj_remove_flag(ctx->cursor, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(ctx->cursor, LV_OBJ_FLAG_HIDDEN);
    }
}

// Only hides the cursor object - never deletes it
static void hide_cursor(LvglTrackballCtx* ctx) {
    if (ctx->cursor == nullptr) {
        return;
    }
    lv_obj_add_flag(ctx->cursor, LV_OBJ_FLAG_HIDDEN);
}

static void lvgl_trackball_read_cb(lv_indev_t* indev, lv_indev_data_t* data) {
    auto* wrapper = static_cast<LvglDeviceContext*>(lv_indev_get_driver_data(indev));
    auto* ctx = static_cast<LvglTrackballCtx*>(wrapper->context);

    // Always drain accumulated movement so it doesn't jump on re-enable, but discard it while disabled.
    int32_t dx = 0;
    int32_t dy = 0;
    trackball_read_delta(wrapper->device, &dx, &dy);
    if (!ctx->settings.enabled) {
        dx = 0;
        dy = 0;
    }

    lv_display_t* display = lv_indev_get_display(indev);

    if (ctx->settings.mode == LVGL_TRACKBALL_MODE_ENCODER) {
        int32_t ticks = (dx + dy) * static_cast<int32_t>(ctx->settings.encoder_sensitivity);
        data->enc_diff = static_cast<int16_t>(clamp(ticks, INT16_MIN, INT16_MAX));
        if (ticks != 0) {
            lv_display_trigger_activity(display);
        }
    } else {
        int32_t max_x = display != nullptr ? lv_display_get_original_horizontal_resolution(display) - 1 : 0;
        int32_t max_y = display != nullptr ? lv_display_get_original_vertical_resolution(display) - 1 : 0;
        ctx->cursor_x = clamp(ctx->cursor_x + dx * static_cast<int32_t>(ctx->settings.pointer_sensitivity), 0, max_x);
        ctx->cursor_y = clamp(ctx->cursor_y + dy * static_cast<int32_t>(ctx->settings.pointer_sensitivity), 0, max_y);
        data->point.x = static_cast<int16_t>(ctx->cursor_x);
        data->point.y = static_cast<int16_t>(ctx->cursor_y);
    }

    bool pressed = false;
    if (ctx->settings.enabled) {
        trackball_get_button_pressed(wrapper->device, &pressed);
    }
    data->state = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;

    if (pressed) {
        lv_display_trigger_activity(display);
    }
}

extern "C" {

LvglTrackballSettings lvgl_trackball_settings_get_default() {
    return LvglTrackballSettings {
        .mode = LVGL_TRACKBALL_MODE_ENCODER,
        .enabled = true,
        .encoder_sensitivity = 1,
        .pointer_sensitivity = 10,
    };
}

error_t lvgl_trackball_add(struct Device* device, lv_display_t* display, lv_indev_t** out_indev) {
    if (device == nullptr || out_indev == nullptr) {
        return ERROR_INVALID_ARGUMENT;
    }
    if (device_get_type(device) != &TRACKBALL_TYPE) {
        return ERROR_INVALID_ARGUMENT;
    }

    auto* ctx = new(std::nothrow) LvglTrackballCtx();
    if (ctx == nullptr) {
        return ERROR_OUT_OF_MEMORY;
    }
    ctx->settings = lvgl_trackball_settings_get_default();

    auto* wrapper = new(std::nothrow) LvglDeviceContext(ctx);
    if (wrapper == nullptr) {
        delete ctx;
        return ERROR_OUT_OF_MEMORY;
    }
    wrapper->device = device;

    lv_indev_t* indev = lv_indev_create();
    if (indev == nullptr) {
        delete wrapper;
        return ERROR_OUT_OF_MEMORY;
    }

    lv_indev_set_type(indev, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_read_cb(indev, lvgl_trackball_read_cb);
    lv_indev_set_driver_data(indev, wrapper);
    if (display != nullptr) {
        lv_indev_set_display(indev, display);
    }
    recenter_cursor(ctx, indev);

    // Encoder indevs are useless without a group (LVGL dispatch bails out immediately if indev->group is NULL)
    // Use the keyboard group for now, until it's refactored into a proper global/shared group. See ideas.md
    // When refactoring this, you probably want to remove the calls to lvgl_keyboard_* below.
    if (ctx->settings.mode == LVGL_TRACKBALL_MODE_ENCODER) {
        lvgl_keyboard_enable(indev);
    }

    *out_indev = indev;
    return ERROR_NONE;
}

void lvgl_trackball_remove(lv_indev_t* indev) {
    if (indev == nullptr) {
        return;
    }

    auto* wrapper = static_cast<LvglDeviceContext*>(lv_indev_get_driver_data(indev));
    check(wrapper);
    auto* ctx = static_cast<LvglTrackballCtx*>(wrapper->context);
    check(ctx);
    if (ctx->cursor != nullptr) {
        lv_obj_delete(ctx->cursor);
    }
    lv_indev_delete(indev);
    delete wrapper;
}

error_t lvgl_trackball_set_settings(lv_indev_t* indev, const struct LvglTrackballSettings* settings) {
    if (indev == nullptr || settings == nullptr) {
        return ERROR_INVALID_ARGUMENT;
    }

    if (
        (settings->mode != LVGL_TRACKBALL_MODE_ENCODER && settings->mode != LVGL_TRACKBALL_MODE_POINTER) ||
        settings->encoder_sensitivity == 0 ||
        settings->pointer_sensitivity == 0
    ) {
        return ERROR_INVALID_ARGUMENT;
    }

    auto* wrapper = static_cast<LvglDeviceContext*>(lv_indev_get_driver_data(indev));
    check(wrapper);
    auto* ctx = static_cast<LvglTrackballCtx*>(wrapper->context);
    check(ctx);
    bool mode_changed = ctx->settings.mode != settings->mode;
    ctx->settings = *settings;

    if (mode_changed) {
        if (settings->mode == LVGL_TRACKBALL_MODE_POINTER) {
            lvgl_keyboard_disable(indev);
            lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
            recenter_cursor(ctx, indev);
            show_cursor(ctx, indev);
        } else {
            hide_cursor(ctx);
            lv_indev_set_type(indev, LV_INDEV_TYPE_ENCODER);
            lvgl_keyboard_enable(indev);
        }
    }

    // Cursor visibility only tracks the enabled toggle in pointer mode - in encoder mode it must
    // stay hidden regardless of enabled, otherwise this unconditionally un-hides the cursor
    // hide_cursor() just hid above (enabled is independent of mode, and defaults to true).
    if (ctx->cursor != nullptr && ctx->settings.mode == LVGL_TRACKBALL_MODE_POINTER) {
        if (ctx->settings.enabled) {
            lv_obj_remove_flag(ctx->cursor, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(ctx->cursor, LV_OBJ_FLAG_HIDDEN);
        }
    }

    return ERROR_NONE;
}

bool lvgl_trackball_get_settings(lv_indev_t* indev, struct LvglTrackballSettings* out_settings) {
    if (indev == nullptr || out_settings == nullptr) {
        return false;
    }

    auto* wrapper = static_cast<LvglDeviceContext*>(lv_indev_get_driver_data(indev));
    check(wrapper);
    auto* ctx = static_cast<LvglTrackballCtx*>(wrapper->context);
    check(ctx);
    *out_settings = ctx->settings;
    return true;
}

void lvgl_trackball_set_cursor_image(lv_indev_t* indev, const void* image_src) {
    if (indev == nullptr) {
        return;
    }

    auto* wrapper = static_cast<LvglDeviceContext*>(lv_indev_get_driver_data(indev));
    check(wrapper);
    auto* ctx = static_cast<LvglTrackballCtx*>(wrapper->context);
    check(ctx);
    ctx->cursor_image_src = image_src;

    if (ctx->settings.mode == LVGL_TRACKBALL_MODE_POINTER) {
        if (image_src == nullptr) {
            hide_cursor(ctx);
        } else {
            show_cursor(ctx, indev);
        }
    }
}

} // extern "C"
