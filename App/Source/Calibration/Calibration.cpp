#include "esp_log.h"
#include <Tactility/app/App.h>
#include <Tactility/app/AppManifest.h>
#include <Tactility/lvgl/Toolbar.h>
#include <lvgl.h>

// Board-specific check based on Boards.h
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

        lv_obj_t* toolbar = tt::lvgl::toolbar_create(parent, context);
        lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);

        label = lv_label_create(parent);
        updateScreen("Tap the top-left corner");
        lv_obj_add_event_cb(lv_scr_act(), eventCallback, LV_EVENT_PRESSED, this);
#else
        ESP_LOGI("Calibration", "Calibration not supported on this board");

        lv_obj_t* toolbar = tt::lvgl::toolbar_create(parent, context);
        lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);

        label = lv_label_create(parent);
        lv_label_set_text(label, "Calibration only supported\non CYD-2432S028R");
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
#endif
    }

    void onHide(AppContext& /*context*/) override {
        ESP_LOGI("Calibration", "Hiding calibration");
        if (label) {
            lv_obj_del(label);
            label = nullptr;
        }
    }

private:
#ifdef CONFIG_TT_BOARD_CYD_2432S028R
    static void eventCallback(lv_event_t* e) {
        Calibration* app = static_cast<Calibration*>(lv_event_get_user_data(e));
        uint16_t rawX, rawY;
        extern XPT2046_TouchscreenSOFTSPI<CYD_TOUCH_MISO_PIN, CYD_TOUCH_MOSI_PIN, CYD_TOUCH_SCK_PIN, 0> touch;
        touch.getRawTouch(rawX, rawY);

        app->logTouchData(rawX, rawY);

        app->step++;
        switch (app->step) {
            case 1:
                app->updateScreen("Tap the top-right corner");
                break;
            case 2:
                app->updateScreen("Tap the bottom-left corner");
                break;
            case 3:
                app->updateScreen("Tap the bottom-right corner");
                break;
            case 4: {
                app->updateScreen("Calibration complete!");
                ESP_LOGI("Calibration", "Calibration Results:");
                ESP_LOGI("Calibration", "Top-Left: x=%d, y=%d", app->rawX[0], app->rawY[0]);
                ESP_LOGI("Calibration", "Top-Right: x=%d, y=%d", app->rawX[1], app->rawY[1]);
                ESP_LOGI("Calibration", "Bottom-Left: x=%d, y=%d", app->rawX[2], app->rawY[2]);
                ESP_LOGI("Calibration", "Bottom-Right: x=%d, y=%d", app->rawX[3], app->rawY[3]);

                int minX = std::min({app->rawX[0], app->rawX[1], app->rawX[2], app->rawX[3]});
                int maxX = std::max({app->rawX[0], app->rawX[1], app->rawX[2], app->rawX[3]});
                int minY = std::min({app->rawY[0], app->rawY[1], app->rawY[2], app->rawY[3]});
                int maxY = std::max({app->rawY[0], app->rawY[1], app->rawY[2], app->rawY[3]});

                ESP_LOGI("Calibration", "X Range: %d to %d", minX, maxX);
                ESP_LOGI("Calibration", "Y Range: %d to %d", minY, maxY);
                ESP_LOGI("Calibration", "Suggested X: (ux - %d) * 239 / (%d - %d)", minX, maxX, minX);
                ESP_LOGI("Calibration", "Suggested Y: (uy - %d) * 319 / (%d - %d)", minY, maxY, minY);
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

    void logTouchData(uint16_t rawX, uint16_t rawY) {
        if (step < 4) {
            this->rawX[step] = rawX;
            this->rawY[step] = rawY;
            ESP_LOGI("Calibration", "Step %d: rawX=%d, rawY=%d", step, rawX, rawY);
        }
    }

    lv_obj_t* label = nullptr;
    int step = 0;  // 0: top-left, 1: top-right, 2: bottom-left, 3: bottom-right, 4: done
    uint16_t rawX[4] = {0};
    uint16_t rawY[4] = {0};
#else
    // No-op versions for non-CYD-2432S028R boards
    static void eventCallback(lv_event_t* /*e*/) {}
    void updateScreen(const char* /*instruction*/) {}
    void logTouchData(uint16_t /*rawX*/, uint16_t /*rawY*/) {}

    lv_obj_t* label = nullptr;
#endif
};

// Define the manifest (works for all boards)
extern const AppManifest calibration_app = {
    .id = "Calibration",
    .name = "Touch Calibration",
    .createApp = create<Calibration>
};
