#pragma once

#include "Tactility/hal/gps/GpsDevice.h"

namespace tt::hal::uart { class Uart; }

namespace tt::hal::gps {

/**
 * Called by main HAL init to ready the internal state of the GPS HAL.
 * @param[in] configurations HAL configuration for a board
 * @return true on success
 */
bool init(const std::vector<GpsDevice::Configuration>& configurations);

/**
 * Init sequence on UART for a specific GPS model.
 */
bool init(uart::Uart& uart, GpsModel type);

}
