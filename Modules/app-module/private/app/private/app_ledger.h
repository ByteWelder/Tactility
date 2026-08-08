// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <app/instance.h>
#include <app/manifest.h>

#include <tactility/concurrent/mutex.h>
#include <tactility/freertos/freertos.h>
#include <tactility/freertos/task.h>

#include <stdint.h>
#include <string>
#include <unordered_map>

/** A registered/running app instance, as tracked internally by app-module. */
struct AppInstanceRecord {
    uint32_t id;
    const AppManifest* manifest;
    AppInstanceState state;
    /** The FreeRTOS task currently executing AppLoaderApi::run() for this instance; NULL when
     * not running. */
    TaskHandle_t task;

    /** 0 for a top-level launch (app_manager_start()). Non-zero for a modal child launched via
     * app_manager_start_for_result() - the instance that receives this child's APP_EVENT_RESULT. */
    uint32_t parent_id = 0;

    /** The task currently blocked in app_scheduler_stop() for this instance, if any - notified
     * (via xTaskNotifyGive()) as the literal last action app_task_main() takes before
     * vTaskDelete(), so app_scheduler_stop() can't observe completion before the task has
     * actually finished running. See app_scheduler.cpp. */
    TaskHandle_t stop_waiter = nullptr;
};

struct AppLedger {
    std::unordered_map<std::string, const AppManifest*> manifests;
    std::unordered_map<uint32_t, AppInstanceRecord> instances;
    uint32_t next_instance_id = 1;
    Mutex mutex {};

    AppLedger() { mutex_construct(&mutex); }
    ~AppLedger() { mutex_destruct(&mutex); }
};

inline AppLedger& app_ledger() {
    static AppLedger ledger;
    return ledger;
}

/** Frees a deep-copied argv previously built by app_manager_start_with_parameters()/
 * app_manager_start_for_result() (see app_scheduler.cpp's TaskContext::argv) - each
 * individually heap-allocated string, then the array itself. Safe to call with count == 0 /
 * values == nullptr (no-op). */
inline void app_ledger_free_arguments(int count, char** values) {
    if (values == nullptr) {
        return;
    }
    for (int i = 0; i < count; i++) {
        delete[] values[i];
    }
    delete[] values;
}
