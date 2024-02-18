#include "statusbar.h"

#include "mutex.h"
#include "pubsub.h"
#include "tactility_core.h"
#include "ui/spacer.h"
#include "ui/style.h"

#include "lvgl.h"
#include "lvgl_sync.h"

#define TAG "statusbar"

typedef struct {
    const char* image;
    bool visible;
    bool claimed;
} StatusbarIcon;

typedef struct {
    Mutex* mutex;
    PubSub* pubsub;
    StatusbarIcon icons[STATUSBAR_ICON_LIMIT];
} StatusbarData;

static StatusbarData statusbar_data = {
    .mutex = NULL,
    .icons = {0}
};

typedef struct {
    lv_obj_t obj;
    lv_obj_t* icons[STATUSBAR_ICON_LIMIT];
    PubSubSubscription* pubsub_subscription;
} Statusbar;

static void statusbar_init() {
    statusbar_data.mutex = tt_mutex_alloc(MutexTypeRecursive);
    statusbar_data.pubsub = tt_pubsub_alloc();
    for (int i = 0; i < STATUSBAR_ICON_LIMIT; i++) {
        statusbar_data.icons[i].image = NULL;
        statusbar_data.icons[i].visible = false;
        statusbar_data.icons[i].claimed = false;
    }
}

static void statusbar_ensure_initialized() {
    if (statusbar_data.mutex == NULL) {
        statusbar_init();
    }
}

void statusbar_lock() {
    statusbar_ensure_initialized();
    tt_mutex_acquire(statusbar_data.mutex, TtWaitForever);
}

void statusbar_unlock() {
    tt_mutex_release(statusbar_data.mutex);
}

static void statusbar_constructor(const lv_obj_class_t* class_p, lv_obj_t* obj);
static void statusbar_destructor(const lv_obj_class_t* class_p, lv_obj_t* obj);
static void statusbar_event(const lv_obj_class_t* class_p, lv_event_t* event);

static void update_main(Statusbar* statusbar);

static const lv_obj_class_t statusbar_class = {
    .constructor_cb = &statusbar_constructor,
    .destructor_cb = &statusbar_destructor,
    .event_cb = &statusbar_event,
    .width_def = LV_PCT(100),
    .height_def = STATUSBAR_HEIGHT,
    .group_def = LV_OBJ_CLASS_GROUP_DEF_TRUE,
    .instance_size = sizeof(Statusbar),
    .base_class = &lv_obj_class
};

static void statusbar_pubsub_event(TT_UNUSED const void* message, void* obj) {
    TT_LOG_I(TAG, "event");
    Statusbar* statusbar = (Statusbar*)obj;
    if (tt_lvgl_lock(tt_ms_to_ticks(100))) {
        update_main(statusbar);
        lv_obj_invalidate(&statusbar->obj);
        tt_lvgl_unlock();
    }
}

static void statusbar_constructor(const lv_obj_class_t* class_p, lv_obj_t* obj) {
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    LV_TRACE_OBJ_CREATE("finished");
    Statusbar* statusbar = (Statusbar*)obj;
    statusbar_ensure_initialized();
    statusbar->pubsub_subscription = tt_pubsub_subscribe(statusbar_data.pubsub, &statusbar_pubsub_event, statusbar);
}

static void statusbar_destructor(const lv_obj_class_t* class_p, lv_obj_t* obj) {
    Statusbar* statusbar = (Statusbar*)obj;
    tt_pubsub_unsubscribe(statusbar_data.pubsub, statusbar->pubsub_subscription);
}

