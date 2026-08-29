// SPDX-License-Identifier: Apache-2.0
#include <sdkconfig.h>

#include <freertos/module.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/portmacro.h>
#include <freertos/task.h>
#include <freertos/timers.h>

extern "C" {

static const ModuleSymbol freertos_module_symbols[] = {
    // Task
    DEFINE_MODULE_SYMBOL(uxTaskGetStackHighWaterMark),
    DEFINE_MODULE_SYMBOL(uxTaskGetNumberOfTasks),
    DEFINE_MODULE_SYMBOL(uxTaskGetTaskNumber),
    DEFINE_MODULE_SYMBOL(uxTaskPriorityGet),
    DEFINE_MODULE_SYMBOL(uxTaskPriorityGetFromISR),
    DEFINE_MODULE_SYMBOL(vTaskDelay),
    DEFINE_MODULE_SYMBOL(vTaskDelete),
    DEFINE_MODULE_SYMBOL(vTaskDeleteWithCaps),
    DEFINE_MODULE_SYMBOL(vTaskSetTimeOutState),
    DEFINE_MODULE_SYMBOL(vTaskPrioritySet),
    DEFINE_MODULE_SYMBOL(vTaskSetTaskNumber),
    DEFINE_MODULE_SYMBOL(vTaskSetThreadLocalStoragePointer),
    DEFINE_MODULE_SYMBOL(vTaskSetThreadLocalStoragePointerAndDelCallback),
    DEFINE_MODULE_SYMBOL(vTaskGetInfo),
    DEFINE_MODULE_SYMBOL(vTaskResume),
    DEFINE_MODULE_SYMBOL(vTaskSuspend),
    DEFINE_MODULE_SYMBOL(xTaskCreate),
    DEFINE_MODULE_SYMBOL(xTaskAbortDelay),
    DEFINE_MODULE_SYMBOL(xTaskCheckForTimeOut),
    DEFINE_MODULE_SYMBOL(xTaskCreatePinnedToCore),
    DEFINE_MODULE_SYMBOL(xTaskCreateStatic),
    DEFINE_MODULE_SYMBOL(xTaskCreateStaticPinnedToCore),
    DEFINE_MODULE_SYMBOL(xTaskCreateWithCaps),
    DEFINE_MODULE_SYMBOL(xTaskCreatePinnedToCoreWithCaps),
    DEFINE_MODULE_SYMBOL(xTaskDelayUntil),
    DEFINE_MODULE_SYMBOL(xTaskGenericNotify),
    DEFINE_MODULE_SYMBOL(xTaskGenericNotifyFromISR),
    DEFINE_MODULE_SYMBOL(ulTaskGenericNotifyTake),
    DEFINE_MODULE_SYMBOL(xTaskGetCurrentTaskHandle),
    DEFINE_MODULE_SYMBOL(xTaskGetTickCount),
    DEFINE_MODULE_SYMBOL(xTaskGetTickCountFromISR),
    DEFINE_MODULE_SYMBOL(pvTaskGetThreadLocalStoragePointer),
    DEFINE_MODULE_SYMBOL(pvTaskIncrementMutexHeldCount),
    // EventGroup
    DEFINE_MODULE_SYMBOL(xEventGroupCreate),
    DEFINE_MODULE_SYMBOL(xEventGroupCreateWithCaps),
    DEFINE_MODULE_SYMBOL(xEventGroupCreateStatic),
    DEFINE_MODULE_SYMBOL(xEventGroupClearBits),
    DEFINE_MODULE_SYMBOL(xEventGroupClearBitsFromISR),
    DEFINE_MODULE_SYMBOL(vEventGroupDelete),
    DEFINE_MODULE_SYMBOL(xEventGroupGetStaticBuffer),
    DEFINE_MODULE_SYMBOL(xEventGroupGetBitsFromISR),
    DEFINE_MODULE_SYMBOL(xEventGroupSetBits),
    DEFINE_MODULE_SYMBOL(xEventGroupSetBitsFromISR),
    DEFINE_MODULE_SYMBOL(xEventGroupSync),
    DEFINE_MODULE_SYMBOL(xEventGroupWaitBits),
    // Queue
    DEFINE_MODULE_SYMBOL(vQueueDelete),
    DEFINE_MODULE_SYMBOL(vQueueDeleteWithCaps),
    DEFINE_MODULE_SYMBOL(vQueueSetQueueNumber),
    DEFINE_MODULE_SYMBOL(vQueueWaitForMessageRestricted),
    DEFINE_MODULE_SYMBOL(uxQueueGetQueueNumber),
    DEFINE_MODULE_SYMBOL(uxQueueMessagesWaiting),
    DEFINE_MODULE_SYMBOL(uxQueueMessagesWaitingFromISR),
    DEFINE_MODULE_SYMBOL(uxQueueSpacesAvailable),
    DEFINE_MODULE_SYMBOL(xQueueCreateCountingSemaphore),
    DEFINE_MODULE_SYMBOL(xQueueCreateCountingSemaphoreStatic),
    DEFINE_MODULE_SYMBOL(xQueueCreateMutex),
    DEFINE_MODULE_SYMBOL(xQueueCreateMutexStatic),
    DEFINE_MODULE_SYMBOL(xQueueCreateSet),
    DEFINE_MODULE_SYMBOL(xQueueGetMutexHolder),
    DEFINE_MODULE_SYMBOL(xQueueGetMutexHolderFromISR),
    DEFINE_MODULE_SYMBOL(xQueueGiveMutexRecursive),
    DEFINE_MODULE_SYMBOL(xQueueTakeMutexRecursive),
    DEFINE_MODULE_SYMBOL(xQueueGenericCreate),
    DEFINE_MODULE_SYMBOL(xQueueGenericCreateStatic),
    DEFINE_MODULE_SYMBOL(xQueueGenericReset),
    DEFINE_MODULE_SYMBOL(xQueueGenericSend),
    DEFINE_MODULE_SYMBOL(xQueueGenericSendFromISR),
    DEFINE_MODULE_SYMBOL(xQueueSemaphoreTake),
    DEFINE_MODULE_SYMBOL(xQueueReceive),
    // Timer
    DEFINE_MODULE_SYMBOL(pvTimerGetTimerID),
    DEFINE_MODULE_SYMBOL(xTimerCreate),
    DEFINE_MODULE_SYMBOL(xTimerCreateStatic),
    DEFINE_MODULE_SYMBOL(xTimerGenericCommand),
    DEFINE_MODULE_SYMBOL(xTimerIsTimerActive),
    DEFINE_MODULE_SYMBOL(xTimerGetExpiryTime),
    DEFINE_MODULE_SYMBOL(xTimerPendFunctionCall),
    DEFINE_MODULE_SYMBOL(xTimerPendFunctionCallFromISR),
    DEFINE_MODULE_SYMBOL(xTimerGetPeriod),
    DEFINE_MODULE_SYMBOL(uxTimerGetReloadMode),
    DEFINE_MODULE_SYMBOL(uxTimerGetTimerNumber),
    DEFINE_MODULE_SYMBOL(vTimerSetReloadMode),
    DEFINE_MODULE_SYMBOL(vTimerSetTimerID),
    DEFINE_MODULE_SYMBOL(vTimerSetTimerNumber),
    // portmacro.h
    DEFINE_MODULE_SYMBOL(vPortYield),
    DEFINE_MODULE_SYMBOL(vPortEnterCritical),
    DEFINE_MODULE_SYMBOL(vPortExitCritical),
#if defined(CONFIG_IDF_TARGET_ESP32P4) || defined(CONFIG_IDF_TARGET_ESP32S3)
    DEFINE_MODULE_SYMBOL(xPortEnterCriticalTimeout),
#endif
#if (configNUM_CORES > 1)
    DEFINE_MODULE_SYMBOL(vPortExitCriticalMultiCore),
#endif
    // freertos_tasks_c_additions.h
    DEFINE_MODULE_SYMBOL(xPortInIsrContext),
    DEFINE_MODULE_SYMBOL(xPortCanYield),
    DEFINE_MODULE_SYMBOL(xPortGetCoreID),
    DEFINE_MODULE_SYMBOL(xPortGetTickRateHz),
    DEFINE_MODULE_SYMBOL(xPortInterruptedFromISRContext),
    DEFINE_MODULE_SYMBOL(__getreent),
    MODULE_SYMBOL_TERMINATOR
};

Module freertos_module = {
    .name = "freertos",
    .symbols = freertos_module_symbols
};

}
