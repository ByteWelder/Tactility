#include <Tactility/file/ObjectFile.h>
#include <Tactility/Logger.h>
#include <Tactility/service/gps/GpsService.h>
#include <Tactility/service/ServicePaths.h>

#include <cstring>
#include <unistd.h>

using tt::hal::gps::GpsDevice;

namespace tt::service::gps {

static const auto LOGGER = Logger("GpsService");

bool GpsService::getConfigurationFilePath(std::string& output) const {
    if (paths == nullptr) {
        LOGGER.error("Can't add configuration: service not started");
        return false;
    }

    if (!file::findOrCreateDirectory(paths->getUserDataDirectory(), 0777)) {
        LOGGER.error("Failed to find or create path {}", paths->getUserDataDirectory());
        return false;
    }

    output = paths->getUserDataPath("config.bin");
    return true;
}

bool GpsService::getGpsConfigurations(std::vector<hal::gps::GpsConfiguration>& configurations) const {
    std::string path;
    if (!getConfigurationFilePath(path)) {
        return false;
    }

    // If file does not exist, return empty list
    if (access(path.c_str(), F_OK) != 0) {
        LOGGER.warn("No configurations (file not found: {})", path);
        return true;
    }

    LOGGER.info("Reading configuration file {}", path);
    auto reader = file::ObjectFileReader(path, sizeof(hal::gps::GpsConfiguration));
    if (!reader.open()) {
        LOGGER.error("Failed to open configuration file");
        return false;
    }

    hal::gps::GpsConfiguration configuration;
    while (reader.hasNext()) {
        if (!reader.readNext(&configuration)) {
            LOGGER.error("Failed to read configuration");
            reader.close();
            return false;
        } else {
            configurations.push_back(configuration);
        }
    }

    return true;
}

bool GpsService::addGpsConfiguration(hal::gps::GpsConfiguration configuration) {
    std::string path;
    if (!getConfigurationFilePath(path)) {
        return false;
    }

    auto appender = file::ObjectFileWriter(path, sizeof(hal::gps::GpsConfiguration), 1, true);
    if (!appender.open()) {
        LOGGER.error("Failed to open/create configuration file");
        return false;
    }

    if (!appender.write(&configuration)) {
        LOGGER.error("Failed to add configuration");
        appender.close();
        return false;
    }

    appender.close();
    return true;
}

bool GpsService::removeGpsConfiguration(hal::gps::GpsConfiguration configuration) {
    std::string path;
    if (!getConfigurationFilePath(path)) {
        return false;
    }

    std::vector<hal::gps::GpsConfiguration> configurations;
    if (!getGpsConfigurations(configurations)) {
        LOGGER.error("Failed to get gps configurations");
        return false;
    }

    auto count = std::erase_if(configurations, [&configuration](auto& item) {
        return strcmp(item.uartName, configuration.uartName) == 0 &&
            item.baudRate == configuration.baudRate &&
            item.model == configuration.model;
    });

    auto writer = file::ObjectFileWriter(path, sizeof(hal::gps::GpsConfiguration), 1, false);
    if (!writer.open()) {
        LOGGER.error("Failed to open configuration file");
        return false;
    }

    for (auto& configuration : configurations) {
        writer.write(&configuration);
    }

    writer.close();

    return count > 0;
}

} // namespace tt::service::gps
