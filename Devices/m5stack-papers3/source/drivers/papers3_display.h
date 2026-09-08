#pragma once

#include <stdint.h>

#include <epdiy.h>

#ifdef __cplusplus
extern "C" {
#endif

struct Papers3DisplayConfig {
    int temperature_celsius;
    enum EpdDrawMode quality_draw_mode;
    enum EpdRotation rotation;
};

#ifdef __cplusplus
}
#endif
