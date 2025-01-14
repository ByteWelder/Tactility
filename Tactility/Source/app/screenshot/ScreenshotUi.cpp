#include "TactilityConfig.h"

#if TT_FEATURE_SCREENSHOT_ENABLED

#include "app/screenshot/ScreenshotUi.h"

#include "TactilityCore.h"
#include "hal/SdCard.h"
#include "service/gui/Gui.h"
#include "service/loader/Loader.h"
#include "service/screenshot/Screenshot.h"
#include "lvgl/Toolbar.h"
#include "TactilityHeadless.h"
#include "lvgl/LvglSync.h"

namespace tt::app::screenshot {

#define TAG "screenshot_ui"

extern AppManifest manifest;

/** Returns the app data if the app is active. Note that this could clash if the same app is started twice and a background thread is slow. */
std::shared_ptr<ScreenshotUi> _Nullable optScreenshotUi() {
    app::AppContext* app = service::loader::getCurrentApp();
    if (app->getManifest().id == manifest.id) {
        return std::static_pointer_cast<ScreenshotUi>(app->getData());
    } else {
        return nullptr;
    }
}

static void onStartPressedCallback(TT_UNUSED lv_event_t* event) {
    auto ui = optScreenshotUi();
    if (ui != nullptr) {
        ui->onStartPressed();
    }
}

static void onModeSetCallback(TT_UNUSED lv_event_t* event) {
    auto ui = optScreenshotUi();
    if (ui != nullptr) {
        ui->onModeSet();
    }
}

static void onTimerCallback(TT_UNUSED std::shared_ptr<void> context) {
    auto screenshot_ui = optScreenshotUi();
    if (screenshot_ui != nullptr) {
        screenshot_ui->onTimerTick();
    }
}

ScreenshotUi::ScreenshotUi() {
    updateTimer = std::make_unique<Timer>(Timer::TypePeriodic, onTimerCallback, nullptr);
}

ScreenshotUi::~ScreenshotUi() {
    if (updateTimer->isRunning()) {
        updateTimer->stop();
    }
}

void ScreenshotUi::onTimerTick() {
    auto lvgl_lock = lvgl::getLvglSyncLockable()->scoped();
    if (lvgl_lock->lock(50 / portTICK_PERIOD_MS)) {
        updateScreenshotMode();
    }
}

void ScreenshotUi::onModeSet() {
    updateScreenshotMode();
}

void ScreenshotUi::onStartPressed() {
    auto service = service::screenshot::optScreenshotService();
    if (service == nullptr) {
        TT_LOG_E(TAG, "Service not found/running");
        return;
    }

    if (service->isTaskStarted()) {
        TT_LOG_I(TAG, "Stop screenshot");
        service->stop();
    } else {
        uint32_t selected = lv_dropdown_get_selected(modeDropdown);
        const char* path = lv_textarea_get_text(pathTextArea);
        if (selected == 0) {
            TT_LOG_I(TAG, "Start timed screenshots");
            const char* delay_text = lv_textarea_get_text(delayTextArea);
            int delay = atoi(delay_text);
            if (delay > 0) {
                service->startTimed(path, delay, 1);
            } else {
                TT_LOG_W(TAG, "Ignored screenshot start because delay was 0");
            }
        } else {
            TT_LOG_I(TAG, "Start app screenshots");
            service->startApps(path);
        }
    }

    updateScreenshotMode();
}

void ScreenshotUi::updateScreenshotMode() {
    auto service = service::screenshot::optScreenshotService();
    if (service == nullptr) {
        TT_LOG_E(TAG, "Service not found/running");
        return;
    }

    lv_obj_t* label = startStopButtonLabel;
    if (service->isTaskStarted()) {
        lv_label_set_text(label, "Stop");
    } else {
        lv_label_set_text(label, "Start");
    }

    uint32_t selected = lv_dropdown_get_selected(modeDropdown);
    if (selected == 0) { // Timer
        lv_obj_remove_flag(timerWrapper, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(timerWrapper, LV_OBJ_FLAG_HIDDEN);
    }
}


void ScreenshotUi::createModeSettingWidgets(lv_obj_t* parent) {
    auto service = service::screenshot::optScreenshotService();
    if (service == nullptr) {
        TT_LOG_E(TAG, "Service not found/running");
        return;
    }

    auto* mode_wrapper = lv_obj_create(parent);
    lv_obj_set_size(mode_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(mode_wrapper, 0, 0);
    lv_obj_set_style_border_width(mode_wrapper, 0, 0);

    auto* mode_label = lv_label_create(mode_wrapper);
    lv_label_set_text(mode_label, "Mode:");
    lv_obj_align(mode_label, LV_ALIGN_LEFT_MID, 0, 0);

    modeDropdown = lv_dropdown_create(mode_wrapper);
    lv_dropdown_set_options(modeDropdown, "Timer\nApp start");
    lv_obj_align_to(modeDropdown, mode_label, LV_ALIGN_OUT_RIGHT_MID, 8, 0);
    lv_obj_add_event_cb(modeDropdown, onModeSetCallback, LV_EVENT_VALUE_CHANGED, nullptr);
    service::screenshot::Mode mode = service->getMode();
    if (mode == service::screenshot::ScreenshotModeApps) {
        lv_dropdown_set_selected(modeDropdown, 1);
    }

    auto* button = lv_button_create(mode_wrapper);
    lv_obj_align(button, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(button, &onStartPressedCallback, LV_EVENT_SHORT_CLICKED, nullptr);
    startStopButtonLabel = lv_label_create(button);
    lv_obj_align(startStopButtonLabel, LV_ALIGN_CENTER, 0, 0);
}

void ScreenshotUi::createFilePathWidgets(lv_obj_t* parent) {
    auto* path_wrapper = lv_obj_create(parent);
    lv_obj_set_size(path_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(path_wrapper, 0, 0);
    lv_obj_set_style_border_width(path_wrapper, 0, 0);
    lv_obj_set_flex_flow(path_wrapper, LV_FLEX_FLOW_ROW);

    auto* label_wrapper = lv_obj_create(path_wrapper);
    lv_obj_set_style_border_width(label_wrapper, 0, 0);
    lv_obj_set_style_pad_all(label_wrapper, 0, 0);
    lv_obj_set_size(label_wrapper, 44, 36);
    auto* path_label = lv_label_create(label_wrapper);
    lv_label_set_text(path_label, "Path:");
    lv_obj_align(path_label, LV_ALIGN_LEFT_MID, 0, 0);

    pathTextArea = lv_textarea_create(path_wrapper);
    lv_textarea_set_one_line(pathTextArea, true);
    lv_obj_set_flex_grow(pathTextArea, 1);
    if (kernel::getPlatform() == kernel::PlatformEsp) {
        auto sdcard = tt::hal::getConfiguration()->sdcard;
        if (sdcard != nullptr && sdcard->getState() == hal::SdCard::StateMounted) {
            lv_textarea_set_text(pathTextArea, "A:/sdcard");
        } else {
            lv_textarea_set_text(pathTextArea, "Error: no SD card");
        }
    } else { // PC
        lv_textarea_set_text(pathTextArea, "A:");
    }
}

void ScreenshotUi::createTimerSettingsWidgets(lv_obj_t* parent) {
    timerWrapper = lv_obj_create(parent);
    lv_obj_set_size(timerWrapper, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(timerWrapper, 0, 0);
    lv_obj_set_style_border_width(timerWrapper, 0, 0);

    auto* delay_wrapper = lv_obj_create(timerWrapper);
    lv_obj_set_size(delay_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(delay_wrapper, 0, 0);
    lv_obj_set_style_border_width(delay_wrapper, 0, 0);
    lv_obj_set_flex_flow(delay_wrapper, LV_FLEX_FLOW_ROW);

    auto* delay_label_wrapper = lv_obj_create(delay_wrapper);
    lv_obj_set_style_border_width(delay_label_wrapper, 0, 0);
    lv_obj_set_style_pad_all(delay_label_wrapper, 0, 0);
    lv_obj_set_size(delay_label_wrapper, 44, 36);
    auto* delay_label = lv_label_create(delay_label_wrapper);
    lv_label_set_text(delay_label, "Delay:");
    lv_obj_align(delay_label, LV_ALIGN_LEFT_MID, 0, 0);

    delayTextArea = lv_textarea_create(delay_wrapper);
    lv_textarea_set_one_line(delayTextArea, true);
    lv_textarea_set_accepted_chars(delayTextArea, "0123456789");
    lv_textarea_set_text(delayTextArea, "10");
    lv_obj_set_flex_grow(delayTextArea, 1);

    auto* delay_unit_label_wrapper = lv_obj_create(delay_wrapper);
    lv_obj_set_style_border_width(delay_unit_label_wrapper, 0, 0);
    lv_obj_set_style_pad_all(delay_unit_label_wrapper, 0, 0);
    lv_obj_set_size(delay_unit_label_wrapper, LV_SIZE_CONTENT, 36);
    auto* delay_unit_label = lv_label_create(delay_unit_label_wrapper);
    lv_obj_align(delay_unit_label, LV_ALIGN_LEFT_MID, 0, 0);
    lv_label_set_text(delay_unit_label, "seconds");
}

void ScreenshotUi::createWidgets(const AppContext& app, lv_obj_t* parent) {
    if (updateTimer->isRunning()) {
        updateTimer->stop();
    }

    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    auto* toolbar = lvgl::toolbar_create(parent, app);
    lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);

    auto* wrapper = lv_obj_create(parent);
    lv_obj_set_width(wrapper, LV_PCT(100));
    lv_obj_set_flex_grow(wrapper, 1);
    lv_obj_set_style_border_width(wrapper, 0, 0);
    lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);

    createModeSettingWidgets(wrapper);
    createFilePathWidgets(wrapper);
    createTimerSettingsWidgets(wrapper);

    service::gui::keyboardAddTextArea(delayTextArea);
    service::gui::keyboardAddTextArea(pathTextArea);

    updateScreenshotMode();

    if (!updateTimer->isRunning()) {
        updateTimer->start(500 / portTICK_PERIOD_MS);
    }
}

} // namespace

#endif