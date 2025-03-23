#pragma once

#include <Tactility/app/App.h>
#include <lvgl.h>

namespace tt::app {

class TouchCalibrationApp : public App {
public:
    void onShow(AppContext& context, lv_obj_t* parent) override;
    void onHide(AppContext& appContext) override;  // Updated signature

private:
    static void touch_event_cb(lv_event_t* event);
    static void reset_button_cb(lv_event_t* event);  // Callback for reset button
    void next_calibration_point();
    void calculate_offsets();
    void save_offsets();
    void reset_calibration();  // Method to reset calibration data

    AppContext* context_ = nullptr;
    lv_obj_t* parent_ = nullptr;
    lv_obj_t* label_ = nullptr;
    lv_obj_t* crosshair_ = nullptr;

    // Calibration state
    lv_display_rotation_t current_rotation_ = LV_DISPLAY_ROTATION_0;
    int current_point_ = 0;  // 0 to 3 (top-left, top-right, bottom-left, bottom-right)
    struct CalibrationPoint {
        int32_t expected_x;
        int32_t expected_y;
        int32_t actual_x;
        int32_t actual_y;
    };
    CalibrationPoint points_[4];  // Store touch data for each point
};

}  // namespace tt::app
