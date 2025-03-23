#include "TouchCalibrationApp.h"
#include <Tactility/lvgl/Toolbar.h>
#include <Tactility/app/AppManifest.h>
#include <Tactility/Core.h>  // For tt::core::display
#include <esp_log.h>

static const char *TAG = "TouchCalibrationApp";

namespace tt::app {

void TouchCalibrationApp::onShow(AppContext& context, lv_obj_t* parent) {
    context_ = &context;
    parent_ = parent;

    // Create toolbar
    lv_obj_t* toolbar = tt::lvgl::toolbar_create(parent, context);
    lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);

    // Create label for instructions
    label_ = lv_label_create(parent);
    lv_obj_align(label_, LV_ALIGN_CENTER, 0, -20);

    // Create crosshair for touch target
    crosshair_ = lv_obj_create(parent);
    lv_obj_set_size(crosshair_, 20, 20);
    lv_obj_set_style_bg_color(crosshair_, lv_color_hex(0xFF0000), 0);  // Red crosshair
    lv_obj_set_style_border_width(crosshair_, 2, 0);
    lv_obj_set_style_border_color(crosshair_, lv_color_hex(0xFFFFFF), 0);

    // Register touch event callback
    lv_obj_add_event_cb(parent, touch_event_cb, LV_EVENT_PRESSED, this);

    // Start calibration for the first rotation
    current_rotation_ = LV_DISPLAY_ROTATION_0;
    current_point_ = 0;
    next_calibration_point();
}

void TouchCalibrationApp::onHide() {
    context_ = nullptr;
    parent_ = nullptr;
    label_ = nullptr;
    crosshair_ = nullptr;
}

void TouchCalibrationApp::touch_event_cb(lv_event_t* event) {
    auto* app = static_cast<TouchCalibrationApp*>(event->user_data);
    lv_point_t point;
    lv_indev_get_point(lv_indev_get_act(), &point);

    // Store the actual touch coordinates
    app->points_[app->current_point_].actual_x = point.x;
    app->points_[app->current_point_].actual_y = point.y;

    ESP_LOGI(TAG, "Calibration point %d: Expected (%d, %d), Actual (%d, %d)",
             app->current_point_,
             app->points_[app->current_point_].expected_x,
             app->points_[app->current_point_].expected_y,
             point.x, point.y);

    // Move to the next point
    app->current_point_++;
    if (app->current_point_ < 4) {
        app->next_calibration_point();
    } else {
        // Calculate offsets for the current rotation
        app->calculate_offsets();

        // Move to the next rotation
        if (app->current_rotation_ == LV_DISPLAY_ROTATION_0) {
            app->current_rotation_ = LV_DISPLAY_ROTATION_90;
            app->current_point_ = 0;
            tt::core::display::set_rotation(app->current_rotation_);
            app->next_calibration_point();
        } else if (app->current_rotation_ == LV_DISPLAY_ROTATION_90) {
            app->current_rotation_ = LV_DISPLAY_ROTATION_180;
            app->current_point_ = 0;
            tt::core::display::set_rotation(app->current_rotation_);
            app->next_calibration_point();
        } else if (app->current_rotation_ == LV_DISPLAY_ROTATION_180) {
            app->current_rotation_ = LV_DISPLAY_ROTATION_270;
            app->current_point_ = 0;
            tt::core::display::set_rotation(app->current_rotation_);
            app->next_calibration_point();
        } else {
            // All rotations done, save offsets and finish
            app->save_offsets();
            lv_label_set_text(app->label_, "Calibration Complete!");
            lv_obj_clear_flag(app->crosshair_, LV_OBJ_FLAG_HIDDEN);
            lv_obj_align(app->crosshair_, LV_ALIGN_CENTER, 0, 0);
        }
    }
}

void TouchCalibrationApp::next_calibration_point() {
    const char* rotation_names[] = {
        "Portrait (USB Bottom)",
        "Landscape (USB Right)",
        "Portrait Flipped (USB Top)",
        "Landscape Flipped (USB Left)"
    };

    // Define expected coordinates for each point based on rotation
    int32_t max_x = (current_rotation_ == LV_DISPLAY_ROTATION_90 || current_rotation_ == LV_DISPLAY_ROTATION_270) ? 319 : 239;
    int32_t max_y = (current_rotation_ == LV_DISPLAY_ROTATION_90 || current_rotation_ == LV_DISPLAY_ROTATION_270) ? 239 : 319;

    // Expected coordinates for the four points (top-left, top-right, bottom-left, bottom-right)
    points_[0] = {10, 10, 0, 0};                    // Top-left
    points_[1] = {max_x - 10, 10, 0, 0};            // Top-right
    points_[2] = {10, max_y - 10, 0, 0};            // Bottom-left
    points_[3] = {max_x - 10, max_y - 10, 0, 0};    // Bottom-right

    // Update UI
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "Calibrating %s\nTouch point %d", rotation_names[current_rotation_], current_point_ + 1);
    lv_label_set_text(label_, buffer);
    lv_obj_align(crosshair_, LV_ALIGN_TOP_LEFT, points_[current_point_].expected_x - 10, points_[current_point_].expected_y - 10);
}

void TouchCalibrationApp::calculate_offsets() {
    // Calculate average offsets for X and Y
    int32_t x_offset = 0, y_offset = 0;
    for (int i = 0; i < 4; i++) {
        x_offset += (points_[i].actual_x - points_[i].expected_x);
        y_offset += (points_[i].actual_y - points_[i].expected_y);
    }
    x_offset /= 4;
    y_offset /= 4;

    ESP_LOGI(TAG, "Rotation %d: X offset = %d, Y offset = %d", current_rotation_, x_offset, y_offset);

    // Store offsets in a global array (for simplicity; could use NVS in production)
    static int32_t touch_offsets[4][2];  // [rotation][x_offset, y_offset]
    touch_offsets[current_rotation_][0] = x_offset;
    touch_offsets[current_rotation_][1] = y_offset;
}

void TouchCalibrationApp::save_offsets() {
    // In a production app, save to NVS here
    ESP_LOGI(TAG, "Offsets saved (placeholder for NVS storage)");
}

}  // namespace tt::app

extern const AppManifest touch_calibration_app = {
    .id = "TouchCalibration",
    .name = "Touch Calibration",
    .createApp = create<TouchCalibrationApp>
};
