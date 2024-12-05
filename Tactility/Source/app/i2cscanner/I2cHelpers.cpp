#include "I2cHelpers.h"
#include "Tactility.h"
#include "StringUtils.h"
#include <iomanip>
#include <vector>

namespace tt::app::i2cscanner {

std::string getAddressText(uint8_t address) {
    std::stringstream stream;
    stream << "0x"
           << std::uppercase << std::setfill ('0')
           << std::setw(2) << std::hex << (uint32_t)address;
    return stream.str();
}

std::string getPortNamesForDropdown() {
    std::vector<std::string> config_names;
    size_t port_index = 0;
    for (const auto& i2c_config: tt::getConfiguration()->hardware->i2c) {
        if (!i2c_config.name.empty()) {
            config_names.push_back(i2c_config.name);
        } else {
            std::stringstream stream;
            stream << "Port " << std::to_string(port_index);
            config_names.push_back(stream.str());
        }
        port_index++;
    }
    return string_join(config_names, "\n");
}

}
