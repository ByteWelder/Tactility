#include <Tactility/lvgl/SystemRecents.h>

#ifdef ESP_PLATFORM
#include <sdkconfig.h>
#endif

#if defined(CONFIG_TT_SYSTEM_GESTURES)

#include <Tactility/TactilityPrivate.h>
#include <Tactility/app/AppIcon.h>

#include <app/event.h>
#include <app/manager.h>
#include <app/manifest.h>

#include <lvgl/devices/indev.h>
#include <lvgl/fonts.h>
#include <lvgl/icons/shared.h>
#include <lvgl/lvgl.h>
#include <lvgl/theme.h>
#include <lvgl/widgets/card.h>
#include <lvgl/widgets/icon_button.h>

#include <lvgl_window_manager/window_manager.h>

#include <cstdint>
#include <cstring>

namespace tt::lvgl {

namespace {

/** How many cards the switcher shows: the most recently used apps. */
constexpr size_t MAX_ENTRIES = 5;
/** Entries per copy while scanning the window stack, so no stack-sized array is needed. */
constexpr size_t SCAN_CHUNK = 8;

struct RecentsEntry {
    WindowId window;
    AppInstanceId app_instance_id;
    char name[APP_MANIFEST_NAME_LENGTH + 1];
    const char* icon;
};

void raise_on_main_dispatcher(void* context) {
    window_manager_raise(static_cast<WindowId>(reinterpret_cast<uintptr_t>(context)));
}

/**
 * Defers to the main dispatcher: window_manager_raise() must not run on the LVGL task, see its
 * warning.
 */
void raiseWindow(WindowId window) {
    dispatchOnMainDispatcher(raise_on_main_dispatcher, reinterpret_cast<void*>(static_cast<uintptr_t>(window)));
}

/** Looks up which app owns a window, so a card action can act on it. */
AppInstanceId findAppInstance(WindowId window) {
    WindowInfo windows[SCAN_CHUNK];
    const size_t total = window_manager_get_windows(nullptr, 0, 0);
    for (size_t offset = 0; offset < total; offset += SCAN_CHUNK) {
        const size_t count = window_manager_get_windows(windows, offset, SCAN_CHUNK);
        for (size_t i = 0; i < count && i < SCAN_CHUNK; i++) {
            if (windows[i].id == window) {
                return windows[i].app_instance_id;
            }
        }
    }
    return 0;
}

void hideOverlay() {
    window_manager_overlay_hide();
}

void onCardClicked(lv_event_t* event) {
    auto* card = lv_event_get_current_target_obj(event);
    const auto window = static_cast<WindowId>(reinterpret_cast<uintptr_t>(lv_obj_get_user_data(card)));
    // Hide first: the app's window is rebuilt below where the (deleted) overlay was
    hideOverlay();
    raiseWindow(window);
}

void onCardGesture(lv_event_t* event) {
    lv_indev_t* indev = lv_indev_active();
    if (indev == nullptr || lv_indev_get_gesture_dir(indev) != LV_DIR_TOP) {
        return;
    }
    auto* card = lv_event_get_current_target_obj(event);
    const auto window = static_cast<WindowId>(reinterpret_cast<uintptr_t>(lv_obj_get_user_data(card)));
    const AppInstanceId app_instance_id = findAppInstance(window);
    // The release must not click the card
    lv_indev_wait_release(indev);
    hideOverlay();
    if (app_instance_id != 0) {
        // The app closes itself, which raises whichever window is below it
        app_event_emit_close(app_instance_id);
    }
}

void onBackdropGesture(lv_event_t* /*event*/) {
    lv_indev_t* indev = lv_indev_active();
    if (indev != nullptr && lv_indev_get_gesture_dir(indev) == LV_DIR_TOP) {
        // The release must not click the pressed widget
        lv_indev_wait_release(indev);
        hideOverlay();
    }
}

void onCloseClicked(lv_event_t* /*event*/) {
    hideOverlay();
}

void createCard(lv_obj_t* parent, const RecentsEntry& entry) {
    auto* card = lvgl_card_create(parent);
    lv_obj_set_size(card, LV_PCT(66), LV_PCT(100));
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(card, lvgl_theme_is_compact() ? 4 : 8, LV_STATE_DEFAULT);
    lv_obj_set_scrollable(card, false);
    lv_obj_set_user_data(card, reinterpret_cast<void*>(static_cast<uintptr_t>(entry.window)));
    lv_obj_add_event_cb(card, onCardClicked, LV_EVENT_SHORT_CLICKED, nullptr);
    // Swiping up closes this card's app
    lv_obj_set_gesture_bubble(card, false);
    lv_obj_add_event_cb(card, onCardGesture, LV_EVENT_GESTURE, nullptr);

    auto* icon = lv_label_create(card);
    lv_obj_set_style_text_font(icon, lvgl_get_shared_icon_large_font(), LV_STATE_DEFAULT);
    lv_label_set_text(icon, entry.icon);

    auto* name = lv_label_create(card);
    lv_obj_set_style_text_font(name, lvgl_get_text_font(FONT_SIZE_DEFAULT), LV_STATE_DEFAULT);
    lv_obj_set_width(name, LV_PCT(100));
    lv_obj_set_style_text_align(name, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
    lv_label_set_long_mode(name, LV_LABEL_LONG_MODE_WRAP);
    lv_label_set_text(name, entry.name);
}

/** Collects the most recent apps, topmost first. */
size_t collectEntries(RecentsEntry* entries) {
    const size_t total = window_manager_get_windows(nullptr, 0, 0);
    const size_t shown = total > MAX_ENTRIES ? MAX_ENTRIES : total;
    if (shown == 0) {
        return 0;
    }

    WindowInfo windows[MAX_ENTRIES];
    window_manager_get_windows(windows, total - shown, shown);

    size_t entry_count = 0;
    for (size_t i = shown; i > 0; i--) {
        const WindowInfo& window = windows[i - 1];
        char app_id[APP_MANIFEST_ID_LENGTH + 1];
        if (app_manager_get_instance_app_id(window.app_instance_id, app_id, sizeof(app_id)) != ERROR_NONE) {
            continue;
        }
        ::AppManifest manifest {};
        if (app_manager_find_manifest(app_id, &manifest) != ERROR_NONE) {
            continue;
        }
        RecentsEntry& entry = entries[entry_count++];
        entry.window = window.id;
        entry.app_instance_id = window.app_instance_id;
        entry.icon = ::tt::app::getAppIcon(manifest);
        strncpy(entry.name, manifest.name, APP_MANIFEST_NAME_LENGTH);
        entry.name[APP_MANIFEST_NAME_LENGTH] = '\0';
    }
    return entry_count;
}

void createWidgets(lv_obj_t* parent, void* /*userData*/) {
    // Swiping up anywhere closes the switcher
    lv_obj_set_gesture_bubble(parent, false);
    lv_obj_add_event_cb(parent, onBackdropGesture, LV_EVENT_GESTURE, nullptr);

    auto* title = lv_label_create(parent);
    lv_label_set_text(title, "Recent apps");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, lvgl_theme_is_compact() ? 4 : 8);

    auto* row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_PCT(100), LV_PCT(60));
    lv_obj_center(row);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(row, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(row, lvgl_theme_is_compact() ? 4 : 8, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(row, lvgl_theme_is_compact() ? 4 : 8, LV_STATE_DEFAULT);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scroll_dir(row, LV_DIR_HOR);

    RecentsEntry entries[MAX_ENTRIES];
    const size_t entry_count = collectEntries(entries);
    for (size_t i = 0; i < entry_count; i++) {
        createCard(row, entries[i]);
    }

    // A keyboard or encoder can't tap outside of a card or swipe up
    if (lvgl_indev_exists(LV_INDEV_TYPE_KEYPAD) || lvgl_indev_exists(LV_INDEV_TYPE_ENCODER)) {
        auto* close_button = lvgl_icon_button_create(parent);
        lv_obj_align(close_button, LV_ALIGN_BOTTOM_MID, 0, lvgl_theme_is_compact() ? -4 : -8);
        lv_obj_add_event_cb(close_button, onCloseClicked, LV_EVENT_SHORT_CLICKED, nullptr);
        auto* close_icon = lv_label_create(close_button);
        lv_obj_set_style_text_font(close_icon, lvgl_get_shared_icon_default_font(), LV_STATE_DEFAULT);
        lv_label_set_text(close_icon, LVGL_ICON_SHARED_CLOSE);
        lv_group_focus_obj(close_button);
    }
}

}

void systemRecentsShow() {
    // Nothing to list (no app has a window)
    if (window_manager_get_windows(nullptr, 0, 0) == 0) {
        return;
    }
    window_manager_overlay_show(createWidgets, nullptr, nullptr);
}

}

#else

namespace tt::lvgl {

void systemRecentsShow() {}

}

#endif
