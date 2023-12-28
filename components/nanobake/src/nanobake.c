#include "nanobake.h"
#include "applications/nb_applications.h"
#include "esp_log.h"
#include "m-list.h"
#include "nb_app_i.h"
#include "nb_hardware_i.h"
#include "nb_lvgl_i.h"
// Furi
#include "kernel.h"
#include "record.h"
#include "thread.h"

M_LIST_DEF(thread_ids, FuriThreadId);

#define TAG "nanobake"

thread_ids_t prv_thread_ids;

static void prv_furi_init() {
    // TODO: can we remove the suspend-resume logic?
    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        vTaskSuspendAll();
    }

    furi_record_init();

    xTaskResumeAll();
}

FuriThreadId nanobake_get_app_thread_id(size_t index) {
    return *thread_ids_get(prv_thread_ids, index);
}

size_t nanobake_get_app_thread_count() {
    return thread_ids_size(prv_thread_ids);
}

static void prv_start_app(const App _Nonnull* app) {
    ESP_LOGI(TAG, "Starting %s app \"%s\"", nb_app_type_to_string(app->type), app->name);

    FuriThread* thread = furi_thread_alloc_ex(
        app->name,
        app->stack_size,
        app->entry_point,
        NULL
    );

    if (app->type == SERVICE) {
        furi_thread_mark_as_service(thread);
    }

    furi_thread_set_appid(thread, app->id);
    furi_thread_set_priority(thread, app->priority);
    furi_thread_start(thread);

    FuriThreadId thread_id = furi_thread_get_id(thread);
    thread_ids_push_back(prv_thread_ids, thread_id);
}

__attribute__((unused)) extern void nanobake_start(Config _Nonnull* config) {
    prv_furi_init();

    Devices hardware = nb_hardware_create(config);
    /*NbLvgl lvgl =*/nb_lvgl_init(&hardware);

    thread_ids_init(prv_thread_ids);

    ESP_LOGI(TAG, "Starting apps");

    // Services
    for (size_t i = 0; i < FLIPPER_SERVICES_COUNT; i++) {
        prv_start_app(FLIPPER_SERVICES[i]);
    }

    // System
    for (size_t i = 0; i < FLIPPER_SYSTEM_APPS_COUNT; i++) {
        prv_start_app(FLIPPER_SYSTEM_APPS[i]);
    }

    // User
    for (size_t i = 0; i < config->apps_count; i++) {
        prv_start_app(config->apps[i]);
    }

    ESP_LOGI(TAG, "Startup complete");
}
