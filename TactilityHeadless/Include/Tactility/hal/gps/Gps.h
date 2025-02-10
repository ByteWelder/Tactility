#pragma once

#include "GpsDevice.h"
#include <minmea.h>

namespace tt::hal::gps {

bool init(const std::vector<GpsDevice::Configuration>& configurations);

inline bool isValid(const minmea_float& inFloat) { return inFloat.value != 0 && inFloat.scale != 0; }

}