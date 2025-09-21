#pragma once

#include <string>
#include <memory>

namespace tt::service {

// Forward declarations
class ServiceManifest;

class ServicePaths {

    std::shared_ptr<const ServiceManifest> manifest;

public:

    explicit ServicePaths(std::shared_ptr<const ServiceManifest> manifest) : manifest(std::move(manifest)) {}

    /**
     * Returns the directory path for the data location for a service.
     * The data directory is intended to survive OS upgrades.
     * The path will not end with a "/".
     */
    std::string getDataDirectory() const;

    /**
     * Returns the full path for an entry inside the data location for a service.
     * The data directory is intended to survive OS upgrades.
     * Configuration data should be stored here.
     * @param[in] childPath the path without a "/" prefix
     */
    std::string getDataPath(const std::string& childPath) const;

    /**
     * Returns the directory path for the system location for a service.
     * The system directory is not intended to survive OS upgrades.
     * You should not store configuration data here.
     * The path will not end with a "/".
     * This is mainly used for core services.
     */
    std::string getSystemDirectory() const;

    /**
     * Returns the full path for an entry inside the system location for an app.
     * The data directory is not intended to survive OS upgrades.
     * You should not store configuration data here.
     * This is mainly used for core apps (system/boot/settings type).
     * @param[in] childPath the path without a "/" prefix
     */
    std::string getSystemPath(const std::string& childPath) const;
};

}