static void update_icon(lv_obj_t* image, const StatusbarIcon* icon) {
    if (icon->image != NULL && icon->visible && icon->claimed) {
        lv_obj_set_style_image_recolor(image, lv_color_white(), 0);
        lv_obj_set_style_image_recolor_opa(image, 255, 0);
        lv_image_set_src(image, icon->image);
        lv_obj_remove_flag(image, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(image, LV_OBJ_FLAG_HIDDEN);
    }
}

lv_obj_t* tt_statusbar_create(lv_obj_t* parent) {
    LV_LOG_INFO("begin");
    lv_obj_t* obj = lv_obj_class_create_obj(&statusbar_class, parent);
    lv_obj_class_init_obj(obj);

    Statusbar* statusbar = (Statusbar*)obj;

    lv_obj_set_width(obj, LV_PCT(100));
    lv_obj_set_height(obj, STATUSBAR_HEIGHT);
    tt_lv_obj_set_style_no_padding(obj);
    lv_obj_center(obj);
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);

    lv_obj_t* left_spacer = tt_lv_spacer_create(obj, 1, 1);
    lv_obj_set_flex_grow(left_spacer, 1);

    statusbar_lock();
    for (int i = 0; i < STATUSBAR_ICON_LIMIT; ++i) {
        lv_obj_t* image = lv_image_create(obj);
        lv_obj_set_size(image, STATUSBAR_ICON_SIZE, STATUSBAR_ICON_SIZE);
        tt_lv_obj_set_style_no_padding(image);
        tt_lv_obj_set_style_bg_blacken(image);
        statusbar->icons[i] = image;

        update_icon(image, &(statusbar_data.icons[i]));
    }
    statusbar_unlock();

    return obj;
}

static void update_main(Statusbar* statusbar) {
    statusbar_lock();
    for (int i = 0; i < STATUSBAR_ICON_LIMIT; ++i) {
        update_icon(statusbar->icons[i], &(statusbar_data.icons[i]));
    }
    statusbar_unlock();
}

static void statusbar_event(TT_UNUSED const lv_obj_class_t* class_p, lv_event_t* event) {
    // Call the ancestor's event handler
    lv_result_t result = lv_obj_event_base(&statusbar_class, event);
    if (result != LV_RES_OK) {
        return;
    }

    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t* obj = lv_event_get_target(event);

    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_obj_invalidate(obj);
    } else if (code == LV_EVENT_DRAW_MAIN) {
    }
}

int8_t tt_statusbar_icon_add(const char* _Nullable image) {
    statusbar_lock();
    int8_t result = -1;
    for (int8_t i = 0; i < STATUSBAR_ICON_LIMIT; ++i) {
        if (!statusbar_data.icons[i].claimed) {
            statusbar_data.icons[i].claimed = true;
            statusbar_data.icons[i].visible = (image != NULL);
            statusbar_data.icons[i].image = image;
            result = i;
            TT_LOG_I(TAG, "id %d: added", i);
            break;
        }
    }
    tt_pubsub_publish(statusbar_data.pubsub, NULL);
    statusbar_unlock();
    return result;
}

void tt_statusbar_icon_remove(int8_t id) {
    TT_LOG_I(TAG, "id %d: remove", id);
    tt_check(id >= 0 && id < STATUSBAR_ICON_LIMIT);
    statusbar_lock();
    StatusbarIcon* icon = &statusbar_data.icons[id];
    icon->claimed = false;
    icon->visible = false;
    icon->image = NULL;
    tt_pubsub_publish(statusbar_data.pubsub, NULL);
    statusbar_unlock();
}

void tt_statusbar_icon_set_image(int8_t id, const char* image) {
    TT_LOG_I(TAG, "id %d: set image %s", id, image);
    tt_check(id >= 0 && id < STATUSBAR_ICON_LIMIT);
    statusbar_lock();
    StatusbarIcon* icon = &statusbar_data.icons[id];
    tt_check(icon->claimed);
    icon->image = image;
    tt_pubsub_publish(statusbar_data.pubsub, NULL);
    statusbar_unlock();
}

void tt_statusbar_icon_set_visibility(int8_t id, bool visible) {
    TT_LOG_I(TAG, "id %d: set visibility %d", id, visible);
    tt_check(id >= 0 && id < STATUSBAR_ICON_LIMIT);
    statusbar_lock();
    StatusbarIcon* icon = &statusbar_data.icons[id];
    tt_check(icon->claimed);
    icon->visible = visible;
    tt_pubsub_publish(statusbar_data.pubsub, NULL);
    statusbar_unlock();
}
