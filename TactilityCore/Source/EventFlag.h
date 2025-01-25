#pragma once

#include "CoreTypes.h"
#include "RtosCompatEventGroups.h"
#include <memory>

namespace tt {

/**
 * Wrapper for FreeRTOS xEventGroup.
 */
class EventFlag {
private:

    struct EventGroupHandleDeleter {
        void operator()(EventGroupHandle_t handleToDelete) {
            vEventGroupDelete(handleToDelete);
        }
    };

    std::unique_ptr<std::remove_pointer_t<EventGroupHandle_t>, EventGroupHandleDeleter> handle;

public:
    EventFlag();
    ~EventFlag();
    uint32_t set(uint32_t flags) const;
    uint32_t clear(uint32_t flags) const;
    uint32_t get() const;
    uint32_t wait(
        uint32_t flags,
        uint32_t options = TtFlagWaitAny,
        uint32_t timeout = portMAX_DELAY
    ) const;
};

} // namespace
