#include <Tactility/lvgl/Style.h>

#include <app/event.h>
#include <app/manager.h>
#include <app/manifest.h>
#include <app/scheduler.h>

#include <location/location.h>

#include <lvgl_window_manager/window_manager.h>

#include <tactility/check.h>
#include <tactility/concurrent/task_event_group.h>

#include <lvgl.h>
#include <lvgl/fonts.h>
#include <lvgl/grid_navigation.h>
#include <lvgl/lvgl.h>
#include <lvgl/widgets/card.h>
#include <lvgl/widgets/chip.h>
#include <lvgl/widgets/toolbar.h>

#include <TactilityCpp/std/string.h>

#include <atomic>
#include <cmath>
#include <format>
#include <iterator>

namespace tt::app::locationsettings {

extern const ::AppManifest manifest;

namespace {

struct PrecisionOption {
    LocationPrecision precision;
    const char* name;
};

constexpr PrecisionOption PRECISION_OPTIONS[] = {
    { LOCATION_PRECISION_APPROXIMATE, "Approximate" },
    { LOCATION_PRECISION_PRECISE, "Precise" },
};

struct Context {
    uint32_t appInstanceId;
    TaskEventGroup* eventGroup = nullptr;
    uint32_t enableBit = 0;
    uint32_t precisionBit = 0;
    ::std::atomic<bool> requestedEnabled = false;
    ::std::atomic<LocationPrecision> requestedPrecision = LOCATION_PRECISION_PRECISE;
    // Valid while the window is shown
    lv_obj_t* enableSwitch = nullptr;
    lv_obj_t* precisionChips = nullptr;
    lv_obj_t* statusLabel = nullptr;
};

LocationPrecision getChipPrecision(lv_obj_t* chip) {
    return static_cast<LocationPrecision>(reinterpret_cast<intptr_t>(lv_obj_get_user_data(chip)));
}

std::string getStatusText() {
    if (!location_is_enabled()) {
        return "Location services are off";
    }
    Location location;
    if (location_get(&location) != ERROR_NONE) {
        return "Waiting for location...";
    }
    std::string text;
    auto output = ::std::back_inserter(text);
    ::std::format_to(output, "{:.5f}, {:.5f}", location.latitude, location.longitude);
    if (!::std::isnan(location.horizontal_accuracy)) {
        ::std::format_to(output, " (±{:.0f} m)", location.horizontal_accuracy);
    }
    ::std::format_to(output, "\nSource: {}", location.provider);
    return text;
}

void updateWidgets(Context* ctx) {
    if (ctx->statusLabel == nullptr) {
        return;
    }
    lv_obj_set_state(ctx->enableSwitch, LV_STATE_CHECKED, location_is_enabled());

    const LocationPrecision precision = location_get_precision();
    const uint32_t count = lv_obj_get_child_count(ctx->precisionChips);
    for (uint32_t i = 0; i < count; i++) {
        auto* chip = lv_obj_get_child(ctx->precisionChips, static_cast<int32_t>(i));
        lv_obj_set_state(chip, LV_STATE_CHECKED, getChipPrecision(chip) == precision);
    }

    lv_label_set_text(ctx->statusLabel, getStatusText().c_str());
}

void onBackPressed(lv_event_t* event) {
    auto* ctx = static_cast<Context*>(lv_event_get_user_data(event));
    app_event_emit_close(ctx->appInstanceId);
}

// Starting and stopping the service blocks, so it happens on the app's own task
void onEnableSwitchChanged(lv_event_t* event) {
    auto* ctx = static_cast<Context*>(lv_event_get_user_data(event));
    ctx->requestedEnabled = lv_obj_has_state(lv_event_get_target_obj(event), LV_STATE_CHECKED);
    task_event_group_signal(ctx->eventGroup, ctx->enableBit);
}

void onPrecisionChipPressed(lv_event_t* event) {
    auto* ctx = static_cast<Context*>(lv_event_get_user_data(event));
    ctx->requestedPrecision = getChipPrecision(lv_event_get_target_obj(event));
    task_event_group_signal(ctx->eventGroup, ctx->precisionBit);
}

void createWidgets(lv_obj_t* parent, void* userData) {
    auto* ctx = static_cast<Context*>(userData);

    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(parent, 0, LV_STATE_DEFAULT);

    auto* toolbar = lvgl_toolbar_create(parent, "Location");
    lvgl_toolbar_set_nav_action(toolbar, LV_SYMBOL_CLOSE, onBackPressed, ctx);
    ctx->enableSwitch = lvgl_toolbar_add_switch_action(toolbar);
    lv_obj_add_event_cb(ctx->enableSwitch, onEnableSwitchChanged, LV_EVENT_VALUE_CHANGED, ctx);

    auto* wrapper = lv_obj_create(parent);
    lv_obj_set_width(wrapper, LV_PCT(100));
    lv_obj_set_flex_grow(wrapper, 1);
    lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_border_width(wrapper, 0, LV_STATE_DEFAULT);
    lvgl::obj_set_style_bg_invisible(wrapper);

    auto* precisionLabel = lv_label_create(wrapper);
    lv_label_set_text(precisionLabel, "Precision");

    // The chips are centered in the card while they fit, and scroll when they don't
    auto* precisionCard = lvgl_card_create(wrapper);
    lv_obj_set_size(precisionCard, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(precisionCard, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(precisionCard, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_left(precisionLabel, lv_obj_get_style_pad_left(precisionCard, LV_PART_MAIN), LV_STATE_DEFAULT);

    ctx->precisionChips = lv_obj_create(precisionCard);
    lv_obj_set_size(ctx->precisionChips, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_max_width(ctx->precisionChips, LV_PCT(100), LV_STATE_DEFAULT);
    lv_obj_set_flex_flow(ctx->precisionChips, LV_FLEX_FLOW_ROW);
    lv_obj_set_scroll_dir(ctx->precisionChips, LV_DIR_HOR);
    lv_obj_set_style_bg_opa(ctx->precisionChips, LV_OPA_TRANSP, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ctx->precisionChips, 0, LV_STATE_DEFAULT);
    // The chips' margins leave room for their focus rings and space them apart
    lv_obj_set_style_pad_all(ctx->precisionChips, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(ctx->precisionChips, 0, LV_STATE_DEFAULT);

    for (const auto& option : PRECISION_OPTIONS) {
        auto* chip = lvgl_chip_create(ctx->precisionChips);
        lv_label_set_text(lv_label_create(chip), option.name);
        lv_obj_set_user_data(chip, reinterpret_cast<void*>(static_cast<intptr_t>(option.precision)));
        lv_obj_add_event_cb(chip, onPrecisionChipPressed, LV_EVENT_SHORT_CLICKED, ctx);
    }
    lvgl_grid_navigation_add(ctx->precisionChips);

    auto* precisionHint = lv_label_create(precisionCard);
    lv_obj_set_width(precisionHint, LV_PCT(100));
    lv_label_set_long_mode(precisionHint, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(precisionHint, lvgl_get_text_font(FONT_SIZE_SMALL), LV_STATE_DEFAULT);
    lv_label_set_text(precisionHint, "Approximate location is accurate to a few kilometers. Location based on the IP address is never precise.");

    auto* statusCard = lvgl_card_create(wrapper);
    lv_obj_set_size(statusCard, LV_PCT(100), LV_SIZE_CONTENT);
    ctx->statusLabel = lv_label_create(statusCard);
    lv_obj_set_width(ctx->statusLabel, LV_PCT(100));
    lv_label_set_long_mode(ctx->statusLabel, LV_LABEL_LONG_WRAP);

    updateWidgets(ctx);
}

void destroyWidgets(void* userData) {
    auto* ctx = static_cast<Context*>(userData);
    ctx->enableSwitch = nullptr;
    ctx->precisionChips = nullptr;
    ctx->statusLabel = nullptr;
}

int32_t appMain(int argc, char* argv[]) {
    Context ctx {};
    ctx.appInstanceId = app_scheduler_current_app_id();

    TaskEventGroup eventGroup {};
    task_event_group_construct(&eventGroup);
    ctx.eventGroup = &eventGroup;
    check(task_event_group_claim_bit(&eventGroup, &ctx.enableBit) == ERROR_NONE);
    check(task_event_group_claim_bit(&eventGroup, &ctx.precisionBit) == ERROR_NONE);

    AppEventSubscription appEvents {};
    check(app_event_subscribe(&appEvents, &eventGroup) == ERROR_NONE);
    LocationSubscription locationEvents {};
    check(location_subscribe(&locationEvents, &eventGroup) == ERROR_NONE);

    WindowId window = window_manager_create_ext(ctx.appInstanceId, createWidgets, destroyWidgets, &ctx);

    bool shouldClose = false;
    while (!shouldClose) {
        uint32_t flags = 0;
        task_event_group_wait_any(&eventGroup, &flags, portMAX_DELAY);

        if (flags & ctx.enableBit) {
            location_set_enabled(ctx.requestedEnabled);
        }
        if (flags & ctx.precisionBit) {
            location_set_precision(ctx.requestedPrecision);
        }

        AppEvent event {};
        while (app_event_poll(&appEvents, &event) == ERROR_NONE) {
            if (event.type == APP_EVENT_CLOSE) {
                shouldClose = true;
                break;
            }
        }

        lvgl_lock();
        updateWidgets(&ctx);
        lvgl_unlock();
    }

    window_manager_remove(window);
    check(location_unsubscribe(&locationEvents) == ERROR_NONE);
    check(app_event_unsubscribe(&appEvents) == ERROR_NONE);
    task_event_group_destruct(&eventGroup);
    return 0;
}

} // namespace

extern const ::AppManifest manifest = {
    .id = "tactility.locationsettings",
    .name = "Location",
    .category = APP_CATEGORY_SETTINGS,
    .location = { APP_LOCATION_MEMORY, reinterpret_cast<void*>(appMain) },
    .flags = 0,
    .stack = {}
};

}
