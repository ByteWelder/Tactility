#pragma once

#include "Tactility/hal/spi/Spi.h"

#include <vector>
#include <memory>

namespace tt::hal::spi {

/**
 * Called by main HAL init to ready the internal state of the SPI HAL.
 * @param[in] configurations HAL configuration for a board
 * @return true on success
 */
bool init(const std::vector<spi::Configuration>& configurations);

}
