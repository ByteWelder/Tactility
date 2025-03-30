#pragma once

#include "Tactility/hal/gps/GpsDevice.h"

namespace tt::hal::uart { class Uart; }

namespace tt::hal::gps {

/**
 * Init sequence on UART for a specific GPS model.
 */
bool init(uart::Uart& uart, GpsModel type);

}
