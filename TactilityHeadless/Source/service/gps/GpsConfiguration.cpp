#include "Tactility/service/gps/GpsService.h"

#include <Tactility/file/ObjectFile.h>
#include <cstring>

using tt::hal::gps::GpsDevice;

namespace tt::service::gps {

constexpr const char* TAG = "GpsService";

bool GpsService::getConfigurationFilePath(std::string& output) const {
    if (paths == nullptr) {
        TT_LOG_E(TAG, "Can't add configuration: service not started");
        return false;
    }

    if (!file::findOrCreateDirectory(paths->getDataDirectory(), 0777)) {
        TT_LOG_E(TAG, "Failed to find or create path %s", paths->getDataDirectory().c_str());
        return false;
    }

    output = paths->getDataPath("config.bin");
    return true;
}

bool GpsService::getGpsConfigurations(std::vector<hal::gps::GpsConfiguration>& configurations) const {
    std::string path;
    if (!getConfigurationFilePath(path)) {
        return false;
    }

    auto reader = file::ObjectFileReader(path, sizeof(hal::gps::GpsConfiguration));
    if (!reader.open()) {
        TT_LOG_E(TAG, "Failed to open configuration file");
        return false;
    }

    hal::gps::GpsConfiguration configuration;
    while (reader.hasNext()) {
        if (!reader.readNext(&configuration)) {
            TT_LOG_E(TAG, "Failed to read configuration");
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
        TT_LOG_E(TAG, "Failed to open/create configuration file");
        return false;
    }

    if (!appender.write(&configuration)) {
        TT_LOG_E(TAG, "Failed to add configuration");
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
        TT_LOG_E(TAG, "Failed to get gps configurations");
        return false;
    }

    auto count = std::erase_if(configurations, [&configuration](auto& item) {
        return strcmp(item.uartName, configuration.uartName) == 0 &&
            item.baudRate == configuration.baudRate &&
            item.model == configuration.model;
    });

    auto writer = file::ObjectFileWriter(path, sizeof(hal::gps::GpsConfiguration), 1, false);
    if (!writer.open()) {
        TT_LOG_E(TAG, "Failed to open configuration file");
        return false;
    }

    for (auto& configuration : configurations) {
        writer.write(&configuration);
    }

    writer.close();

    return count > 0;
}

} // namespace tt::service::gps
