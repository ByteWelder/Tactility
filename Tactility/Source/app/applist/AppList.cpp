#include <app/event.h>
#include <app/manager.h>
#include <app/manifest.h>
#include <app/scheduler.h>

#include <lvgl_window_manager/window_manager.h>

#include <tactility/check.h>

#include <lvgl.h>
#include <algorithm>
#include <cstring>
#include <vector>

#include <lvgl/icons/shared.h>
#include <lvgl/fonts.h>
#include <lvgl/widgets/toolbar.h>

namespace tt::app::applist {

namespace {

uint32_t appListInstanceId = 0;

void onAppPressed(lv_event_t* e) {
    // Fire-and-forget top-level navigation, same as Launcher's own app-launch buttons.
    const auto* manifest = static_cast<const ::AppManifest*>(lv_event_get_user_data(e));
    uint32_t instanceId = 0;
    app_manager_start(manifest->id, &instanceId);
}

void onBackPressed(lv_event_t*) {
    // The global toolbar nav callback (ToolbarConfig.nav_action_callback, set once in
    // Tactility.cpp) only knows how to stop old-model apps, so this new-model app overrides
    // its own toolbar's nav action to close itself instead. Async, non-blocking - must NOT
    // call app_manager_stop() directly here: that bound-waits (thread_join) for this app's
    // own thread to finish, which needs the LVGL lock (window_manager_remove()) - but this
    // callback runs ON the LVGL task, which would deadlock against itself.
    AppEvent event { .type = APP_EVENT_CLOSE, .timestamp = 0, .result = {} };
    app_event_emit(appListInstanceId, &event);
}

void createAppWidget(const ::AppManifest* manifest, lv_obj_t* list) {
    // The new AppManifest has no per-app icon - use a shared generic one for every entry,
    // same fallback the old model used for apps that didn't provide one.
    lv_obj_t* btn = lv_list_add_button(list, LVGL_ICON_SHARED_TOOLBAR, manifest->name);
    lv_obj_t* image = lv_obj_get_child(btn, 0);
    lv_obj_set_style_text_font(image, lvgl_get_shared_icon_font(), LV_PART_MAIN);
    lv_obj_add_event_cb(btn, &onAppPressed, LV_EVENT_SHORT_CLICKED, const_cast<::AppManifest*>(manifest));
}

void collectManifest(const ::AppManifest* manifest, void* context) {
    auto* manifests = static_cast<std::vector<const ::AppManifest*>*>(context);
    manifests->push_back(manifest);
}

void createWidgets(lv_obj_t* parent, void*) {
    auto* toolbar = lvgl_toolbar_create(parent, "Apps");
    lvgl_toolbar_set_nav_action(toolbar, LV_SYMBOL_CLOSE, onBackPressed, nullptr);
    lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t* list = lv_list_create(parent);
    lv_obj_set_width(list, LV_PCT(100));
    lv_obj_align_to(list, toolbar, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

    auto toolbar_height = lv_obj_get_height(toolbar);
    auto parent_content_height = lv_obj_get_content_height(parent);
    lv_obj_set_height(list, parent_content_height - toolbar_height);

    std::vector<const ::AppManifest*> manifests;
    app_manager_for_each_manifest(collectManifest, &manifests);
    std::ranges::sort(manifests, [](const ::AppManifest* a, const ::AppManifest* b) {
        return strcmp(a->name, b->name) < 0;
    });

    for (const auto* manifest: manifests) {
        bool is_valid_category = (manifest->category == APP_CATEGORY_USER) || (manifest->category == APP_CATEGORY_SYSTEM);
        if (is_valid_category && (manifest->flags & APP_MANIFEST_FLAG_HIDDEN) == 0) {
            createAppWidget(manifest, list);
        }
    }
}

int32_t appMain(int argc, char* argv[]) {
    uint32_t appInstanceId = app_scheduler_current_app_id();
    appListInstanceId = appInstanceId;

    TaskEventGroup event_group {};
    task_event_group_construct(&event_group);

    AppEventSubscription sub {};
    check(app_event_subscribe(&sub, &event_group) == ERROR_NONE);

    WindowId window = window_manager_create(appInstanceId, createWidgets, nullptr);

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
    return 0;
}

} // namespace

extern const ::AppManifest manifest = {
    .id = "tactility.applist",
    .name = "Apps",
    .category = APP_CATEGORY_SYSTEM,
    .location = { APP_LOCATION_MEMORY, reinterpret_cast<void*>(appMain) },
    .flags = APP_MANIFEST_FLAG_HIDDEN,
};

} // namespace
