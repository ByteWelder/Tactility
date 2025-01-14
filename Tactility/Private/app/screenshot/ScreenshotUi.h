#include "Timer.h"
#include "TactilityConfig.h"

#if TT_FEATURE_SCREENSHOT_ENABLED

#pragma once

#include "app/AppContext.h"
#include "lvgl.h"

namespace tt::app::screenshot {

class ScreenshotUi {

    lv_obj_t* modeDropdown = nullptr;
    lv_obj_t* pathTextArea = nullptr;
    lv_obj_t* startStopButtonLabel = nullptr;
    lv_obj_t* timerWrapper = nullptr;
    lv_obj_t* delayTextArea = nullptr;
    std::unique_ptr<Timer> updateTimer;

    void createTimerSettingsWidgets(lv_obj_t* parent);
    void createModeSettingWidgets(lv_obj_t* parent);
    void createFilePathWidgets(lv_obj_t* parent);

    void updateScreenshotMode();

public:

    ScreenshotUi();
    ~ScreenshotUi();

    void createWidgets(const AppContext& app, lv_obj_t* parent);
    void onStartPressed();
    void onModeSet();
    void onTimerTick();
};


} // namespace

#endif
