#include <Tactility/app/setup/Setup.h>

#include <tactility/paths.h>

#include <Tactility/StringUtils.h>
#include <Tactility/app/timezone/TimeZoneView.h>
#include <Tactility/app/wifimanage/WifiManage.h>
#include <Tactility/file/File.h>
#include <Tactility/settings/Time.h>
#include <Tactility/Tactility.h>

#include <app/event.h>
#include <app/manager.h>
#include <app/manifest.h>
#include <app/scheduler.h>
#include <app/start.h>

#include <location/location.h>

#include <wifi/wifi_settings.h>

#include <lvgl_window_manager/window_manager.h>

#include <tactility/check.h>
#include <tactility/device.h>
#include <tactility/drivers/wifi.h>
#include <tactility/log.h>

#include <lvgl/devices/indev.h>
#include <lvgl/fonts.h>
#include <lvgl/grid_navigation.h>
#include <lvgl/lvgl.h>
#include <lvgl/widgets/chip.h>
#include <lvgl/widgets/step_indicator.h>
#include <lvgl/widgets/text_button.h>
#include <lvgl.h>

#include <functional>
#include <memory>
#include <vector>

#ifdef ESP_PLATFORM
#include <sdkconfig.h>
#endif

#ifdef CONFIG_TT_TOUCH_CALIBRATION_REQUIRED
#include <Tactility/app/touchcalibration/TouchCalibration.h>
#endif

namespace tt::app::setup {

extern const ::AppManifest manifest;

constexpr auto* TAG = "setup";

namespace {

bool getCompletedMarkerPath(std::string& outPath) {
    char path[128];
    if (paths_get_data_path(path, sizeof(path)) != ERROR_NONE) {
        return false;
    }
    outPath = std::string(path) + "/.setup_complete";
    return true;
}

} // namespace

bool isCompleted() {
    std::string path;
    if (!getCompletedMarkerPath(path)) {
        LOG_E(TAG, "Setup path not found");
        return false;
    }
    return file::isFile(path);
}

namespace {

void markCompleted() {
    std::string path;
    if (!getCompletedMarkerPath(path)) {
        return;
    }
    file::writeString(path, "");
}

enum class Phase {
    Welcome,
    StepIntro,
    Done
};

struct StepConfiguration {
    std::string title;
    std::string description;
    std::function<void()> run;
    /** Optional: handles the result of the dialog that run() started */
    std::function<void(int32_t)> onResult = nullptr;
    /** What the page shows below its title */
    enum class Content {
        Description,
        /** The description, with the location choices below it */
        LocationChoice,
        /** The time zone list instead of the description */
        TimeZones
    } content = Content::Description;
};

/** The location choices of the location page, in the order of their chips */
constexpr const char* LOCATION_CHOICES[] = { "Precise", "Approximate", "Disabled" };

struct Context {
    uint32_t appInstanceId;

    Phase phase = Phase::Welcome;
    size_t stepIndex = 0;
    std::vector<StepConfiguration> steps;
    uint32_t pendingStepDialogId = 0;

    // Valid while the window is shown
    lv_obj_t* titleLabel = nullptr;
    lv_obj_t* descriptionLabel = nullptr;
    lv_obj_t* backButton = nullptr;
    lv_obj_t* skipButton = nullptr;
    lv_obj_t* continueButton = nullptr;
    lv_obj_t* stepIndicator = nullptr;
    lv_obj_t* bottom = nullptr;
    lv_obj_t* timeZoneContainer = nullptr;
    lv_obj_t* locationChips = nullptr;
    // The index in LOCATION_CHOICES picked on the location page, -1 when none. Guarded by the LVGL lock.
    int32_t locationChoice = -1;

