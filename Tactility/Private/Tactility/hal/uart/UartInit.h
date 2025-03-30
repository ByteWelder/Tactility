#pragma once

#include "Tactility/hal/uart/Uart.h"

namespace tt::hal::uart {

bool init(const std::vector<uart::Configuration>& configurations);

}
