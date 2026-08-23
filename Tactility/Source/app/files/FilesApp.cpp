#include <Tactility/app/files/View.h>
#include <Tactility/app/files/State.h>

#include <app/event.h>
#include <app/manager.h>
#include <app/manifest.h>

#include <lvgl_window_manager/window_manager.h>

#include <memory>

namespace tt::app::files {

extern const ::AppManifest manifest;

namespace {

struct CreateContext {
    View* view;
    uint32_t appInstanceId;
};

void createWidgets(lv_obj_t* parent, void* userData) {
    auto* ctx = static_cast<CreateContext*>(userData);
    ctx->view->init(ctx->appInstanceId, parent);
}

int32_t appMain(uint32_t appInstanceId, int argc, char* argv[]) {
    auto state = std::make_shared<State>();
    View view(state);
    CreateContext createContext { &view, appInstanceId };

    TaskEventGroup event_group {};
    task_event_group_construct(&event_group);

    AppEventSubscription sub {};
    sub.app_instance_id = appInstanceId;
    app_event_subscribe(&sub, &event_group);

    WindowId window = window_manager_create(appInstanceId, createWidgets, &createContext);

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
                    view.onResult(event.result.launch_id, event.result.result);
                    app_manager_stop(event.result.launch_id);
                    break;
                default:
                    break;
            }
            if (shouldClose) break;
        }
    }

    view.deinit();
    window_manager_remove(window);
    app_event_unsubscribe(&sub);
    task_event_group_destruct(&event_group);

    return 0;
}

} // namespace

extern const ::AppManifest manifest = {
    .id = "tactility.files",
    .name = "Files",
    .category = APP_CATEGORY_SYSTEM,
    .location = { APP_LOCATION_MEMORY, reinterpret_cast<void*>(appMain) },
    .flags = APP_MANIFEST_FLAG_HIDDEN,
};

} // namespace
