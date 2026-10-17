#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct Papers3DisplayConfig {
    /** Clockwise rotation in degrees: 0, 90, 180 or 270. 0 is the panel's native 960x540 landscape. */
    uint32_t rotation;
};

#ifdef __cplusplus
}
#endif