    std::unique_ptr<timezone::TimeZoneView> timeZoneView;
    // The time zone picked on the time zone page, guarded by the LVGL lock
    std::string timeZoneName;
    std::string timeZoneCode;
};

/** The welcome page, a page per step, and the completion page */
uint32_t getPageCount(const Context* ctx) {
    return static_cast<uint32_t>(ctx->steps.size()) + 2;
}

uint32_t getPage(const Context* ctx) {
    switch (ctx->phase) {
        case Phase::Welcome:
            return 0;
        case Phase::StepIntro:
            return static_cast<uint32_t>(ctx->stepIndex) + 1;
        case Phase::Done:
            return getPageCount(ctx) - 1;
    }
    return 0;
}


/** The welcome page counts as the first step, followed by the configuration steps */
uint32_t getStepCount(const Context* ctx) {
    return static_cast<uint32_t>(ctx->steps.size()) + 1;
}

/** The steps before the current one, or all of them once setup is done */
uint32_t getFinishedStepCount(const Context* ctx) {
    switch (ctx->phase) {
        case Phase::Welcome:
            return 0;
        case Phase::StepIntro:
            return static_cast<uint32_t>(ctx->stepIndex) + 1;
        case Phase::Done:
            return getStepCount(ctx);
    }
    return 0;
}

StepConfiguration::Content getContent(const Context* ctx) {
    return ctx->phase == Phase::StepIntro ? ctx->steps[ctx->stepIndex].content : StepConfiguration::Content::Description;
}

/** Shows which location choice is picked */
void renderLocationChips(Context* ctx) {
    const uint32_t count = lv_obj_get_child_count(ctx->locationChips);
    for (uint32_t i = 0; i < count; i++) {
        lv_obj_set_state(lv_obj_get_child(ctx->locationChips, static_cast<int32_t>(i)), LV_STATE_CHECKED, static_cast<int32_t>(i) == ctx->locationChoice);
    }
}

/** The time zone page shows the title at the top and the list below it, the other pages center their text */
void renderLayout(Context* ctx) {
    const auto content = getContent(ctx);
    const bool showsTimeZones = content == StepConfiguration::Content::TimeZones;
    const bool showsLocationChoice = content == StepConfiguration::Content::LocationChoice;
    const bool hasTimeZoneWidgets = lv_obj_get_child_count(ctx->timeZoneContainer) > 0;
    lv_obj_set_hidden(ctx->descriptionLabel, showsTimeZones);
    lv_obj_set_hidden(ctx->timeZoneContainer, !showsTimeZones);
    lv_obj_set_hidden(ctx->locationChips, !showsLocationChoice);
    if (!showsTimeZones) {
        if (hasTimeZoneWidgets) {
            lv_obj_clean(ctx->timeZoneContainer);
            ctx->timeZoneView->destroyWidgets();
        }
        const int titleMargin = lvgl_get_text_font_height(FONT_SIZE_LARGE);
        if (showsLocationChoice) {
            // Centered as a group in the space above the buttons
            const int32_t gap = titleMargin / 2;
            lv_obj_update_layout(lv_obj_get_parent(ctx->titleLabel));
            const int32_t height = lv_obj_get_height(ctx->titleLabel) + gap + lv_obj_get_height(ctx->descriptionLabel) + gap + lv_obj_get_height(ctx->locationChips);
            lv_area_t parent;
            lv_area_t bottom;
            lv_obj_get_content_coords(lv_obj_get_parent(ctx->titleLabel), &parent);
            lv_obj_get_coords(ctx->bottom, &bottom);
            const int32_t top = LV_MAX((bottom.y1 - parent.y1 - height) / 2, 0);
            lv_obj_align(ctx->titleLabel, LV_ALIGN_TOP_MID, 0, top);
            lv_obj_align_to(ctx->descriptionLabel, ctx->titleLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, gap);
            lv_obj_align_to(ctx->locationChips, ctx->descriptionLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, gap);
            // Continue confirms the picked choice
            renderLocationChips(ctx);
            lv_obj_set_state(ctx->continueButton, LV_STATE_DISABLED, ctx->locationChoice < 0);
        } else {
            lv_obj_align(ctx->descriptionLabel, LV_ALIGN_CENTER, 0, 0);
            lv_obj_align_to(ctx->titleLabel, ctx->descriptionLabel, LV_ALIGN_OUT_TOP_MID, 0, -titleMargin);
            lv_obj_remove_state(ctx->continueButton, LV_STATE_DISABLED);
        }
        return;
    }

    lv_obj_align(ctx->titleLabel, LV_ALIGN_TOP_MID, 0, LV_DPX(8));
    if (!hasTimeZoneWidgets) {
        ctx->timeZoneView->createWidgets(ctx->timeZoneContainer);
    }
    // Continue confirms the picked time zone
    lv_obj_set_state(ctx->continueButton, LV_STATE_DISABLED, ctx->timeZoneName.empty());

    // Fills the space between the title and the buttons
    lv_obj_update_layout(lv_obj_get_parent(ctx->timeZoneContainer));
    lv_area_t title;
    lv_area_t bottom;
    lv_obj_get_coords(ctx->titleLabel, &title);
    lv_obj_get_coords(ctx->bottom, &bottom);
    lv_area_t parent;
    lv_obj_get_content_coords(lv_obj_get_parent(ctx->timeZoneContainer), &parent);
    lv_obj_set_pos(ctx->timeZoneContainer, 0, title.y2 + LV_DPX(4) - parent.y1);
    lv_obj_set_size(ctx->timeZoneContainer, LV_PCT(100), bottom.y1 - title.y2 - LV_DPX(8));
}

void renderCurrent(Context* ctx) {
    lvgl_step_indicator_set_finished_count(ctx->stepIndicator, getFinishedStepCount(ctx));
    lv_obj_set_hidden(ctx->backButton, ctx->phase == Phase::Welcome);
    switch (ctx->phase) {
        case Phase::Welcome: {
            lv_label_set_text(ctx->titleLabel, "Welcome");
            auto device_names = string::split(std::string(CONFIG_TT_DEVICE_NAME_SIMPLE), ",");
            lv_label_set_text_fmt(ctx->descriptionLabel, "It's time to set up your %s!", device_names.front().c_str());
            lv_obj_set_hidden(ctx->skipButton, true);
            lv_label_set_text(lv_obj_get_child(ctx->continueButton, 0), "Continue");
            break;
        }
        case Phase::StepIntro: {
            const auto& step = ctx->steps[ctx->stepIndex];
            lv_label_set_text(ctx->titleLabel, step.title.c_str());
            lv_label_set_text(ctx->descriptionLabel, step.description.c_str());
            lv_obj_set_hidden(ctx->skipButton, false);
            lv_label_set_text(lv_obj_get_child(ctx->skipButton, 0), "Skip");
            lv_label_set_text(lv_obj_get_child(ctx->continueButton, 0), "Continue");
            break;
        }
        case Phase::Done:
            lv_label_set_text(ctx->titleLabel, "Setup Complete");
            lv_label_set_text(ctx->descriptionLabel, "You're all set.");
            lv_obj_set_hidden(ctx->skipButton, true);
            lv_label_set_text(lv_obj_get_child(ctx->continueButton, 0), "Finish");
            break;
    }
    renderLayout(ctx);
}

void advanceTo(Context* ctx, size_t index) {
    if (index < ctx->steps.size()) {
        ctx->stepIndex = index;
        ctx->phase = Phase::StepIntro;
    } else {
        ctx->phase = Phase::Done;
    }

    lvgl_lock();
    // Covered by a step's app: createWidgets() renders the current page when the window is shown again
    if (ctx->titleLabel != nullptr) {
        renderCurrent(ctx);
    }
    lvgl_unlock();
}

void onBackClicked(lv_event_t* event) {
    auto* ctx = static_cast<Context*>(lv_event_get_user_data(event));
    // The previous page: a step's intro page, or the welcome page before the first step
    const size_t previousPage = getPage(ctx) - 1;
    if (ctx->phase == Phase::Welcome) {
        return;
    } else if (previousPage == 0) {
        ctx->phase = Phase::Welcome;
        lvgl_lock();
        renderCurrent(ctx);
        lvgl_unlock();
    } else {
        advanceTo(ctx, previousPage - 1);
    }
}

void onSkipClicked(lv_event_t* event) {
    auto* ctx = static_cast<Context*>(lv_event_get_user_data(event));
    if (ctx->phase == Phase::StepIntro) {
        advanceTo(ctx, ctx->stepIndex + 1);
    }
}

void onContinueClicked(lv_event_t* event) {
    auto* ctx = static_cast<Context*>(lv_event_get_user_data(event));
    switch (ctx->phase) {
        case Phase::Welcome:
            advanceTo(ctx, 0);
            break;
        case Phase::StepIntro:
            ctx->steps[ctx->stepIndex].run();
            break;
        case Phase::Done: {
            markCompleted();
            app_event_emit_close(ctx->appInstanceId);
            break;
        }
    }
}

void onLocationChipPressed(lv_event_t* event) {
    auto* ctx = static_cast<Context*>(lv_event_get_user_data(event));
    ctx->locationChoice = static_cast<int32_t>(reinterpret_cast<uintptr_t>(lv_obj_get_user_data(lv_event_get_target_obj(event))));
    renderLocationChips(ctx);
    lv_obj_remove_state(ctx->continueButton, LV_STATE_DISABLED);
}

void createWidgets(lv_obj_t* parent, void* userData) {
    auto* ctx = static_cast<Context*>(userData);

    ctx->titleLabel = lv_label_create(parent);
    lv_obj_set_width(ctx->titleLabel, LV_PCT(80));
    lv_obj_set_style_text_align(ctx->titleLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(ctx->titleLabel, LV_LABEL_LONG_WRAP);
    auto* font = lvgl_get_text_font(FONT_SIZE_LARGE);
    lv_obj_set_style_text_font(ctx->titleLabel, font, 0);

    ctx->descriptionLabel = lv_label_create(parent);
    lv_obj_set_width(ctx->descriptionLabel, LV_PCT(80));
    lv_obj_set_style_text_align(ctx->descriptionLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(ctx->descriptionLabel, LV_LABEL_LONG_WRAP);
    lv_obj_align(ctx->descriptionLabel, LV_ALIGN_CENTER, 0, 0);

    // The location choices: one can be picked, and they wrap to more lines on narrow screens
    ctx->locationChips = lv_obj_create(parent);
    lv_obj_remove_style_all(ctx->locationChips);
    lv_obj_set_size(ctx->locationChips, LV_PCT(100), LV_SIZE_CONTENT);
    // The chip margins space the choices, so they fit on one line on small screens
    lv_obj_set_flex_flow(ctx->locationChips, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(ctx->locationChips, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    for (size_t i = 0; i < std::size(LOCATION_CHOICES); i++) {
        auto* chip = lvgl_chip_create(ctx->locationChips);
        lv_label_set_text(lv_label_create(chip), LOCATION_CHOICES[i]);
        lv_obj_set_user_data(chip, reinterpret_cast<void*>(static_cast<uintptr_t>(i)));
        lv_obj_add_event_cb(chip, onLocationChipPressed, LV_EVENT_SHORT_CLICKED, ctx);
    }
    lvgl_grid_navigation_add(ctx->locationChips);
    lv_obj_set_hidden(ctx->locationChips, true);

    // Filled with the time zone list on the time zone page
    ctx->timeZoneContainer = lv_obj_create(parent);
    lv_obj_remove_style_all(ctx->timeZoneContainer);
    lv_obj_set_flex_flow(ctx->timeZoneContainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_hor(ctx->timeZoneContainer, LV_DPX(8), LV_STATE_DEFAULT);
    lv_obj_set_hidden(ctx->timeZoneContainer, true);

    // The buttons, with the progress through the pages below them at the bottom
    auto* bottom = lv_obj_create(parent);
    ctx->bottom = bottom;
    lv_obj_remove_style_all(bottom);
    lv_obj_set_size(bottom, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(bottom, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(bottom, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(bottom, LV_ALIGN_BOTTOM_MID, 0, 0);

    // Back on the left, and Skip next to Continue on the right
    auto* buttonRow = lv_obj_create(bottom);
    lv_obj_remove_style_all(buttonRow);
    lv_obj_set_size(buttonRow, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_hor(buttonRow, LV_DPX(8), LV_STATE_DEFAULT);
    lv_obj_set_flex_flow(buttonRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(buttonRow, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    ctx->backButton = lvgl_text_button_create(buttonRow);
    lv_label_set_text(lv_label_create(ctx->backButton), "Back");
    lv_obj_add_event_cb(ctx->backButton, onBackClicked, LV_EVENT_SHORT_CLICKED, ctx);

    auto* spacer = lv_obj_create(buttonRow);
    lv_obj_remove_style_all(spacer);
    lv_obj_set_height(spacer, 1);
    lv_obj_set_flex_grow(spacer, 1);

    ctx->skipButton = lvgl_text_button_create(buttonRow);
    lv_label_set_text(lv_label_create(ctx->skipButton), "Skip");
    lv_obj_add_event_cb(ctx->skipButton, onSkipClicked, LV_EVENT_SHORT_CLICKED, ctx);

    ctx->continueButton = lv_button_create(buttonRow);
    lv_label_set_text(lv_label_create(ctx->continueButton), "Continue");
    lv_obj_add_event_cb(ctx->continueButton, onContinueClicked, LV_EVENT_SHORT_CLICKED, ctx);

    ctx->stepIndicator = lvgl_step_indicator_create(bottom);
    lvgl_step_indicator_set_step_count(ctx->stepIndicator, getStepCount(ctx));

    // The arrow keys move between the buttons, and Continue has the focus first
    lvgl_grid_navigation_add(buttonRow);
    if (!lvgl_indev_exists(LV_INDEV_TYPE_POINTER)) {
        lv_group_focus_obj(buttonRow);
        lv_gridnav_set_focused(buttonRow, ctx->continueButton, LV_ANIM_OFF);
    }

    renderCurrent(ctx);
}

void destroyWidgets(void* userData) {
    auto* ctx = static_cast<Context*>(userData);
    ctx->titleLabel = nullptr;
    ctx->descriptionLabel = nullptr;
    ctx->backButton = nullptr;
    ctx->skipButton = nullptr;
    ctx->continueButton = nullptr;
    ctx->stepIndicator = nullptr;
    ctx->bottom = nullptr;
    ctx->timeZoneContainer = nullptr;
    ctx->locationChips = nullptr;
    ctx->timeZoneView->destroyWidgets();
}

int32_t appMain(int argc, char* argv[]) {
    uint32_t appInstanceId = app_scheduler_current_app_id();
    Context ctx {};
    ctx.appInstanceId = appInstanceId;
    ctx.steps = {
#if defined(CONFIG_TT_TOUCH_CALIBRATION_REQUIRED)
        {
            .title = "Touch Calibration",
            .description = "Let's calibrate the touch screen.",
            .run = [&ctx] { ctx.pendingStepDialogId = touchcalibration::start(ctx.appInstanceId); }
        },
#endif
        {
            .title = "Wi-Fi Setup",
            .description = "Let's connect to a Wi-Fi access point.",
            .run = [&ctx] {
                getMainDispatcher().dispatch([] {
                    Device* wifi_device = nullptr;
                    if (device_get_first_by_type(&WIFI_TYPE, &wifi_device) == ERROR_NONE) {
                        if (wifi_set_radio_on(wifi_device) == ERROR_NONE && wifi_settings_set_enable(true) != ERROR_NONE) {
                            LOG_E(TAG, "Failed to save the Wi-Fi state");
                        }
                        device_put(wifi_device);
                    }
                });
                ctx.pendingStepDialogId = wifimanage::start(ctx.appInstanceId);
            }
        }
    };

    ctx.steps.push_back({
        .title = "Location Services",
        .description = "Do you want to allow apps to retrieve your location? It uses GPS and/or contacts geoiplookup.io",
        .run = [&ctx] {
            // The order of LOCATION_CHOICES: precise, approximate, disabled
            if (ctx.locationChoice == 0 || ctx.locationChoice == 1) {
                location_set_precision(ctx.locationChoice == 0 ? LOCATION_PRECISION_PRECISE : LOCATION_PRECISION_APPROXIMATE);
                location_set_enabled(true);
            } else {
                location_set_enabled(false);
            }
            advanceTo(&ctx, ctx.stepIndex + 1);
        },
        .content = StepConfiguration::Content::LocationChoice
    });

    // Setting the time zone also sets the radio region, as both depend on where the device is
    ctx.steps.push_back({
        .title = "Time Zone",
        .description = "",
        .run = [&ctx] {
            settings::setTimeZone(ctx.timeZoneName, ctx.timeZoneCode);
            advanceTo(&ctx, ctx.stepIndex + 1);
        },
        .content = StepConfiguration::Content::TimeZones
    });
    ctx.timeZoneView = std::make_unique<timezone::TimeZoneView>([&ctx](const std::string& name, const std::string& code) {
        ctx.timeZoneName = name;
        ctx.timeZoneCode = code;
        if (ctx.continueButton != nullptr) {
            lv_obj_remove_state(ctx.continueButton, LV_STATE_DISABLED);
        }
    });

    TaskEventGroup event_group {};
    task_event_group_construct(&event_group);

    AppEventSubscription sub {};
    check(app_event_subscribe(&sub, &event_group) == ERROR_NONE);

    WindowId window = window_manager_create_ext(appInstanceId, createWidgets, destroyWidgets, &ctx);

    bool shouldClose = false;
    while (!shouldClose) {
        task_event_group_wait_any(&event_group, nullptr, portMAX_DELAY);

        AppEvent event {};
        while (app_event_poll(&sub, &event) == ERROR_NONE) {
            switch (event.type) {
                case APP_EVENT_CLOSE:
                    shouldClose = true;
                    break;
                case APP_EVENT_RESULT:
                    if (event.result.launch_id == ctx.pendingStepDialogId) {
                        ctx.pendingStepDialogId = 0;
                        const auto& onResult = ctx.steps[ctx.stepIndex].onResult;
                        if (onResult != nullptr) {
                            onResult(event.result.result);
                        }
                        advanceTo(&ctx, ctx.stepIndex + 1);
                    }
                    app_manager_stop(event.result.launch_id);
                    break;
                default:
                    break;
            }
            if (shouldClose) break;
        }
    }

    window_manager_remove(window);
    check(app_event_unsubscribe(&sub) == ERROR_NONE);
    task_event_group_destruct(&event_group);

    return 0;
}

} // namespace

void start() {
    uint32_t instanceId = 0;
    AppStartContext context = app_start_context_for_manifest(&manifest);
    app_start_with_context(&context, &instanceId);
}

extern const ::AppManifest manifest = {
    .id = "tactility.setup",
    .name = "Setup",
    .category = APP_CATEGORY_SYSTEM,
    .location = { APP_LOCATION_MEMORY, reinterpret_cast<void*>(appMain) },
    .flags = APP_MANIFEST_FLAG_HIDDEN,
    .stack = {}
};

}
