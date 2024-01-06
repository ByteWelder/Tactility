#include "furi_core.h"
#include "services/wifi/wifi.h"
#include "app_manifest.h"
#include "thread.h"
#include "lvgl.h"

typedef struct {
    FuriPubSubSubscription* wifi_subscription;
} Wifi;

static void app_show(Context* context, lv_obj_t* parent) {
    UNUSED(context);

    lv_obj_t* text = lv_label_create(parent);
    lv_label_set_recolor(text, true);
    lv_obj_set_width(text, 200);
    lv_obj_set_style_text_align(text, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(
        text,
        "Wifi?"
    );
    lv_obj_align(text, LV_ALIGN_CENTER, 0, -20);
}

static void wifi_event_callback(const void* message, void* context) {
}

static void app_start(Context* context) {
    Wifi* wifi = malloc(sizeof(Wifi));

    FuriPubSub* wifi_pubsub = wifi_get_pubsub();
    wifi->wifi_subscription = furi_pubsub_subscribe(wifi_pubsub, &wifi_event_callback, wifi);

    context->data = wifi;
}

static void app_stop(Context* context) {
    Wifi* wifi = context->data;
    furi_assert(wifi != NULL);

    FuriPubSub* wifi_pubsub = wifi_get_pubsub();
    furi_pubsub_unsubscribe(wifi_pubsub, wifi->wifi_subscription);
    context->data = NULL;

    free(wifi);
}

AppManifest wifi_app = {
    .id = "wifi",
    .name = "Wi-Fi",
    .icon = NULL,
    .type = AppTypeSystem,
    .on_start = &app_start,
    .on_stop = &app_stop,
    .on_show = &app_show
};
