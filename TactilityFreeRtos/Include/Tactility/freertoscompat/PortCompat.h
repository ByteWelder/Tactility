#pragma once

#include "RTOS.h"

#ifndef ESP_PLATFORM
#define xPortInIsrContext(x) (false)
#endif
