// SPDX-License-Identifier: Apache-2.0
#include <lvgl_window_manager/window_manager.h>

#include <lvgl/lvgl.h>

#include <tactility/concurrent/mutex.h>

#include <algorithm>
#include <vector>

namespace {

struct WindowRecord {
    WindowId id;
    uint32_t app_instance_id;
    WindowCreateWidgetsFn create_widgets;
    void* user_data;
};

struct WindowManagerState {
    Mutex mutex {};

    bool started = false;
    WindowManagerScreenInitFn screen_init = nullptr;

    /** The raw, full-size container window_manager_start() creates; owns (and deletion
     * cascades to) whatever the screen-init callback added under it. */
    lv_obj_t* real_root_widget = nullptr;
    /** The stable parent each window's own widget is created under - real_root_widget itself,
     * unless the screen-init callback returned a nested content widget instead. */
    lv_obj_t* content_root_widget = nullptr;

    WindowId next_id = 1;
    /** windows.back() is topmost; only it ever has a live widget (top_widget). */
    std::vector<WindowRecord> windows;
    lv_obj_t* top_widget = nullptr;

    /** Task blocked in window_manager_await_state_change(), if any. */
    TaskHandle_t waiting_task = nullptr;

    WindowManagerState() { mutex_construct(&mutex); }
};

WindowManagerState& state() {
    static WindowManagerState instance;
    return instance;
}

lv_obj_t* build_window_widget(lv_obj_t* content, WindowCreateWidgetsFn create_widgets, void* user_data) {
    if (content == nullptr) {
        return nullptr;
    }
    lvgl_lock();
    lv_obj_t* widget = lv_obj_create(content);
    lv_obj_set_size(widget, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(widget, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(widget, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_radius(widget, 0, LV_STATE_DEFAULT);
    if (create_widgets != nullptr) {
        create_widgets(widget, user_data);
    }
    lvgl_unlock();
    return widget;
}

void delete_widget(lv_obj_t* widget) {
    if (widget == nullptr) {
        return;
    }
    lvgl_lock();
    lv_obj_delete(widget);
    lvgl_unlock();
}

} // namespace

extern "C" {

void window_manager_configure(WindowManagerScreenInitFn screen_init) {
    auto& s = state();
    mutex_lock(&s.mutex);
    s.screen_init = screen_init;
    mutex_unlock(&s.mutex);
}

error_t window_manager_start(void) {
    auto& s = state();

    mutex_lock(&s.mutex);
    if (s.started) {
        mutex_unlock(&s.mutex);
        return ERROR_NONE;
    }
    WindowManagerScreenInitFn screen_init = s.screen_init;
    mutex_unlock(&s.mutex);

    lv_obj_t* real_widget = nullptr;
    lv_obj_t* content_widget = nullptr;

    lvgl_lock();
    lv_obj_t* screen = lv_screen_active();
    if (screen != nullptr) {
        real_widget = lv_obj_create(screen);
        lv_obj_set_size(real_widget, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_pad_all(real_widget, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(real_widget, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_radius(real_widget, 0, LV_STATE_DEFAULT);

        content_widget = (screen_init != nullptr) ? screen_init(real_widget) : nullptr;
        if (content_widget == nullptr) {
            content_widget = real_widget;
        }
    }
    lvgl_unlock();

    if (real_widget == nullptr) {
        return ERROR_RESOURCE;
    }

    mutex_lock(&s.mutex);
    s.real_root_widget = real_widget;
    s.content_root_widget = content_widget;
    s.started = true;
    mutex_unlock(&s.mutex);

    return ERROR_NONE;
}

error_t window_manager_stop(void) {
    auto& s = state();

    mutex_lock(&s.mutex);
    if (!s.started) {
        mutex_unlock(&s.mutex);
        return ERROR_NONE;
    }
    lv_obj_t* widget = s.real_root_widget;
    TaskHandle_t waiter = s.waiting_task;
    s.real_root_widget = nullptr;
    s.content_root_widget = nullptr;
    s.top_widget = nullptr;
    s.windows.clear();
    s.started = false;
    s.waiting_task = nullptr;
    mutex_unlock(&s.mutex);

    if (waiter != nullptr) {
        xTaskNotifyGive(waiter);
    }

    // Deleting the real widget cascades to everything under it - chrome and top_widget alike.
    delete_widget(widget);

    return ERROR_NONE;
}

WindowId window_manager_create(uint32_t app_instance_id, WindowCreateWidgetsFn create_widgets, void* user_data) {
    auto& s = state();

    mutex_lock(&s.mutex);
    if (!s.started) {
        mutex_unlock(&s.mutex);
        return 0;
    }
    lv_obj_t* content = s.content_root_widget;
    lv_obj_t* old_top_widget = s.top_widget;
    TaskHandle_t waiter = s.waiting_task;
    s.waiting_task = nullptr;
    s.top_widget = nullptr;
    WindowId new_id = s.next_id++;
    s.windows.push_back(WindowRecord { new_id, app_instance_id, create_widgets, user_data });
    mutex_unlock(&s.mutex);

    if (waiter != nullptr) {
        xTaskNotifyGive(waiter);
    }

    delete_widget(old_top_widget);
    lv_obj_t* new_widget = build_window_widget(content, create_widgets, user_data);

    mutex_lock(&s.mutex);
    bool still_topmost = !s.windows.empty() && s.windows.back().id == new_id;
    if (still_topmost) {
        s.top_widget = new_widget;
        new_widget = nullptr; // consumed
    }
    mutex_unlock(&s.mutex);

    // Something else became topmost while we were building (e.g. a concurrent create() from
    // another app thread) - discard what we just made.
    delete_widget(new_widget);

    return new_id;
}

void window_manager_remove(WindowId id) {
    auto& s = state();

    mutex_lock(&s.mutex);
    auto iterator = std::find_if(s.windows.begin(), s.windows.end(),
        [id](const WindowRecord& window) { return window.id == id; });
    if (iterator == s.windows.end()) {
        mutex_unlock(&s.mutex);
        return;
    }
    bool was_topmost = (iterator + 1 == s.windows.end());
    s.windows.erase(iterator);

    lv_obj_t* content = s.content_root_widget;
    lv_obj_t* old_widget = nullptr;
    WindowCreateWidgetsFn next_create_widgets = nullptr;
    void* next_user_data = nullptr;
    WindowId next_id = 0;
    bool has_next = false;

    if (was_topmost) {
        old_widget = s.top_widget;
        s.top_widget = nullptr;
        if (!s.windows.empty()) {
            next_create_widgets = s.windows.back().create_widgets;
            next_user_data = s.windows.back().user_data;
            next_id = s.windows.back().id;
            has_next = true;
        }
    }

    TaskHandle_t waiter = s.waiting_task;
    s.waiting_task = nullptr;
    mutex_unlock(&s.mutex);

    if (waiter != nullptr) {
        xTaskNotifyGive(waiter);
    }

    if (!was_topmost) {
        // A buried window was removed - the topmost window's widgets are unaffected.
        return;
    }

    delete_widget(old_widget);
    lv_obj_t* new_widget = has_next ? build_window_widget(content, next_create_widgets, next_user_data) : nullptr;

    mutex_lock(&s.mutex);
    bool still_topmost = has_next && !s.windows.empty() && s.windows.back().id == next_id;
    if (still_topmost) {
        s.top_widget = new_widget;
        new_widget = nullptr; // consumed
    }
    mutex_unlock(&s.mutex);

    delete_widget(new_widget);
}

WindowState window_manager_get_state(WindowId id) {
    auto& s = state();
    mutex_lock(&s.mutex);
    bool is_top = !s.windows.empty() && s.windows.back().id == id;
    mutex_unlock(&s.mutex);
    return is_top ? WINDOW_STATE_GRANTED : WINDOW_STATE_REVOKED;
}

WindowState window_manager_await_state_change(WindowId id, TickType_t timeout) {
    auto& s = state();

    mutex_lock(&s.mutex);
    bool is_top = !s.windows.empty() && s.windows.back().id == id;
    if (!is_top) {
        mutex_unlock(&s.mutex);
        return WINDOW_STATE_REVOKED;
    }
    s.waiting_task = xTaskGetCurrentTaskHandle();
    mutex_unlock(&s.mutex);

    ulTaskNotifyTake(pdTRUE, timeout);

    return window_manager_get_state(id);
}

} // extern "C"
