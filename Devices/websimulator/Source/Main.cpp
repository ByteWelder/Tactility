// SPDX-License-Identifier: Apache-2.0
#include "Main.h"
#include <Tactility/Thread.h>

#include "FreeRTOS.h"
#include "task.h"

#include <tactility/log.h>

constexpr auto* TAG = "FreeRTOS";

namespace websimulator {

static MainFunction mainFunction = nullptr;

void setMain(MainFunction newMainFunction) {
    mainFunction = newMainFunction;
}

static void freertosMainTask(void*) {
    LOG_I(TAG, "starting app_main()");
    assert(websimulator::mainFunction);
    mainFunction();
    LOG_I(TAG, "returned from app_main()");
    vTaskDelete(nullptr);
}

void freertosMain() {
    BaseType_t task_result = xTaskCreate(
        freertosMainTask,
        "main",
        8192,
        nullptr,
        static_cast<UBaseType_t>(tt::Thread::Priority::Normal),
        nullptr
    );

    assert(task_result == pdTRUE);

    // Blocks forever
    vTaskStartScheduler();
}

} // namespace