#define LV_USE_PRIVATE_API 1 // For actual lv_obj_t declaration
#include "Statusbar.h"

#include "Mutex.h"
#include "Pubsub.h"
#include "TactilityCore.h"
#include "lvgl/Style.h"

#include "LvglSync.h"
#include "lvgl.h"

namespace tt::lvgl {

#define TAG "statusbar"

struct StatusbarIcon {
    std::string image;
    bool visible = false;
    bool claimed = false;
};

struct StatusbarData {
    Mutex mutex = Mutex(Mutex::TypeRecursive);
    std::shared_ptr<PubSub> pubsub = std::make_shared<PubSub>();
    StatusbarIcon icons[STATUSBAR_ICON_LIMIT] = {};
};

static StatusbarData statusbar_data;

typedef struct {
    lv_obj_t obj;
    lv_obj_t* icons[STATUSBAR_ICON_LIMIT];
    lv_obj_t* battery_icon;
    PubSubSubscription* pubsub_subscription;
} Statusbar;

static bool statusbar_lock(uint32_t timeoutTicks) {
    return statusbar_data.mutex.lock(timeoutTicks);
}

static bool statusbar_unlock() {
    return statusbar_data.mutex.unlock();
}

static void statusbar_constructor(const lv_obj_class_t* class_p, lv_obj_t* obj);
static void statusbar_destructor(const lv_obj_class_t* class_p, lv_obj_t* obj);
static void statusbar_event(const lv_obj_class_t* class_p, lv_event_t* event);

static void update_main(Statusbar* statusbar);

static const lv_obj_class_t statusbar_class = {
    .base_class = &lv_obj_class,
    .constructor_cb = &statusbar_constructor,
    .destructor_cb = &statusbar_destructor,
    .event_cb = &statusbar_event,
    .user_data = nullptr,
    .name = nullptr,
    .width_def = LV_PCT(100),
    .height_def = STATUSBAR_HEIGHT,
    .editable = false,
    .group_def = LV_OBJ_CLASS_GROUP_DEF_TRUE,
    .instance_size = sizeof(Statusbar),
    .theme_inheritable = false
};

static void statusbar_pubsub_event(TT_UNUSED const void* message, void* obj) {
    TT_LOG_I(TAG, "event");
    auto* statusbar = static_cast<Statusbar*>(obj);
    if (lock(kernel::millisToTicks(100))) {
        update_main(statusbar);
        lv_obj_invalidate(&statusbar->obj);
        unlock();
    }
}

static void statusbar_constructor(const lv_obj_class_t* class_p, lv_obj_t* obj) {
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    LV_TRACE_OBJ_CREATE("finished");
    auto* statusbar = (Statusbar*)obj;
    statusbar->pubsub_subscription = tt_pubsub_subscribe(statusbar_data.pubsub, &statusbar_pubsub_event, statusbar);
}

static void statusbar_destructor(TT_UNUSED const lv_obj_class_t* class_p, lv_obj_t* obj) {
    auto* statusbar = (Statusbar*)obj;
    tt_pubsub_unsubscribe(statusbar_data.pubsub, statusbar->pubsub_subscription);
}

static void update_icon(lv_obj_t* image, const StatusbarIcon* icon) {
    if (!icon->image.empty() && icon->visible && icon->claimed) {
        lv_image_set_src(image, icon->image.c_str());
        lv_obj_remove_flag(image, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(image, LV_OBJ_FLAG_HIDDEN);
    }
}

lv_obj_t* statusbar_create(lv_obj_t* parent) {
    LV_LOG_INFO("begin");
    lv_obj_t* obj = lv_obj_class_create_obj(&statusbar_class, parent);
    lv_obj_class_init_obj(obj);

    auto* statusbar = (Statusbar*)obj;

    lv_obj_set_width(obj, LV_PCT(100));
    lv_obj_set_height(obj, STATUSBAR_HEIGHT);
    obj_set_style_no_padding(obj);
    lv_obj_center(obj);
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);

    lv_obj_t* left_spacer = lv_obj_create(obj);
    lv_obj_set_size(left_spacer, 1, 1);
    obj_set_style_bg_invisible(left_spacer);
    lv_obj_set_flex_grow(left_spacer, 1);

    statusbar_lock(TtWaitForever);
    for (int i = 0; i < STATUSBAR_ICON_LIMIT; ++i) {
        lv_obj_t* image = lv_image_create(obj);
        lv_obj_set_size(image, STATUSBAR_ICON_SIZE, STATUSBAR_ICON_SIZE);
        obj_set_style_no_padding(image);
        obj_set_style_bg_blacken(image);
        statusbar->icons[i] = image;

        update_icon(image, &(statusbar_data.icons[i]));
    }
    statusbar_unlock();

    return obj;
}

static void update_main(Statusbar* statusbar) {
    if (statusbar_lock(50 / portTICK_PERIOD_MS)) {
        for (int i = 0; i < STATUSBAR_ICON_LIMIT; ++i) {
            update_icon(statusbar->icons[i], &(statusbar_data.icons[i]));
        }
        statusbar_unlock();
    }
}

static void statusbar_event(TT_UNUSED const lv_obj_class_t* class_p, lv_event_t* event) {
    // Call the ancestor's event handler
    lv_result_t result = lv_obj_event_base(&statusbar_class, event);
    if (result != LV_RES_OK) {
        return;
    }

    lv_event_code_t code = lv_event_get_code(event);
    auto* obj = static_cast<lv_obj_t*>(lv_event_get_target(event));

    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_invalidate(obj);
    } else if (code == LV_EVENT_DRAW_MAIN) {
        // NO-OP
    }
}

