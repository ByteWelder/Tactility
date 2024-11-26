#pragma once

#include "CoreTypes.h"

#ifdef ESP_TARGET
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#else
#include "FreeRTOS.h"
#include "event_groups.h"
#endif

namespace tt {

#define TT_API_LOCK_EVENT (1U << 0)

/**
 * Wrapper for FreeRTOS xEventGroup.
 */
class EventFlag {
private:
    EventGroupHandle_t handle;
public:
    EventFlag();
    ~EventFlag();
    uint32_t set(uint32_t flags) const;
    uint32_t clear(uint32_t flags) const;
    uint32_t get() const;
    uint32_t wait(
        uint32_t flags,
        uint32_t options = TtFlagWaitAny,
        uint32_t timeout = TtWaitForever
    ) const;
};

} // namespace
