#pragma once

#include "Tactility/hal/i2c/I2c.h"
#include <vector>

namespace tt::hal::i2c {

/**
 * Called by main HAL init to ready the internal state of the I2C HAL.
 * @param[in] configurations HAL configuration for a board
 * @return true on success
 */
bool init(const std::vector<i2c::Configuration>& configurations);

}