#include "EventFlag.h"

#include "Check.h"
#include "CoreDefines.h"

#define TT_EVENT_FLAG_MAX_BITS_EVENT_GROUPS 24U
#define TT_EVENT_FLAG_INVALID_BITS (~((1UL << TT_EVENT_FLAG_MAX_BITS_EVENT_GROUPS) - 1U))

namespace tt {

EventFlag::EventFlag() :
    handle(xEventGroupCreate())
{
    assert(!TT_IS_IRQ_MODE());
    tt_check(handle);
}

EventFlag::~EventFlag() {
    assert(!TT_IS_IRQ_MODE());
}

uint32_t EventFlag::set(uint32_t flags) const {
    assert(handle);
    assert((flags & TT_EVENT_FLAG_INVALID_BITS) == 0U);

    uint32_t rflags;
    BaseType_t yield;

    if (TT_IS_IRQ_MODE()) {
        yield = pdFALSE;
        if (xEventGroupSetBitsFromISR(handle.get(), (EventBits_t)flags, &yield) == pdFAIL) {
            rflags = (uint32_t)ErrorResource;
        } else {
            rflags = flags;
            portYIELD_FROM_ISR(yield);
        }
    } else {
        rflags = xEventGroupSetBits(handle.get(), (EventBits_t)flags);
    }

    /* Return event flags after setting */
    return rflags;
}

uint32_t EventFlag::clear(uint32_t flags) const {
    assert((flags & TT_EVENT_FLAG_INVALID_BITS) == 0U);

    uint32_t rflags;

    if (TT_IS_IRQ_MODE()) {
        rflags = xEventGroupGetBitsFromISR(handle.get());

        if (xEventGroupClearBitsFromISR(handle.get(), (EventBits_t)flags) == pdFAIL) {
            rflags = (uint32_t)ErrorResource;
        } else {
            /* xEventGroupClearBitsFromISR only registers clear operation in the timer command queue. */
            /* Yield is required here otherwise clear operation might not execute in the right order. */
            /* See https://github.com/FreeRTOS/FreeRTOS-Kernel/issues/93 for more info.               */
            portYIELD_FROM_ISR(pdTRUE);
        }
    } else {
        rflags = xEventGroupClearBits(handle.get(), (EventBits_t)flags);
    }

    /* Return event flags before clearing */
    return rflags;
}

uint32_t EventFlag::get() const {
    uint32_t rflags;

    if (TT_IS_IRQ_MODE()) {
        rflags = xEventGroupGetBitsFromISR(handle.get());
    } else {
        rflags = xEventGroupGetBits(handle.get());
    }

    /* Return current event flags */
    return (rflags);
}

uint32_t EventFlag::wait(
    uint32_t flags,
    uint32_t options,
    uint32_t timeout
) const {
    assert(!TT_IS_IRQ_MODE());
    assert((flags & TT_EVENT_FLAG_INVALID_BITS) == 0U);

    BaseType_t wait_all;
    BaseType_t exit_clear;
    uint32_t rflags;

    if (options & WaitAll) {
        wait_all = pdTRUE;
    } else {
        wait_all = pdFAIL;
    }

    if (options & NoClear) {
        exit_clear = pdFAIL;
    } else {
        exit_clear = pdTRUE;
    }

    rflags = xEventGroupWaitBits(
        handle.get(),
        (EventBits_t)flags,
        exit_clear,
        wait_all,
        (TickType_t)timeout
    );

    if (options & WaitAll) {
        if ((flags & rflags) != flags) {
            if (timeout > 0U) {
                rflags = (uint32_t)ErrorTimeout;
            } else {
                rflags = (uint32_t)ErrorResource;
            }
        }
    } else {
        if ((flags & rflags) == 0U) {
            if (timeout > 0U) {
                rflags = (uint32_t)ErrorTimeout;
            } else {
                rflags = (uint32_t)ErrorResource;
            }
        }
    }

    return rflags;
}

} // namespace
