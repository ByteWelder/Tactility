#include "TactilityConfig.h"

#if TT_FEATURE_SCREENSHOT_ENABLED

#pragma once

#include "app/AppContext.h"
#include "lvgl.h"

namespace tt::app::screenshot {

class ScreenshotUi {

    lv_obj_t* modeDropdown;
    lv_obj_t* pathTextArea;
    lv_obj_t* startStopButtonLabel;
    lv_obj_t* timerWrapper;
    lv_obj_t* delayTextArea;

    void createTimerSettingsWidgets(lv_obj_t* parent);
    void createModeSettingWidgets(lv_obj_t* parent);
    void createFilePathWidgets(lv_obj_t* parent);

    void updateScreenshotMode();
public:

    void createWidgets(const AppContext& app, lv_obj_t* parent);
    void onStartPressed();
    void onModeSet();
};


} // namespace

#endif
