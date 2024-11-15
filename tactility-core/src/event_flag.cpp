#include "event_flag.h"

#include "check.h"
#include "core_defines.h"


#ifdef ESP_TARGET
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/portmacro.h"
#else
#include "FreeRTOS.h"
#include "event_groups.h"
#include "portmacro.h"
#endif

#define TT_EVENT_FLAG_MAX_BITS_EVENT_GROUPS 24U
#define TT_EVENT_FLAG_INVALID_BITS (~((1UL << TT_EVENT_FLAG_MAX_BITS_EVENT_GROUPS) - 1U))

EventFlag* tt_event_flag_alloc() {
    tt_assert(!TT_IS_IRQ_MODE());

    EventGroupHandle_t handle = xEventGroupCreate();
    tt_check(handle);

    return ((EventFlag*)handle);
}

void tt_event_flag_free(EventFlag* instance) {
    tt_assert(!TT_IS_IRQ_MODE());
    vEventGroupDelete((EventGroupHandle_t)instance);
}

uint32_t tt_event_flag_set(EventFlag* instance, uint32_t flags) {
    tt_assert(instance);
    tt_assert((flags & TT_EVENT_FLAG_INVALID_BITS) == 0U);

    EventGroupHandle_t hEventGroup = (EventGroupHandle_t)instance;
    uint32_t rflags;
    BaseType_t yield;

    if (TT_IS_IRQ_MODE()) {
        yield = pdFALSE;
        if (xEventGroupSetBitsFromISR(hEventGroup, (EventBits_t)flags, &yield) == pdFAIL) {
            rflags = (uint32_t)TtFlagErrorResource;
        } else {
            rflags = flags;
            portYIELD_FROM_ISR(yield);
        }
    } else {
        rflags = xEventGroupSetBits(hEventGroup, (EventBits_t)flags);
    }

    /* Return event flags after setting */
    return (rflags);
}

uint32_t tt_event_flag_clear(EventFlag* instance, uint32_t flags) {
    tt_assert(instance);
    tt_assert((flags & TT_EVENT_FLAG_INVALID_BITS) == 0U);

    EventGroupHandle_t hEventGroup = (EventGroupHandle_t)instance;
    uint32_t rflags;

    if (TT_IS_IRQ_MODE()) {
        rflags = xEventGroupGetBitsFromISR(hEventGroup);

        if (xEventGroupClearBitsFromISR(hEventGroup, (EventBits_t)flags) == pdFAIL) {
            rflags = (uint32_t)TtStatusErrorResource;
        } else {
            /* xEventGroupClearBitsFromISR only registers clear operation in the timer command queue. */
            /* Yield is required here otherwise clear operation might not execute in the right order. */
            /* See https://github.com/FreeRTOS/FreeRTOS-Kernel/issues/93 for more info.               */
            portYIELD_FROM_ISR(pdTRUE);
        }
    } else {
        rflags = xEventGroupClearBits(hEventGroup, (EventBits_t)flags);
    }

    /* Return event flags before clearing */
    return (rflags);
}

uint32_t tt_event_flag_get(EventFlag* instance) {
    tt_assert(instance);

    EventGroupHandle_t hEventGroup = (EventGroupHandle_t)instance;
    uint32_t rflags;

    if (TT_IS_IRQ_MODE()) {
        rflags = xEventGroupGetBitsFromISR(hEventGroup);
    } else {
        rflags = xEventGroupGetBits(hEventGroup);
    }

    /* Return current event flags */
    return (rflags);
}

uint32_t tt_event_flag_wait(
    EventFlag* instance,
    uint32_t flags,
    uint32_t options,
    uint32_t timeout
) {
    tt_assert(!TT_IS_IRQ_MODE());
    tt_assert(instance);
    tt_assert((flags & TT_EVENT_FLAG_INVALID_BITS) == 0U);

    EventGroupHandle_t hEventGroup = (EventGroupHandle_t)instance;
    BaseType_t wait_all;
    BaseType_t exit_clr;
    uint32_t rflags;

    if (options & TtFlagWaitAll) {
        wait_all = pdTRUE;
    } else {
        wait_all = pdFAIL;
    }

    if (options & TtFlagNoClear) {
        exit_clr = pdFAIL;
    } else {
        exit_clr = pdTRUE;
    }

    rflags = xEventGroupWaitBits(
        hEventGroup,
        (EventBits_t)flags,
        exit_clr,
        wait_all,
        (TickType_t)timeout
    );

    if (options & TtFlagWaitAll) {
        if ((flags & rflags) != flags) {
            if (timeout > 0U) {
                rflags = (uint32_t)TtStatusErrorTimeout;
            } else {
                rflags = (uint32_t)TtStatusErrorResource;
            }
        }
    } else {
        if ((flags & rflags) == 0U) {
            if (timeout > 0U) {
                rflags = (uint32_t)TtStatusErrorTimeout;
            } else {
                rflags = (uint32_t)TtStatusErrorResource;
            }
        }
    }

    /* Return event flags before clearing */
    return (rflags);
}
