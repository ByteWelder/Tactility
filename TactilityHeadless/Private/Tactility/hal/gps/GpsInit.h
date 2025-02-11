#pragma once

#include "Tactility/hal/gps/GpsDevice.h"

namespace tt::hal::gps {

bool init(const std::vector<GpsDevice::Configuration>& configurations);

}
