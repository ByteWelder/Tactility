#include <app/event.h>
#include <app/manager.h>
#include <app/scheduler.h>

#include <lvgl_window_manager/window_manager.h>

#include <lvgl.h>
#include <lvgl/widgets/toolbar.h>

#include <stdbool.h>

static void create_widgets(lv_obj_t* parent, void* userData) {
    lv_obj_t* toolbar = lvgl_toolbar_create(parent, "Title");
    lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t* label = lv_label_create(parent);
    lv_label_set_text(label, "Hello, world!");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}

int main(int argc, char* argv[]) {
    AppInstanceId app_instance_id = app_scheduler_current_app_id();

    struct AppEventSubscription sub = { .app_instance_id = app_instance_id };
    app_event_subscribe(&sub);

    WindowId window = window_manager_create(app_instance_id, create_widgets, NULL);

    bool should_close = false;
    while (!should_close) {
        struct AppEvent event;
        if (app_event_await(&sub, &event, portMAX_DELAY) != ERROR_NONE) {
            break;
        }
        if (event.type == APP_EVENT_CLOSE) {
            should_close = true;
        }
    }

    window_manager_remove(window);
    app_event_unsubscribe(&sub);

    return 0;
}
