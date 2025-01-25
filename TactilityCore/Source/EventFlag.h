#pragma once

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

    enum Flag {
        WaitAny = 0x00000000U, ///< Wait for any flag (default).
        WaitAll = 0x00000001U, ///< Wait for all flags.
        NoClear = 0x00000002U, ///< Do not clear flags which have been specified to wait for.

        Error = 0x80000000U,          ///< Error indicator.
        ErrorUnknown = 0xFFFFFFFFU,   ///< TtStatusError (-1).
        ErrorTimeout = 0xFFFFFFFEU,   ///< TtStatusErrorTimeout (-2).
        ErrorResource = 0xFFFFFFFDU,  ///< TtStatusErrorResource (-3).
        ErrorParameter = 0xFFFFFFFCU, ///< TtStatusErrorParameter (-4).
        ErrorISR = 0xFFFFFFFAU,       ///< TtStatusErrorISR (-6).
    };

    uint32_t set(uint32_t flags) const;
    uint32_t clear(uint32_t flags) const;
    uint32_t get() const;
    uint32_t wait(
        uint32_t flags,
        uint32_t options = WaitAny,
        uint32_t timeout = portMAX_DELAY
    ) const;
};

} // namespace
