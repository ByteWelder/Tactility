// SPDX-License-Identifier: Apache-2.0
#include <lvgl/devices/trackball.h>
#include <lvgl/devices/device_context.h>
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

static void show_cursor(LvglTrackballCtx* ctx, lv_indev_t* indev) {
    if (ctx->cursor != nullptr || ctx->cursor_image_src == nullptr) {
        return;
    }

    ctx->cursor = lv_image_create(lv_layer_sys());
    if (ctx->cursor == nullptr) {
        return;
    }

    lv_obj_remove_flag(ctx->cursor, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(ctx->cursor, ctx->cursor_image_src);
    if (!ctx->settings.enabled) {
        lv_obj_add_flag(ctx->cursor, LV_OBJ_FLAG_HIDDEN);
    }
    lv_indev_set_cursor(indev, ctx->cursor);
}

static void hide_cursor(LvglTrackballCtx* ctx) {
    if (ctx->cursor == nullptr) {
        return;
    }

    // Deletes the object and detaches it from the indev.
    lv_obj_delete(ctx->cursor);
    ctx->cursor = nullptr;
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

struct LvglTrackballSettings lvgl_trackball_settings_get_default() {
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

    *out_indev = indev;
    return ERROR_NONE;
}

void lvgl_trackball_remove(lv_indev_t* indev) {
    if (indev == nullptr) {
        return;
    }

    auto* wrapper = static_cast<LvglDeviceContext*>(lv_indev_get_driver_data(indev));
    auto* ctx = static_cast<LvglTrackballCtx*>(wrapper->context);

    hide_cursor(ctx);
    lv_indev_delete(indev);
    delete wrapper;
}

error_t lvgl_trackball_set_settings(lv_indev_t* indev, const struct LvglTrackballSettings* settings) {
    if (indev == nullptr || settings == nullptr) {
        return ERROR_INVALID_ARGUMENT;
    }
    if (settings->encoder_sensitivity == 0 || settings->pointer_sensitivity == 0) {
        return ERROR_INVALID_ARGUMENT;
    }

    auto* wrapper = static_cast<LvglDeviceContext*>(lv_indev_get_driver_data(indev));
    auto* ctx = static_cast<LvglTrackballCtx*>(wrapper->context);
    bool mode_changed = ctx->settings.mode != settings->mode;
    ctx->settings = *settings;

    if (mode_changed) {
        if (settings->mode == LVGL_TRACKBALL_MODE_POINTER) {
            lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
            recenter_cursor(ctx, indev);
            show_cursor(ctx, indev);
        } else {
            hide_cursor(ctx);
            lv_indev_set_type(indev, LV_INDEV_TYPE_ENCODER);
        }
    }

    if (ctx->cursor != nullptr) {
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
    auto* ctx = static_cast<LvglTrackballCtx*>(wrapper->context);
    *out_settings = ctx->settings;
    return true;
}

void lvgl_trackball_set_cursor_image(lv_indev_t* indev, const void* image_src) {
    if (indev == nullptr) {
        return;
    }

    auto* wrapper = static_cast<LvglDeviceContext*>(lv_indev_get_driver_data(indev));
    auto* ctx = static_cast<LvglTrackballCtx*>(wrapper->context);
    ctx->cursor_image_src = image_src;

    if (ctx->settings.mode == LVGL_TRACKBALL_MODE_POINTER) {
        // Recreate so a changed (or newly-set/cleared) image takes effect immediately.
        hide_cursor(ctx);
        show_cursor(ctx, indev);
    }
}

} // extern "C"
