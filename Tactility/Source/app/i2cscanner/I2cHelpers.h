#pragma once

#include <string>
#include <cstdint>

namespace tt::app::i2cscanner {

std::string getAddressText(uint8_t address);

std::string getPortNamesForDropdown();

}
