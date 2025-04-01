#pragma once

#include <tt/app/App.h>
#include <lvgl.h>

namespace tt::app {
    class Calibration final : public App {
    public:
        void onShow(AppContext& context, lv_obj_t* parent) override;
        void onHide() override;

    private:
        static void eventCallback(lv_event_t* e);
        void updateScreen(const char* instruction);
        void logTouchData(uint16_t rawX, uint16_t rawY);

        lv_obj_t* label;
        int step = 0;  // 0: top-left, 1: top-right, 2: bottom-left, 3: bottom-right, 4: done
        uint16_t rawX[4] = {0};
        uint16_t rawY[4] = {0};
    };

    extern const AppManifest calibration_app;
}