int8_t statusbar_icon_add(const std::string& image) {
    statusbar_lock(TtWaitForever);
    int8_t result = -1;
    for (int8_t i = 0; i < STATUSBAR_ICON_LIMIT; ++i) {
        if (!statusbar_data.icons[i].claimed) {
            statusbar_data.icons[i].claimed = true;
            statusbar_data.icons[i].visible = !image.empty();
            statusbar_data.icons[i].image = image;
            result = i;
            TT_LOG_I(TAG, "id %d: added", i);
            break;
        }
    }
    tt_pubsub_publish(statusbar_data.pubsub, nullptr);
    statusbar_unlock();
    return result;
}

int8_t statusbar_icon_add() {
    return statusbar_icon_add("");
}

void statusbar_icon_remove(int8_t id) {
    TT_LOG_I(TAG, "id %d: remove", id);
    tt_check(id >= 0 && id < STATUSBAR_ICON_LIMIT);
    statusbar_lock(TtWaitForever);
    StatusbarIcon* icon = &statusbar_data.icons[id];
    icon->claimed = false;
    icon->visible = false;
    icon->image = "";
    tt_pubsub_publish(statusbar_data.pubsub, nullptr);
    statusbar_unlock();
}

void statusbar_icon_set_image(int8_t id, const std::string& image) {
    TT_LOG_I(TAG, "id %d: set image %s", id, image.empty() ? "(none)" : image.c_str());
    tt_check(id >= 0 && id < STATUSBAR_ICON_LIMIT);
    if (statusbar_lock(50 / portTICK_PERIOD_MS)) {
        StatusbarIcon* icon = &statusbar_data.icons[id];
        tt_check(icon->claimed);
        icon->image = image;
        tt_pubsub_publish(statusbar_data.pubsub, nullptr);
        statusbar_unlock();
    }
}

void statusbar_icon_set_visibility(int8_t id, bool visible) {
    TT_LOG_I(TAG, "id %d: set visibility %d", id, visible);
    tt_check(id >= 0 && id < STATUSBAR_ICON_LIMIT);
    if (statusbar_lock(50 / portTICK_PERIOD_MS)) {
        StatusbarIcon* icon = &statusbar_data.icons[id];
        tt_check(icon->claimed);
        icon->visible = visible;
        tt_pubsub_publish(statusbar_data.pubsub, nullptr);
        statusbar_unlock();
    }
}

} // namespace
