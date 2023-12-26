#include "nanobake.h"
#include "nb_hardwarei.h"
#include "nb_lvgli.h"
#include "nb_appi.h"
#include "applications/nb_applications.h"
#include <esp_log.h>
#include <m-list.h>
// Furi
#include <thread.h>
#include <kernel.h>
#include <record.h>

M_LIST_DEF(thread_ids, FuriThreadId);

static const char* TAG = "nanobake";
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

static void prv_start_app(const nb_app_t _Nonnull* app) {
    ESP_LOGI(TAG, "Starting %s app \"%s\"",
         nb_app_type_to_string(app->type),
         app->name
    );

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
    furi_thread_start(thread);

    FuriThreadId thread_id = furi_thread_get_id(thread);
    thread_ids_push_back(prv_thread_ids, thread_id);
}

extern void nanobake_start(nb_config_t _Nonnull* config) {
    prv_furi_init();

    nb_hardware_t hardware = nb_hardware_create(config);
    /*nb_lvgl_t lvgl =*/ nb_lvgl_init(&hardware);

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
