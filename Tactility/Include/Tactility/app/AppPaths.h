#pragma once

#include <string>
#include <memory>

namespace tt::app {

// Forward declarations
class AppManifest;

class AppPaths {

    const AppManifest& manifest;

public:

    explicit AppPaths(const AppManifest& manifest) : manifest(manifest) {}

    /**
     * The user data directory is intended to survive OS upgrades.
     * The path will not end with a "/".
     */
    std::string getUserDataPath() const;

    /**
     * The user data directory is intended to survive OS upgrades.
     * Configuration data should be stored here.
     * @param[in] childPath the path without a "/" prefix
     */
    std::string getUserDataPath(const std::string& childPath) const;

    /**
     * You should not store configuration data here.
     * The path will not end with a "/".
     * This is mainly used for core apps (system/boot/settings type).
     */
    std::string getAssetsPath() const;

    /**
     * You should not store configuration data here.
     * This is mainly used for core apps (system/boot/settings type).
     * @param[in] childPath the path without a "/" prefix
     */
    std::string getAssetsPath(const std::string& childPath) const;
};

}