#include <Tactility/app/App.h>
#include <Tactility/app/AppManifest.h>
#include <Tactility/lvgl/Toolbar.h>
#include <lvgl.h>
#include "../../Drivers/XPT2046-SoftSPI/esp_lcd_touch_xpt2046/include/esp_lcd_touch_xpt2046.h"

#ifdef ESP_PLATFORM
#include "esp_log.h"
#include <nvs_flash.h>
#endif

#ifdef CONFIG_TT_BOARD_CYD_2432S028R
#include "../../../Boards/CYD-2432S028R/Source/hal/YellowDisplayConstants.h"
#include "../../../Drivers/XPT2046-SoftSPI/XPT2046-SoftSPI.h"
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
#ifdef CONFIG_TT_BOARD_CYD_2432S028R
        if (crosshair) {
            lv_obj_del(crosshair);
            crosshair = nullptr;
        }
#endif
        toolbar = nullptr;
    }

private:
#ifdef CONFIG_TT_BOARD_CYD_2432S028R
    struct CalibrationData {
        float xScale;
        float yScale;
        float xOffset;
        float yOffset;
        bool valid;
    };

    static void eventCallback(lv_event_t* e) {
        Calibration* app = static_cast<Calibration*>(lv_event_get_user_data(e));
        uint16_t rawX, rawY;
        extern std::unique_ptr<XPT2046_SoftSPI> touch;
        if (touch) {
            touch->get_raw_touch(rawX, rawY);
        } else {
            rawX = rawY = 0;
        }

        if (rawX == 0 || rawY == 0) return;

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

                CalibrationData cal;
                float dxRaw = app->rawX[1] - app->rawX[0];
                float dyRaw = app->rawY[1] - app->rawY[0];
                float dxScreen = 220 - 20;
                float dyScreen = 300 - 20;

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
                ESP_LOGI("Calibration", "Top-Left: x=%u, y=%u", app->rawX[0], app->rawY[0]);
                ESP_LOGI("Calibration", "Bottom-Right: x=%u, y=%u", app->rawX[1], app->rawY[1]);
                ESP_LOGI("Calibration", "xScale=%.3f, xOffset=%.3f, yScale=%.3f, yOffset=%.3f",
                         cal.xScale, cal.xOffset, cal.yScale, cal.yOffset);

                nvs_handle_t nvs;
                if (nvs_open("touch_cal", NVS_READWRITE, &nvs) == ESP_OK) {
                    uint16_t cal_data[4] = {app->rawX[0], app->rawX[1], app->rawY[0], app->rawY[1]};
                    if (nvs_set_blob(nvs, "cal_data", cal_data, sizeof(cal_data)) == ESP_OK) {
                        nvs_commit(nvs);
                        ESP_LOGI("Calibration", "Saved to NVS: xMinRaw=%u, xMaxRaw=%u, yMinRaw=%u, yMaxRaw=%u",
                                 cal_data[0], cal_data[1], cal_data[2], cal_data[3]);
                    } else {
                        ESP_LOGE("Calibration", "Failed to save to NVS");
                    }
                    nvs_close(nvs);
                }

                vTaskDelay(2000 / portTICK_PERIOD_MS);
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
        static lv_point_precise_t points1[] = {{0, 10}, {20, 10}};
        static lv_point_precise_t points2[] = {{10, 0}, {10, 20}};
        lv_line_set_points(line1, points1, 2);
        lv_line_set_points(line2, points2, 2);
        lv_obj_set_style_line_color(line1, lv_color_make(255, 0, 0), 0);
        lv_obj_set_style_line_color(line2, lv_color_make(255, 0, 0), 0);
    }

    void logTouchData(uint16_t touchX, uint16_t touchY) {
        if (step < 2) {
            rawX[step] = touchX;
            rawY[step] = touchY;
            ESP_LOGI("Calibration", "Step %d: rawX=%u, rawY=%u", step, touchX, touchY);
        }
    }
#else
    static void eventCallback(lv_event_t* /*e*/) {}
    void updateScreen(const char* /*instruction*/) {}
    void drawCrosshair(int16_t /*x*/, int16_t /*y*/) {}
    void logTouchData(uint16_t /*rawX*/, uint16_t /*rawY*/) {}
#endif

    lv_obj_t* label = nullptr;
    lv_obj_t* toolbar = nullptr;
#ifdef CONFIG_TT_BOARD_CYD_2432S028R
    lv_obj_t* crosshair = nullptr;
    int step = 0;
    uint16_t rawX[2] = {0};
    uint16_t rawY[2] = {0};
#endif
};

extern const AppManifest calibration_app = {
    .id = "Calibration",
    .name = "Touch Calibration",
    .createApp = create<Calibration>
};
