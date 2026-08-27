#include <app/event.h>
#include <app/manager.h>
#include <app/manifest.h>
#include <app/install.h>
#include <app/scheduler.h>

#include <format>

#include <lvgl/lvgl.h>
#include <lvgl/widgets/toolbar.h>

#include <lvgl_window_manager/window_manager.h>

#include <Tactility/StringUtils.h>
#include <Tactility/app/alertdialog/AlertDialog.h>
#include <Tactility/file/File.h>
#include <Tactility/lvgl/Style.h>

#include <tactility/check.h>
#include <tactility/log.h>

constexpr auto* TAG = "AppDetails";

namespace tt::app::appdetails {

extern const ::AppManifest manifest;

namespace {

struct Context {
    uint32_t appInstanceId;
    std::string targetAppId;
    // findAppManifestById() returns the old-model registry's AppManifest type - AppDetails
    // shows details for apps in that registry regardless of which system they run under.
    AppManifest targetManifest = { };
    uint32_t pendingUninstallDialogId = 0;
};


void onPressUninstall(lv_event_t* event) {
    auto* ctx = static_cast<Context*>(lv_event_get_user_data(event));
    std::vector<std::string> choices = { "Yes", "No" };
    ctx->pendingUninstallDialogId = alertdialog::start(
        ctx->appInstanceId,
        "Confirmation",
        std::format("Uninstall {}?", ctx->targetManifest.name),
        choices
    );
}

void onBackPressed(lv_event_t* event) {
    auto* ctx = static_cast<Context*>(lv_event_get_user_data(event));
    app_event_emit_close(ctx->appInstanceId);
}

void createWidgets(lv_obj_t* parent, void* userData) {
    auto* ctx = static_cast<Context*>(userData);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(parent, 0, LV_STATE_DEFAULT);

    auto title = std::format("{} details", ctx->targetManifest.name);
    auto* toolbar = lvgl_toolbar_create(parent, title.c_str());
    // The global toolbar nav callback only knows how to stop old-model apps.
    lvgl_toolbar_set_nav_action(toolbar, LV_SYMBOL_CLOSE, onBackPressed, ctx);

    auto* wrapper = lv_obj_create(parent);
    lv_obj_set_width(wrapper, LV_PCT(100));
    lv_obj_set_flex_grow(wrapper, 1);
    lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_border_width(wrapper, 0, LV_STATE_DEFAULT);
    lvgl::obj_set_style_bg_invisible(wrapper);

    auto identifier = std::format("Identifier: {}", ctx->targetManifest.id);
    auto* identifier_label = lv_label_create(wrapper);
    lv_label_set_text(identifier_label, identifier.c_str());

    auto* location_label = lv_label_create(wrapper);
    std::string location;
    bool is_internal = ctx->targetManifest.location.type == APP_LOCATION_MEMORY;
    bool is_external = ctx->targetManifest.location.type == APP_LOCATION_PATH;
    if (is_internal) {
        location = "internal";
    } else if (is_external) {
        if (!string::getPathParent(static_cast<const char*>(ctx->targetManifest.location.location), location)) {
            location = "external";
        }
    } else {
        LOG_E(TAG, "Unknown app location type %d", ctx->targetManifest.location.type);
        return;
    }
    std::string location_label_text = std::format("Location: {}", location);
    lv_label_set_text(location_label, location_label_text.c_str());

    if (is_external) {
        auto* uninstall_button = lv_button_create(wrapper);
        lv_obj_set_width(uninstall_button, LV_PCT(100));
        lv_obj_add_event_cb(uninstall_button, onPressUninstall, LV_EVENT_SHORT_CLICKED, ctx);
        auto* uninstall_label = lv_label_create(uninstall_button);
        lv_obj_align(uninstall_label, LV_ALIGN_CENTER, 0, 0);
        lv_label_set_text(uninstall_label, "Uninstall");
    }
}

int32_t appMain(int argc, char* argv[]) {
    uint32_t appInstanceId = app_scheduler_current_app_id();

    Context ctx {};
    ctx.appInstanceId = appInstanceId;
    ctx.targetAppId = (argc > 0) ? argv[0] : std::string();
    if (app_manager_find_manifest(ctx.targetAppId.c_str(), &ctx.targetManifest) != ERROR_NONE) {
        LOG_W(TAG, "App %s not found", ctx.targetAppId.c_str());
        return 0;
    }

    TaskEventGroup event_group {};
    task_event_group_construct(&event_group);

    AppEventSubscription sub {};
    check(app_event_subscribe(&sub, &event_group) == ERROR_NONE);

    WindowId window = window_manager_create(appInstanceId, createWidgets, &ctx);

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
                    if (event.result.launch_id == ctx.pendingUninstallDialogId) {
                        if (event.result.result == 0) { // 0 = Yes
                            app_uninstall(ctx.targetManifest.id);
                            shouldClose = true;
                        }
                        app_manager_stop(event.result.launch_id);
                    }
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

void start(const std::string& appId) {
    const char* argv[] = { appId.c_str() };
    uint32_t instanceId = 0;
    app_manager_start_with_parameters(manifest.id, 1, argv, &instanceId);
}

extern const ::AppManifest manifest = {
    .id = "tactility.appdetails",
    .name = "App Details",
    .category = APP_CATEGORY_SYSTEM,
    .location = { APP_LOCATION_MEMORY, reinterpret_cast<void*>(appMain) },
    .flags = APP_MANIFEST_FLAG_HIDDEN,
};

} // namespace
