#pragma once

#include <memory>
#include <Tactility/hal/power/PowerDevice.h>

// Battery measurement via ADC2 channel 4 with 85k/100k divider
std::shared_ptr<tt::hal::power::PowerDevice> createPower();
