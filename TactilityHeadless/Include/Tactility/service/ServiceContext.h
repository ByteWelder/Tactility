#pragma once

#include "ServiceManifest.h"

#include <Tactility/Mutex.h>

#include <memory>

namespace tt::service {

class Paths;

/**
 * The public representation of a service instance.
 * @warning Do not store references or pointers to these! You can retrieve them via the Loader service.
 */
class ServiceContext {

protected:

    virtual ~ServiceContext() = default;

public:

    /** @return a reference ot the service's manifest */
    virtual const service::ServiceManifest& getManifest() const = 0;

    /** Retrieve the paths that are relevant to this service */
    virtual std::unique_ptr<Paths> getPaths() const = 0;
};

class Paths {

public:

    Paths() = default;
    virtual ~Paths() = default;

    /**
     * Returns the directory path for the data location for a service.
     * The data directory is intended to survive OS upgrades.
     * The path will not end with a "/".
     */
    virtual std::string getDataDirectory() const = 0;

    /**
     * @see getDataDirectory(), but with LVGL prefix.
     */
    virtual std::string getDataDirectoryLvgl() const = 0;

    /**
     * Returns the full path for an entry inside the data location for a service.
     * The data directory is intended to survive OS upgrades.
     * Configuration data should be stored here.
     * @param[in] childPath the path without a "/" prefix
     */
    virtual std::string getDataPath(const std::string& childPath) const = 0;

    /**
     * @see getDataPath(), but with LVGL prefix.
     */
    virtual std::string getDataPathLvgl(const std::string& childPath) const = 0;

    /**
     * Returns the directory path for the system location for a service.
     * The system directory is not intended to survive OS upgrades.
     * You should not store configuration data here.
     * The path will not end with a "/".
     * This is mainly used for core services.
     */
    virtual std::string getSystemDirectory() const = 0;

    /**
     * @see getSystemDirectory(), but with LVGL prefix.
     */
    virtual std::string getSystemDirectoryLvgl() const = 0;

    /**
     * Returns the full path for an entry inside the system location for an app.
     * The data directory is not intended to survive OS upgrades.
     * You should not store configuration data here.
     * This is mainly used for core apps (system/boot/settings type).
     * @param[in] childPath the path without a "/" prefix
     */
    virtual std::string getSystemPath(const std::string& childPath) const = 0;

    /**
     * @see getSystemPath(), but with LVGL prefix.
     */
    virtual std::string getSystemPathLvgl(const std::string& childPath) const = 0;
};

} // namespace
