#pragma once

#include <minmea.h>

namespace tt::hal::gps {

/** @return true when the input float is valid (contains non-zero values) */
inline bool isValid(const minmea_float& inFloat) { return inFloat.value != 0 && inFloat.scale != 0; }

}
