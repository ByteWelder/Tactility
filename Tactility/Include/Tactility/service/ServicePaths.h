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
     * The user data directory is intended to survive OS upgrades.
     * The path will not end with a "/".
     */
    std::string getUserDataDirectory() const;

    /**
     * The user data directory is intended to survive OS upgrades.
     * Configuration data should be stored here.
     * @param[in] childPath the path without a "/" prefix
     */
    std::string getUserDataPath(const std::string& childPath) const;

    /**
     * You should not store configuration data here.
     * The path will not end with a "/".
     */
    std::string getAssetsDirectory() const;

    /**
     * You should not store configuration data here.
     * @param[in] childPath the path without a "/" prefix
     */
    std::string getAssetsPath(const std::string& childPath) const;
};

}