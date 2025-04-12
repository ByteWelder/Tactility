#ifdef ESP_PLATFORM
#include "esp_log.h"


#include <Tactility/app/App.h>
#include <Tactility/app/AppManifest.h>
#include <Tactility/lvgl/Toolbar.h>
#include <lvgl.h>

#ifdef CONFIG_TT_BOARD_CYD_2432S028R
#include "../../../Boards/CYD-2432S028R/Source/hal/YellowDisplayConstants.h"
#include "../../../Drivers/XPT2046-SoftSPI/XPT2046_TouchscreenSOFTSPI.h"
#endif

using namespace tt::app;

class Calibration : public App {
public:
    void onShow(AppContext& context, lv_obj_t* parent) override {
#ifdef CONFIG_TT_BOARD_CYD_2432S028R
        ESP_LOGI("Calibration", "Starting calibration on CYD-2432S028R");

        toolbar = tt::lvgl::toolbar_create(parent, context);
        lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_add_flag(toolbar, LV_OBJ_FLAG_HIDDEN);

        label = lv_label_create(parent);
        updateScreen("Tap the top-left corner");
        drawCrosshair(20, 20);
        lv_obj_add_event_cb(lv_scr_act(), eventCallback, LV_EVENT_PRESSED, this);
#else
        #ifdef ESP_PLATFORM
        ESP_LOGI("Calibration", "Calibration not supported on this board");
        #endif
        toolbar = tt::lvgl::toolbar_create(parent, context);
        lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);
        label = lv_label_create(parent);
        lv_label_set_text(label, "Calibration only supported\non CYD-2432S028R");
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
#endif
    }

    void onHide(AppContext& /*context*/) override {
        #ifdef ESP_PLATFORM
        ESP_LOGI("Calibration", "Hiding calibration");
        #endif
        if (label) {
            lv_obj_del(label);
            label = nullptr;
        }
        if (crosshair) {
            lv_obj_del(crosshair);
            crosshair = nullptr;
        }
        toolbar = nullptr;
    }

private:
#ifdef CONFIG_TT_BOARD_CYD_2432S028R
    static void eventCallback(lv_event_t* e) {
        Calibration* app = static_cast<Calibration*>(lv_event_get_user_data(e));
        uint16_t rawX, rawY;
        extern XPT2046_TouchscreenSOFTSPI<CYD_TOUCH_MISO_PIN, CYD_TOUCH_MOSI_PIN, CYD_TOUCH_SCK_PIN, 0> touch;
        touch.getRawTouch(rawX, rawY);

        if (rawX == 0 || rawY == 0) return;  // Ignore invalid touches

        app->logTouchData(rawX, rawY);
        app->step++;

        switch (app->step) {
            case 1:
                app->updateScreen("Tap the bottom-right corner");
                if (app->crosshair) lv_obj_del(app->crosshair);
                app->drawCrosshair(220, 300);
                break;
            case 2: {
                app->updateScreen("Calibration complete!");
                lv_obj_clear_flag(app->toolbar, LV_OBJ_FLAG_HIDDEN);
                if (app->crosshair) lv_obj_del(app->crosshair);
                app->crosshair = nullptr;

                // Compute calibration
                CalibrationData cal;
                float dxRaw = app->rawX[1] - app->rawX[0];
                float dyRaw = app->rawY[1] - app->rawY[0];
                float dxScreen = 220 - 20;  // 200 pixels
                float dyScreen = 300 - 20;  // 280 pixels

                if (dxRaw == 0 || dyRaw == 0) {
                    ESP_LOGE("Calibration", "Invalid raw data range");
                    tt::app::start("Launcher");
                    return;
                }

                cal.xScale = dxScreen / dxRaw;
                cal.yScale = dyScreen / dyRaw;
                cal.xOffset = 20 - cal.xScale * app->rawX[0];
                cal.yOffset = 20 - cal.yScale * app->rawY[0];
                cal.valid = true;

                ESP_LOGI("Calibration", "Results:");
                ESP_LOGI("Calibration", "Top-Left: x=%d, y=%d", app->rawX[0], app->rawY[0]);
                ESP_LOGI("Calibration", "Bottom-Right: x=%d, y=%d", app->rawX[1], app->rawY[1]);
                ESP_LOGI("Calibration", "xScale=%.3f, xOffset=%.3f, yScale=%.3f, yOffset=%.3f",
                         cal.xScale, cal.xOffset, cal.yScale, cal.yOffset);

                touch.setCalibration(cal);
                vTaskDelay(2000 / portTICK_PERIOD_MS);  // Show result briefly
                tt::app::start("Launcher");
                break;
            }
            default:
                tt::app::start("Launcher");
                break;
        }
    }

    void updateScreen(const char* instruction) {
        lv_label_set_text(label, instruction);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    }

    void drawCrosshair(int16_t x, int16_t y) {
        crosshair = lv_obj_create(lv_scr_act());
        lv_obj_set_size(crosshair, 20, 20);
        lv_obj_set_pos(crosshair, x - 10, y - 10);
        lv_obj_t* line1 = lv_line_create(crosshair);
        lv_obj_t* line2 = lv_line_create(crosshair);
        static lv_point_t points1[] = {{0, 10}, {20, 10}};
        static lv_point_t points2[] = {{10, 0}, {10, 20}};
        lv_line_set_points(line1, points1, 2);
        lv_line_set_points(line2, points2, 2);
        lv_obj_set_style_line_color(line1, lv_color_red(), 0);
        lv_obj_set_style_line_color(line2, lv_color_red(), 0);
    }

    void logTouchData(uint16_t rawX, uint16_t rawY) {
        if (step < 2) {
            rawX[step] = rawX;
            rawY[step] = rawY;  // Removed manual Y offset
            ESP_LOGI("Calibration", "Step %d: rawX=%d, rawY=%d", step, rawX, rawY);
        }
    }

    lv_obj_t* label = nullptr;
    lv_obj_t* toolbar = nullptr;
    lv_obj_t* crosshair = nullptr;
    int step = 0;  // 0: top-left, 1: bottom-right, 2: done
    uint16_t rawX[2] = {0};
    uint16_t rawY[2] = {0};
#else
    static void eventCallback(lv_event_t* /*e*/) {}
    void updateScreen(const char* /*instruction*/) {}
    void drawCrosshair(int16_t /*x*/, int16_t /*y*/) {}
    void logTouchData(uint16_t /*rawX*/, uint16_t /*rawY*/) {}
    lv_obj_t* label = nullptr;
    lv_obj_t* toolbar = nullptr;
#endif
};

extern const AppManifest calibration_app = {
    .id = "Calibration",
    .name = "Touch Calibration",
    .createApp = create<Calibration>
};


#endif
