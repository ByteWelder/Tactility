#pragma once

#include <cstdint>
#include "TactilityCoreConfig.h"

namespace tt {

[[deprecated("Define flags as needed")]]
typedef enum {
    TtFlagWaitAny = 0x00000000U, ///< Wait for any flag (default).
    TtFlagWaitAll = 0x00000001U, ///< Wait for all flags.
    TtFlagNoClear = 0x00000002U, ///< Do not clear flags which have been specified to wait for.

    TtFlagError = 0x80000000U,          ///< Error indicator.
    TtFlagErrorUnknown = 0xFFFFFFFFU,   ///< TtStatusError (-1).
    TtFlagErrorTimeout = 0xFFFFFFFEU,   ///< TtStatusErrorTimeout (-2).
    TtFlagErrorResource = 0xFFFFFFFDU,  ///< TtStatusErrorResource (-3).
    TtFlagErrorParameter = 0xFFFFFFFCU, ///< TtStatusErrorParameter (-4).
    TtFlagErrorISR = 0xFFFFFFFAU,       ///< TtStatusErrorISR (-6).
} TtFlag;

[[deprecated("Use bool or specific type")]]
typedef enum {
    TtStatusError = -1,                        ///< Unspecified RTOS error: run-time error but no other error message fits.
    TtStatusErrorTimeout = -2,   ///< Operation not completed within the timeout period.
    TtStatusErrorResource = -3,  ///< Resource not available.
    TtStatusErrorParameter = -4, ///< Parameter error.
    TtStatusErrorISR = -6,                         ///< Not allowed in ISR context: the function cannot be called from interrupt service routines.
} TtStatus;

} // namespace
