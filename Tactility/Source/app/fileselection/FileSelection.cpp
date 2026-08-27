#include "Tactility/app/fileselection/FileSelectionPrivate.h"
#include "Tactility/app/fileselection/View.h"
#include "Tactility/app/fileselection/State.h"

#include <app/event.h>
#include <app/manager.h>
#include <app/manifest.h>
#include <app/scheduler.h>

#include <lvgl_window_manager/window_manager.h>

#include <tactility/check.h>

#include <memory>
#include <string>

namespace tt::app::fileselection {

extern const ::AppManifest manifest;

constexpr auto* TAG = "FileSelection";

namespace {

struct Context {
    uint32_t appInstanceId;
    Mode mode;
    std::shared_ptr<State> state;
    std::unique_ptr<View> view;
    // The eventual appMain() return value - see AlertDialog.cpp's Context::result for why this
    // is a plain (non-atomic) field safely shared between the LVGL thread (writer, before
    // emitting APP_EVENT_CLOSE) and this app's own thread (reader, after waking from it).
    int32_t result = 1; // Cancelled - safety-net default if closed without picking a file
};


// The last picked path. Static rather than per-instance: simple, and in practice only one
// FileSelection dialog is ever open at a time. Written on the LVGL thread (View's select-button
// callback, before emitting APP_EVENT_CLOSE); read by the parent via getLastPath() after
// receiving that event - safe without a lock for the same reason Context::result is (see
// AlertDialog.cpp).
std::string lastPath;

void createWidgets(lv_obj_t* parent, void* userData) {
    auto* ctx = static_cast<Context*>(userData);
    ctx->view->init(parent, ctx->mode);
}

int32_t appMain(int argc, char* argv[]) {
    uint32_t appInstanceId = app_scheduler_current_app_id();
    // argv layout: [0]="existing" or "existing_or_new".

    Context ctx {};
    ctx.appInstanceId = appInstanceId;
    ctx.mode = (argc > 0 && std::string(argv[0]) == "existing_or_new") ? Mode::ExistingOrNew : Mode::Existing;
    ctx.state = std::make_shared<State>();
    ctx.view = std::make_unique<View>(appInstanceId, ctx.state, [&ctx, appInstanceId](const std::string& path) {
        // Runs on the LVGL task (View::onSelectButtonPressed) - must NOT call app_manager_stop()
        // here: that bound-waits (thread_join) for this app's own thread to finish, which needs
        // the LVGL lock (window_manager_remove()) - but this callback runs ON the LVGL task,
        // which would deadlock against itself. The caller reaps this instance via
        // app_manager_stop() after it receives the APP_EVENT_RESULT instead.
        lastPath = path;
        ctx.result = 0;
        app_event_emit_close(appInstanceId);
    });

    TaskEventGroup event_group {};
    task_event_group_construct(&event_group);

    AppEventSubscription sub {};
    check(app_event_subscribe(&sub, &event_group) == ERROR_NONE);

    WindowId window = window_manager_create(appInstanceId, createWidgets, &ctx);

    while (true) {
        task_event_group_wait_any(&event_group, nullptr, portMAX_DELAY);

        bool shouldClose = false;
        AppEvent event {};
        while (app_event_poll(&sub, &event) == ERROR_NONE) {
            if (event.type == APP_EVENT_CLOSE) {
                shouldClose = true;
                break;
            }
        }
        if (shouldClose) break;
    }

    window_manager_remove(window);
    check(app_event_unsubscribe(&sub) == ERROR_NONE);
    task_event_group_destruct(&event_group);

    return ctx.result;
}

} // namespace

std::string getLastPath() {
    return lastPath;
}

uint32_t startForExistingFile(uint32_t callerAppInstanceId) {
    const char* argv[] = { "existing" };
    uint32_t instanceId = 0;
    app_manager_start_for_result(manifest.id, callerAppInstanceId, 1, argv, &instanceId);
    return instanceId;
}

uint32_t startForExistingOrNewFile(uint32_t callerAppInstanceId) {
    const char* argv[] = { "existing_or_new" };
    uint32_t instanceId = 0;
    app_manager_start_for_result(manifest.id, callerAppInstanceId, 1, argv, &instanceId);
    return instanceId;
}

extern const ::AppManifest manifest = {
    .id = "tactility.fileselection",
    .name = "File Selection",
    .category = APP_CATEGORY_SYSTEM,
    .location = { APP_LOCATION_MEMORY, reinterpret_cast<void*>(appMain) },
    .flags = APP_MANIFEST_FLAG_HIDDEN,
};

} // namespace
