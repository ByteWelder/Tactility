#include "Calibration.h"
#include "esp_log.h"
#include <Tactility/lvgl/Toolbar.h>
#include "../../../Boards/CYD-2432S028R/Source/hal/YellowDisplayConstants.h"
#include "../../../Drivers/XPT2046-SoftSPI/XPT2046_TouchscreenSOFTSPI.h"  // Access the touch driver

namespace tt::app {
    void Calibration::onShow(AppContext& context, lv_obj_t* parent) {
        ESP_LOGI("Calibration", "Starting calibration");

        lv_obj_t* toolbar = tt::lvgl::toolbar_create(parent, context);
        lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);

        label = lv_label_create(parent);
        updateScreen("Tap the top-left corner");
        lv_obj_add_event_cb(lv_scr_act(), eventCallback, LV_EVENT_PRESSED, this);
    }

    void Calibration::onHide(AppContext& /*context*/) {  // Updated signature, context unused
        ESP_LOGI("Calibration", "Hiding calibration");
        if (label) {
            lv_obj_del(label);
            label = nullptr;
        }
    }

    void Calibration::eventCallback(lv_event_t* e) {
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

    void Calibration::updateScreen(const char* instruction) {
        lv_label_set_text(label, instruction);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    }

    void Calibration::logTouchData(uint16_t rawX, uint16_t rawY) {
        if (step < 4) {
            this->rawX[step] = rawX;  // Fixed: use member variable
            this->rawY[step] = rawY;  // Fixed: use member variable
            ESP_LOGI("Calibration", "Step %d: rawX=%d, rawY=%d", step, rawX, rawY);
        }
    }

    const AppManifest calibration_app = {
        .id = "Calibration",
        .name = "Touch Calibration",
        .createApp = create<Calibration>
    };
}
